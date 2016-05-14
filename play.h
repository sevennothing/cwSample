/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:play.h
 * Author: caijun.Li    Date:2016-05-12
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/

#ifndef __PLAY_H
#define __PLAY_H

#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef  USE_ALAS_DRIVER
#include <alsa/asoundlib.h>
#else
#include <linux/soundcard.h>
#endif

#define SEM_NAME  "insert_play"
#define SEM_KEY  6666

union semun {

	int val; /* value for SETVAL */

	struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */

	unsigned short *array; /* array for GETALL, SETALL */

	struct seminfo *__buf; /* buffer for IPC_INFO */

};


struct pcmConf
{

#ifdef  USE_ALAS_DRIVER
	snd_pcm_t* handle; //PCI设备句柄
	int datablock;
	snd_pcm_uframes_t frames;

#else
	int handle;

#endif
	int bit;	 // 采样位数
	int channels;
	int sampleFrequency;
	int size;
	char *buffer;

	int cwFrequency; /* CW 载频  40 ~ 3200Hz ; 该值不能大于sampleFrequency 的1/4*/
	int volume;       /* 载频幅度  0 ~ 100 */

	//sem_t *sem;    /* 插值与播放 互斥信号灯*/ 
	int semid;
	union semun semVal;

};

int init_pcm_play(struct pcmConf *pcm);
void free_pcm_play(struct pcmConf *pcm);

void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len);



//#define SAMPLERATE 8000
#define SAMPLERATE 44100

#define RISETIME 10
#define FALLTIME 10

#define PI 3.14159           
#define TWOPI 6.28


#endif
