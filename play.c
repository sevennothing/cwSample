/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:play.c
 * Author: caijun.Li    Date:2016-05-12
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcm.h"
#include "play.h"


/* program settings */
int pcm = 1;
int hvox = 0;

/* character time, space time (in samples) */
int cTime, sTime;
/* sample rate (samples per second) */
int sample_rate=44100;

/* get the dit-time for characters */
int getCharacterTime(int c_rate) {
	// dit time is 1200 msec at 1 WPM
	// this returns the number of samples per dit
	// at the desired character rate
	float tbase = sample_rate/1000.0 * 1200;
	return (int)(tbase/c_rate);
}

/* get the dit-time for space between characters */
int getSpaceTime(int c_rate, int w_rate) {
	/* NONB helped with this section. Thanks Nate! */
	int t_total;
	int t_chars;
	int t_space;

	if (w_rate < 5) w_rate = 5;
	if (w_rate >= c_rate) return getCharacterTime(c_rate);

	/* spaces take longer but how much longer? */
	t_total = getCharacterTime(w_rate) * 50;
	t_chars = getCharacterTime(c_rate) * 36;

	t_space = t_total - t_chars;
	return t_space / 14;
}

/* morse-speak: */
void dit(FILE *out) {
	mark(hvox, cTime, out);
	space(hvox, cTime, out);
}
void dah(FILE *out) {
	mark(hvox, cTime * 3, out);
	space(hvox, cTime, out);
}
void err(FILE *out) {
}

void cspace(FILE *out) {
	space(hvox, sTime*2, out);
}
void wspace(FILE *out) {
	space(hvox, sTime*4, out);
	fflush(out);
}

void setupVoice(int hz, int amp) {
	/* freq, amplitude, zero, sample rate */
	hvox = voiceFactory(hz, amp, 128, sample_rate);
	setRisetime(hvox, RISETIME);
	setFalltime(hvox, FALLTIME);
}




/*****************************  alas  Driver  ******************************/
int init_pcm_play(struct pcmConf *pcm)
{
	int rc;
	int ret;
	unsigned int val;
	snd_pcm_hw_params_t* params;//硬件信息和PCM流配置
	int dir=0;


	rc=snd_pcm_open(&(pcm->handle), "default", SND_PCM_STREAM_PLAYBACK, 0);
	if(rc<0)
	{
		perror("\nopen PCM device failed:");
		exit(1);
	}


	snd_pcm_hw_params_alloca(&params); //分配params结构体
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_alloca:");
		exit(1);
	}
	rc=snd_pcm_hw_params_any(pcm->handle, params);//初始化params
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_any:");
		exit(1);
	}
	rc=snd_pcm_hw_params_set_access(pcm->handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
	if(rc<0)
	{
		perror("\nsed_pcm_hw_set_access:");
		exit(1);

	}

	//采样位数
	switch(pcm->bit/8)
	{
		case 1:snd_pcm_hw_params_set_format(pcm->handle, params, SND_PCM_FORMAT_U8);
			   break ;
		case 2:snd_pcm_hw_params_set_format(pcm->handle, params, SND_PCM_FORMAT_S16_LE);
			   break ;
		case 3:snd_pcm_hw_params_set_format(pcm->handle, params, SND_PCM_FORMAT_S24_LE);
			   break ;

	}
	rc=snd_pcm_hw_params_set_channels(pcm->handle, params, pcm->channels); //设置声道,1表示单声>道，2表示立体声
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_set_channels:");
		exit(1);
	}
	val = pcm->frequency;
	rc=snd_pcm_hw_params_set_rate_near(pcm->handle, params, &val, &dir); //设置>频率
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_set_rate_near:");
		exit(1);
	}

	rc = snd_pcm_hw_params(pcm->handle, params);
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params: ");
		exit(1);
	}

	rc=snd_pcm_hw_params_get_period_size(params, &(pcm->frames), &dir); /*获取周期
																   长度*/
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}

	pcm->size = pcm->frames * (pcm->datablock); /*4 代表数据快长度*/

	pcm->buffer =(char*)malloc(pcm->size);


	return 0;
}

void free_pcm_play(struct pcmConf *pcm)
{

	snd_pcm_drain(pcm->handle);
	snd_pcm_close(pcm->handle);
	free(pcm->buffer);
}


