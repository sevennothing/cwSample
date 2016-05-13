/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:main.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "cw_gpio.h"
#include "process.h"
#include "trans.h"



struct gpioOper *cwGpio;
struct signalProcess *cwSp;

#ifdef SELF_TEST_FOR_PIPE
int pipe_fd[2];
#endif

#ifdef ENABLE_DEUG_SIM

//char sos[] = {1,1,1,1,1,1,1,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,1,1,1,1,1,1,1};
char sos[] = {0,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1};

char sos_sim_dithering[] = {1,1,1,1,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,1,1,1,1,1,1};

static void inline signal_level_sim(int *val)
{
	
	if(cwSp->dithering_pass_enable){
		static int len = sizeof(sos_sim_dithering);
		static int pos = 0;

		*val = sos_sim_dithering[pos++];
		if(pos == len)
			pos = 0;

	}else{
		static int len = sizeof(sos);
		static int pos = 0;

		*val = sos[pos++];
		if(pos == len)
			pos = 0;

	}

//	*val = 0;

}
#else
static void inline signal_level_sim(int *val)
{
}
#endif


int get_signal_level(void)
{
	int val = 0;

	val = get_gpio_value(cwGpio);

	/* 桩 */
	signal_level_sim(&val);

	return val;

}


int main(int argc, char **argv)
{

	int i = 0;
	int verbose = 0;
	int transmit_n = 0;
	pid_t recvProcPid; 


#define ARG_IS(s) (strncmp(argv[i], s, strlen(s))==0) 
	for(i=0; i < argc; i++){
		if(ARG_IS("-v")){
			if(argv[i+1][0] == '-')
				verbose = 1;
			else
				verbose = atoi(argv[i+1]);
		}

		if(ARG_IS("-n"))
			transmit_n = atoi(argv[i+1]);

	}

	setuid(geteuid());
	setgid(getegid());

	//TODO: param process
	
	cwGpio = init_gpio();

	cwSp = require_signal_process();

	if(cwSp == NULL){
		exit(-1);
	}

	if(transmit_n > 0){
		cwSp->transmit_enable = 1;
		cwSp->transmit_pos = transmit_n * cwSp->sampleFreq_Hz;
		cwSp->transmit_cb = transmit_packet;
	}

	//printf("transmit_pos=%d(%d)\n", cwSp->transmit_pos, transmit_n );
		
	cwSp->verbose = verbose;

	cwSp->get_signal_cb = get_signal_level;


	/* 启用软件消除抖动 */
//	cwSp->dithering_pass_enable = 1;

	//modify_signal_process(cwSp);
	

	/*  用于测试 */
#ifdef SELF_TEST_FOR_PIPE
	if(pipe(pipe_fd)<0){
		printf("pipe create error\n");
		return -1;
	}
#endif

	/* 从网络接收 CW 进程处理*/
	if((recvProcPid = fork()) < 0)
	{
		perror("fail to fork recevie process process");
		return -1;
	}else if( recvProcPid == 0){
		// recev process
		char buff[1024];
		int ret;
		struct pcmConf g_pcmPlay;

#ifndef  SOUND_CARD
		g_pcmPlay.bit = 8;
		g_pcmPlay.channels = 1;
		g_pcmPlay.sampleFrequency = 400;
		g_pcmPlay.datablock = 1;

		
		init_pcm_play(&g_pcmPlay);
		printf("PCM confi:\n");
		printf("  采样位数:%d\n", g_pcmPlay.bit);
		printf("  通道数:%d\n", g_pcmPlay.channels);
		printf("  采样频率:%d\n", g_pcmPlay.sampleFrequency);
		printf("  datablock:%d\n", g_pcmPlay.datablock);
		printf("  frames:%d\n", g_pcmPlay.frames);
		printf("  size:%d\n", g_pcmPlay.size);
		printf("... %x\n", g_pcmPlay.buffer);
#else
		g_pcmPlay.bit = 8;
		g_pcmPlay.sampleFrequency = 8000;
		g_pcmPlay.size = 8192;
		
		//g_pcmPlay.sampleFrequency = 1024; /* 必须设置TRIG_FREQ 的整数倍 */
		//g_pcmPlay.size = 1024;

		g_pcmPlay.cwFrequency = 800;
		g_pcmPlay.volume   =  70;
		
		init_pcm_play(&g_pcmPlay);
		printf("PCM confi:\n");
		printf("  采样位数:%d\n", g_pcmPlay.bit);
		printf("  采样频率:%d\n", g_pcmPlay.sampleFrequency);
		printf("  size:%d\n", g_pcmPlay.size);
		printf("  CW-Freq:%d\n", g_pcmPlay.cwFrequency);
		printf("  CW-AMP:%d\n", g_pcmPlay.volume);

		if(g_pcmPlay.sampleFrequency % TRIG_FREQ){
			printf("warning: player sample frequency can't be divisible by TRIGE_FREQ(%d)\n", TRIG_FREQ);
		}

#endif


#ifdef SELF_TEST_FOR_PIPE
		close(pipe_fd[1]); //关闭写端 
#endif
		while(1){

			ret = recv_packet(buff, 8);

			if(ret > 0){
				//cwSp->pcm_play(buff, ret);
				pcmPlay(&g_pcmPlay,buff,ret);
			}
		}
#ifdef SELF_TEST_FOR_PIPE
		close(pipe_fd[0]); //关闭读端 
#endif

	}else{

#ifdef SELF_TEST_FOR_PIPE
		close(pipe_fd[0]); //关闭读端 
#endif

		run_process(cwSp);

#ifdef SELF_TEST_FOR_PIPE
		close(pipe_fd[1]); //关闭写端 
#endif
	}
	

	//test_process(timerFd, cwGpio);
	destory_signal_process(cwSp);

	/**  free system */

	free_gpio(cwGpio);

	return 0;

}


