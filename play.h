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

#ifndef  SOUND_CARD
#include <alsa/asoundlib.h>
struct pcmConf
{
	snd_pcm_t* handle; //PCI设备句柄
	int bit;	 // 采样位数
	int channels;
	int sampleFrequency;
	int size;
	int datablock;
	char *buffer;
	snd_pcm_uframes_t frames;

};

int init_pcm_play(struct pcmConf *pcm);
void free_pcm_play(struct pcmConf *pcm);

void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len);

#else
#include <linux/soundcard.h>
struct pcmConf
{
	int handle;
	int bit;
	int sampleFrequency; /* 采样频率  */
	int size;
	char *buffer;

	int cwFrequency; /* CW 载频  40 ~ 3200Hz ; 该值不能大于sampleFrequency 的1/4*/
	int volume;       /* 载频幅度  0 ~ 100 */


};
int init_pcm_play(struct pcmConf *pcm);
void free_pcm_play(struct pcmConf *pcm);

void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len);
#endif


//#define SAMPLERATE 8000
#define SAMPLERATE 44100

#define RISETIME 10
#define FALLTIME 10

#define PI 3.14159           
#define TWOPI 6.28

void setupVoice(int hz, int amp);

#endif
