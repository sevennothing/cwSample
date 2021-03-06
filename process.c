/*******************************************************************************
 * copyright ,2015
 * FileName:process.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 *     信号仅对有效信元进行消除抖动;
 *     目前仅实现RTC定时模式;
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <stdio.h>

#include <stdio.h>  
#include <linux/rtc.h>  
#include <sys/ioctl.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <errno.h>  
#include <stdlib.h>  
#include <time.h>
#include <memory.h>

#include "cw_gpio.h"
#include "process.h"




static int init_timer(void);
static int set_timer(int fd, unsigned int freq);
static int free_timer(int fd);
static int wait_for_timer(int fd);
static int soft_dithering_pass(struct signalProcess *sp, char *flag);
static int stream_fifo_write(struct signalProcess *sp, char v);
static int find_transmit_point(struct signalProcess *sp, int bits);

static int modifyFlag = 0;


struct signalProcess *require_signal_process(void)
{
	struct signalProcess  *sp = calloc(1,sizeof(struct signalProcess));
	sp->timer_mode = RTC_MODE;

	sp->sampleFreq_Hz = TRIG_FREQ;

	sp->verbose = 0;

	sp->fd = init_timer();

	if(sp->fd == 0){
		printf("require TIMER faild\n");
		free(sp);
		return NULL;
	}

	memset(sp->level_stream, 0xff, STREAM_MAX_SECONDS * 4);

	sp->soft_dithering_pass = soft_dithering_pass;

	sp->minCZ = 0xFFFF;
	sp->minCO = 0xFFFF;
	//sp->setup_voice = setupVoice;

	modifyFlag = 0;

	printf("reauire signal process successed.\n");

	return sp;

}

int modify_signal_process(struct signalProcess *sp)
{
	modifyFlag = 1;

	return 0;
}


int destory_signal_process(struct signalProcess *sp)
{
	if(sp == NULL)
		return -1;

	if(sp->fd != 0)
		free_timer(sp->fd);

	free(sp);
	return 0;
}

int run_process(struct signalProcess *sp)
{
	int ret = 0;
	int morseP = 0;	 /* morse 字符位置 */
	int streamP = 0; /* 电平流的位置*/
	int start = STOP_STATUS; /* -1 为初始态或停止态 */
	int k;

	struct timespec ts;
	struct timespec last_ts;
	int runtime = 0;



	int level = INVALID_LEVEL;

	if(sp == NULL){
		printf("%s: invalid signal process handle\n", __func__);
		return -1;
	}
	if(sp->get_signal_cb == NULL){
		printf("%s: no get signal callback fuction\n", __func__);
		return -1;
	}

	clock_gettime(CLOCK_MONOTONIC,&last_ts);

	set_timer(sp->fd, sp->sampleFreq_Hz);
	sp->curCinva = 0;

	while(1){
		if(modifyFlag){
			set_timer(sp->fd, sp->sampleFreq_Hz);
			modifyFlag = 0;
		}

		ret = wait_for_timer(sp->fd);
		if(ret < 0){

		}

		level = sp->get_signal_cb();

		NEED_TREIG_START(level, sp, start)

		if(start > 0){
			stream_fifo_write(sp, level);
			if((start % 3) == 0)
				find_transmit_point(sp, start);	

			if(sp->decode_enable && (sp->decode_process != NULL)){
				//TODO: 电信号解码成morse


			}

			if(sp->transmit_enable && sp->transmit_cb != NULL){
				//TODO: 网络传输采样信息
				if(sp->curCinva < 7){

				}else if( sp->curCinva >= (sp->minCO * 7)){
					/*启动传输*/	
					if(sp->verbose){
						printf(">>%d-%d ",sp->minCZ, sp->minCO);
						fflush(stdout);
					}
#ifdef  PADDING_INVALID_CODE
					int cbit = 0;
					int cb = 0;
					cbit = start % 8;
					if(cbit != 0){
						cbit = 8 - cbit;
						// 插入补齐 TODO: 插值算法需要优化
						for(cb = 0; cb < cbit; cb++){
							stream_fifo_write(sp, INVALID_LEVEL);
						}
					}
					sp->transmit_cb((char *)sp->level_stream, (start + cbit) / 8);
#else
					int byteCnt = 0;
					byteCnt= start / 8;
					if((start % 8) > 0)
						byteCnt++;

					char *tbuf = malloc(byteCnt + 2); // 长度占2个字节
					memcpy(tbuf + 2, (char *)sp->level_stream,  byteCnt);
					*tbuf = (unsigned short)start;   // 标识位长度
					sp->transmit_cb(tbuf, byteCnt + 2);
					free(tbuf);
#endif
					start = STOP_STATUS;
					sp->minCZ = 0xFFFF;
					sp->minCO = 0xFFFF;
				}

			}
			if(start >= (STREAM_MAX_SECONDS * 32)){
				printf("drop this start stream!\n");
				start = STOP_STATUS;
			}
			//if(sp->verbose && (start > 0)){
			if(sp->verbose){
				clock_gettime(CLOCK_MONOTONIC,&ts);
				runtime = (ts.tv_sec - last_ts.tv_sec) * 1000 + (ts.tv_nsec - last_ts.tv_nsec)/1000000;
				last_ts = ts;
				//printf("get io : %d(%d)\n", level, runtime);
				printf("get io(%02x): ", (unsigned char)start);
				for(k=0; k < STREAM_MAX_SECONDS; k++){
					if(sp->verbose == 1){
						/* 按16进制显示*/
						printf(" %08x", sp->level_stream[k]);
					}else if(sp->verbose == 2){
						int j=0;
						/* 按二进制显示*/
						for(j=0; j<32; j++)
							printf("%d", (sp->level_stream[k] >> j) & 0x01);
						printf(" ");
					}

				}

				printf("(%d)\n",runtime);
			}

		}
		

	}

}

