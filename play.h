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

#include <alsa/asoundlib.h>

//#define SAMPLERATE 8000
#define SAMPLERATE 44100

#define RISETIME 10
#define FALLTIME 10
//
///* tbase (dit time in samples) is 1200 msec * 8 samples per msec */
////#define tbase 9600 // for 8000 Samples per second
//
///* 900 Hz */
#define PITCH 800
/* volume on 0..100 percent */
#define VOLUME 70
//
///* rates (character, word) for various speeds (slow|med|fast|extra) */
//#define CSLOW 15
//#define WSLOW 5
//#define CMED 18
//#define WMED 12
//#define CFAST 18
//#define WFAST 18
//#define CEXTRA 20
//#define WEXTRA 20





struct pcmConf
{
	snd_pcm_t* handle; //PCI设备句柄
	int bit;	 // 采样位数
	int channels;
	int frequency;
	int size;
	int datablock;
	char *buffer;
	snd_pcm_uframes_t frames;

};

int init_pcm_play(struct pcmConf *pcm);
void free_pcm_play(struct pcmConf *pcm);

void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len);

//void setupVoice(int hz, int amp);

#endif