/***************************************************************************/
#if 0
	while (1)
	{
		memset(buffer,0,sizeof(buffer));
		ret = fread(buffer, 1, size, fp);
		if(ret == 0)
		{
			printf("歌曲写入结束\n");
			break;
		}
		else if (ret != size)
		{
		}
		// 写音频数据到PCM设备 
		while(ret = snd_pcm_writei(handle, buffer, frames)<0)
		{
			usleep(2000); 
			if (ret == -EPIPE)
			{
				/* EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				//完成硬件参数设置，使设备准备好 
				snd_pcm_prepare(handle);
			}
			else if (ret < 0)
			{
				fprintf(stderr,
						"error from writei: %s\n",
						snd_strerror(ret));
			}
		}

	}
#endif


void pcmPlay(struct pcmConf *g_pcmPlay, char buff[], int len)
{
	int ret;
		FILE *fp;
		fp=fopen("sos.wav","rb");
		if(fp==NULL)
		{
			perror("open file failed:\n");
			exit(1);
		}

		fseek(fp,58,SEEK_SET); //定位歌曲到数据区

		while(1){
			//memset(g_pcmPlay->buffer, 0x8,g_pcmPlay.size);
			memset(g_pcmPlay->buffer,0,sizeof(g_pcmPlay->buffer));

			ret = fread(g_pcmPlay->buffer, 1, g_pcmPlay->size, fp);
			if(ret == 0)
			{
				printf("歌曲写入结束\n");
				//fseek(fp,58,SEEK_SET); //定位歌曲到数据区
				break;
			}

			printf("?%x:%x  %x : %d \n", g_pcmPlay->handle, g_pcmPlay->buffer , *(g_pcmPlay->buffer + 10), g_pcmPlay->frames);


			// 写音频数据到PCM设备 
			while(ret = snd_pcm_writei(g_pcmPlay->handle, g_pcmPlay->buffer, g_pcmPlay->frames)<0)
				//              while(ret = snd_pcm_writei(ipcmPlay->handle, ipcmPlay->buffer, 32)<0)
			{
				usleep(2000); 
				if (ret == -EPIPE)
				{
					/* EPIPE means underrun */
					fprintf(stderr, "underrun occurred\n");
					//完成硬件参数设置，使设备准备好 
					snd_pcm_prepare(g_pcmPlay->handle);
				}
				else if (ret < 0)
				{
					fprintf(stderr,
							"error from writei: %s\n",
							snd_strerror(ret));
				}

			}
			usleep(100000);
		}
}
#if 0

void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len)
{
	int *val;
	int c = len / sizeof(int);
	int i = 0;
	char *p = buff + ( c -1 ) * sizeof(int);
	int k;
	int ret;

	char *pb = ipcmPlay->buffer;
	memset(pb,0,sizeof(ipcmPlay->size));


	for(i=c; i>0; i--){
		if(i < c){
			p = p - sizeof(int);
		}
		val = (int *)p;
		// do play
		for( k= 0; k < 32; k++){
			if( *val & 0x80000000){
				//memset(pb, 0x7f, 250);  /* 250 是插值比*/
				*pb = 0x7f;
			}else{
				//memset(pb, 0x0, 250);  /* 250 是插值比*/
				*pb = 0x0;
			}
			*val =(*val & 0x7fffffff) << 1;
			pb++;
		}
#if 1
		memset(ipcmPlay->buffer, 0xaa, ipcmPlay->size);
		printf("?%x:%x  %x : %d \n", ipcmPlay->handle, ipcmPlay->buffer , *(ipcmPlay->buffer + 10), ipcmPlay->frames);

		// 写音频数据到PCM设备 
		while(ret = snd_pcm_writei(ipcmPlay->handle, ipcmPlay->buffer, ipcmPlay->frames)<0)
//		while(ret = snd_pcm_writei(ipcmPlay->handle, ipcmPlay->buffer, 32)<0)
		{
			usleep(2000); 
			if (ret == -EPIPE)
			{
				/* EPIPE means underrun */
				fprintf(stderr, "underrun occurred\n");
				//完成硬件参数设置，使设备准备好 
				snd_pcm_prepare(ipcmPlay->handle);
			}
			else if (ret < 0)
			{
				fprintf(stderr,
						"error from writei: %s\n",
						snd_strerror(ret));
			}
		}
#endif
		pb = ipcmPlay->buffer;

	}

}
#endif