static int soft_dithering_pass(struct signalProcess *sp, char *flag)
{
	usleep(15000); /* 10ms 延迟 */
	if(VALID_LEVEL ==  sp->get_signal_cb()) 
		*flag = 1;  /* 有效 */
	else
		*flag = 0; /* 抖动 */

}

static int stream_fifo_write(struct signalProcess *sp, char v)
{
	int i = 0;
	char l = v & 0x01;
	if(l == INVALID_LEVEL)
		sp->curCinva++;
	else
		sp->curCinva = 0;
	for(i = 0; i < STREAM_MAX_SECONDS; i++){
		char Hbit = (sp->level_stream[i] & 0x80000000) ? 1:0;
		sp->level_stream[i] = (((sp->level_stream[i] & 0x7fffffff) << 1 ) | l);
		l = Hbit;
	}

}

static int find_transmit_point(struct signalProcess *sp, int bits)
{
	int i,j;
	unsigned int c = 0;
	unsigned int co = 0;
	unsigned int minCZ = 0xffff ;//最少连续0个数
	unsigned int minCO = 0xffff ;//最少连续1个数
	char new = 0;
	unsigned char words = bits/32;
	unsigned char lastBit = bits % 32;


	if(lastBit > 0){
		j = lastBit - 1;
		words++;
	}else{
		j = 31;
	}
	//scan zero min
	for(i = words; i>0; i--){
		for(; j >=0; j--){
			if((sp->level_stream[i-1] & (1<< j)) == 0){
				//This is zero
				if((new == 1) &&(co > 0 && co < minCO)){
					minCO = co;
				}
				co = 0;
				new = 0;
				c++;
			}else{
				if((new == 0) && (c > 0) && (c < minCZ)){
					minCZ = c;
				}
				c = 0;
				new = 1;
				co++;

			}
		}
		j = 31;
	}
	if(sp->minCZ > minCZ)
		sp->minCZ = minCZ;
	if(sp->minCO > minCO)
		sp->minCO = minCO;

	//printf("%d:CO=%d(%d), CZ=%d\n",bits, sp->minCO,minCO,sp->minCZ);
	return 0;

}

static int init_timer(void)
{  
	int fd = open ("/dev/rtc0", O_RDONLY);  

	if(fd < 0)  
	{  
		perror("open");  
		//exit(errno);  
		return -1;
	}

	return fd;
}



static int set_timer(int fd, unsigned int freq)
{
	unsigned int trigFreq;
	if(freq == 0)
		trigFreq = TRIG_FREQ;
	else
		trigFreq = freq;

	/*Set the freq as 32Hz*/  
	if(ioctl(fd, RTC_IRQP_SET, trigFreq) < 0)  
	{  
		perror("ioctl(RTC_IRQP_SET)");  
		close(fd);  
		exit(errno);  
	}  

	/* Enable periodic interrupts */  
	if(ioctl(fd, RTC_PIE_ON, 0) < 0)  
	{  
		perror("ioctl(RTC_PIE_ON)");  
		close(fd);  
		exit(errno);  
	}  

}

static int wait_for_timer(int fd)
{
	unsigned long data = 0;
	if(read(fd, &data, sizeof(unsigned long)) < 0)  
	{  
		perror("read");  
		ioctl(fd, RTC_PIE_OFF, 0);  
		close(fd);  
		//exit(errno);  
		return -1;

	}  
	//printf("timer\n");  

	return 0;

}

static int free_timer(int fd)
{
	/* Disable periodic interrupts */  
	ioctl(fd, RTC_PIE_OFF, 0);  
	close(fd);  

	return 0;  
}


