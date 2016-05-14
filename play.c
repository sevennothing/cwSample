/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:play.c
 * Author: caijun.Li    Date:2016-05-12
 * Description:
 *   可以使用2种传输算法，一种是补足整数字节进行传输，另一种是额外传输bit位长度。
 *   见 PADDING_INVALID_CODE ; 默认采用传输Bit 位长度的方法。
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "play.h"
#include "process.h"

#include <pthread.h>


/***对信号量数组semnum编号的信号量做P操作***/
int P(int semid, int semnum)
{

	struct sembuf sops={semnum,-1, SEM_UNDO};

	return (semop(semid,&sops,1));

}

/***对信号量数组semnum编号的信号量做V操作***/
int V(int semid, int semnum)
{

	struct sembuf sops={semnum,+1, SEM_UNDO};

	return (semop(semid,&sops,1));

}


void inline insert_point(struct pcmConf *ipcmPlay, char mode, char *p, int c, int cValid, int cInvalid)
{
	float tv,tv2;
	int baseV = 0x80;
	int pos = 0;
	int riseSample = c / 100;
	int fallSample = c / 100;
	float gain = 1;
	float freq = (float)ipcmPlay->cwFrequency / ipcmPlay->sampleFrequency;

	if(((cValid == 1) && (mode == VALID_LEVEL)) || ((cInvalid == 1) && (mode == INVALID_LEVEL))){
		for(pos = 0; pos < riseSample; pos++){
			if(mode == VALID_LEVEL){
				/* 50% 上升沿 */
				gain = 0.5 + (-0.5 * cos((PI * pos)/riseSample));
			}else{
				/* 50% 下降沿 */
				gain = 0.5 + (0.5 * cos((PI * pos)/fallSample));
			}
			tv = gain * sin((float)pos * TWOPI * freq);
			tv2 = (ipcmPlay->volume * tv) + baseV;
			*(p+pos) = (unsigned char)tv2;

			// 90% 保持  */
			for(pos = riseSample; pos < c; pos++){
				if(mode == VALID_LEVEL){
					tv = 1.0 * sin((float)pos * TWOPI * freq);
					tv2 = (ipcmPlay->volume * tv) + baseV;
				}else {
					tv2 = baseV;
				}
				*(p+pos) = (unsigned char)tv2;
			}
		}
	}else{
		int wavePos = 0; 
		wavePos = (cValid - 1) * c;
		for(pos = 0; pos < c; pos++){
			if(mode == VALID_LEVEL){
				tv = 1.0 * sin((float)wavePos * TWOPI * freq);
				tv2 = (ipcmPlay->volume * tv) + baseV;
				wavePos++;
			}else {
				tv2 = baseV;
			}
			*(p+pos) = (unsigned char)tv2;
		}
	}
	
}


//

struct threadParam {
	struct pcmConf *pcm;
	int len;
	char *pbuf;
	char last;  /*最后一帧标记*/
};

static void playCW(void *param)
{
	struct threadParam *tparam = (struct threadParam *)param;
	struct pcmConf *ipcmPlay = tparam->pcm;
	int len = tparam->len;
	char *pbuf = tparam->pbuf;
	int ret;

	pthread_detach(pthread_self());
#ifdef USE_ALAS_DRIVER
	/* alsa 必须以帧为单位进行播放 */
	while(ret = snd_pcm_writei(ipcmPlay->handle, pbuf, len) < 0)
	{
	//	printf("play %d\n", len);
		if (ret == -EPIPE)
		{
			/* EPIPE means underrun */
			fprintf(stderr, "underrun occurred(%d)\n",len);
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

#else
	ret = write(ipcmPlay->handle, pbuf, len);
	if(ret==-1){
		perror("Fail to play the sound!");
	}
	/* 下面的代码用于在更改播放文件的参数时，播放掉缓冲区内的内容 */
	//ret = ioctl(ipcmPlay->handle, SOUND_PCM_SYNC, 0);
	//if (ret == -1)
	//	perror("SOUND_PCM_SYNC ioctl failed");
#endif

	free(pbuf);
	free(tparam);
	ret = V(ipcmPlay->semid, 0);
	pthread_exit(0);

}

/* 播放较慢且阻塞，插值耗时，将二者分离*/
void fork_play(struct pcmConf *ipcmPlay, int len, char last)
{
	pthread_t tid = 0;
	pthread_attr_t attr;
	int ret;
	static k = 0;

	struct threadParam *tparam = malloc(sizeof(struct threadParam));
	tparam->pcm = ipcmPlay;
	tparam->len = len;
	tparam->pbuf = malloc(len);
	tparam->last = last;
	memcpy(tparam->pbuf, ipcmPlay->buffer, len);

	ret = P(ipcmPlay->semid, 0);

	ret = pthread_create(&tid, NULL, (void *)playCW, tparam);
	if(ret != 0){
		perror("create palyCW thread error");
	}

	return;
}


void pcmPlay(struct pcmConf *ipcmPlay, char buff[], int len)
{
	int i = 0;
	int k;
	int ret;
	int cValid = 0;
	int cInvalid = 0;
	int cbit = 0;
	int bi = ipcmPlay->frames;

	char *pb = ipcmPlay->buffer;
	memset(pb,0,sizeof(ipcmPlay->size));

	/* 人为插入一个补足点，用于平滑初始音频 */
	cInvalid++;
	insert_point(ipcmPlay,INVALID_LEVEL, pb, bi,cValid, cInvalid);
	printf("*");
	pb+=bi;
	cbit +=bi;

#ifdef  PADDING_INVALID_CODE
	for(i = len-1; i >= 0; i--){
		for(k=7; k>=0; k--){
			if( (cbit + bi) > ipcmPlay->size){
				/* 防止数组越界，需要先清空数据 */
				fork_play(ipcmPlay, cbit, 0);
				pb = ipcmPlay->buffer;
				cbit = 0;
			}

			if((buff[i] & (1<<k)) == 0){
				/* 有效电平 */
				cValid++;
				cInvalid = 0;
				insert_point(ipcmPlay,VALID_LEVEL, pb, bi, cValid, cInvalid);
				printf(".");
			}else{
				/* 无效电平或间隔 */
				cValid = 0;
				cInvalid++;
				insert_point(ipcmPlay,INVALID_LEVEL, pb, bi,cValid, cInvalid);
				printf("*");
			}

			cbit += bi;
			pb += bi;
		}
	}

	printf("\n");
	if(cbit != 0){
		fork_play(ipcmPlay, cbit, 1); // the last player
	}

#else
	unsigned short bitLen = *(unsigned short *)buff;
	char remainderBit = bitLen % 8;

	for(i = len-1; i >=2; i--){
		if(remainderBit > 0){
			k = remainderBit - 1;
			remainderBit = 0;
		}else{
			k = 7;
		}
		for(; k>=0; k--){
			if( (cbit + bi) > ipcmPlay->size){
				/* 防止数组越界，需要先清空数据 */
				fork_play(ipcmPlay, cbit,0);
				pb = ipcmPlay->buffer;
				cbit = 0;
			}

			if((buff[i] & (1<<k)) == 0){
				/* 有效电平 */
				cValid++;
				cInvalid = 0;
				insert_point(ipcmPlay,VALID_LEVEL, pb, bi, cValid, cInvalid);
				printf(".");
			}else{
				/* 无效电平或间隔 */
				cValid = 0;
				cInvalid++;
				insert_point(ipcmPlay,INVALID_LEVEL, pb, bi,cValid, cInvalid);
				printf("*");
			}

			cbit += bi;
			pb += bi;
		}
	}

	printf("\n");
	if(cbit != 0){
		fork_play(ipcmPlay, cbit, 1);
	}

#endif

}

#ifdef  USE_ALAS_DRIVER
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
	val = pcm->sampleFrequency;
	rc=snd_pcm_hw_params_set_rate_near(pcm->handle, params, &val, &dir); //设置>频率
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_set_rate_near:");
		exit(1);
	}

	/* set period*/
	val = pcm->frames;
	rc=snd_pcm_hw_params_set_period_size_near(pcm->handle, params, &val, &dir);
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_set_period_size:");
		exit(1);
	}

	rc = snd_pcm_hw_params(pcm->handle, params);
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params: ");
		exit(1);
	}

	rc=snd_pcm_hw_params_get_period_size(params, &(pcm->frames), &dir); /*获取周期长度*/
	if(rc<0)
	{
		perror("\nsnd_pcm_hw_params_get_period_size:");
		exit(1);
	}
	
	pcm->size = pcm->frames * (pcm->datablock); /*4 代表数据快长度*/

	pcm->buffer =(char*)malloc(pcm->size);

	//创建一个命名信号量
	pcm->semid=semget(SEM_KEY,1,IPC_CREAT | 0666);//创建了一个权限为666的信号量
	pcm->semVal.val = 1;

	/***对0号信号量设置初始值***/
	ret =semctl(pcm->semid,0,SETVAL,pcm->semVal);

	if (ret < 0 ){
		perror("ctl sem error");
		semctl(pcm->semid, 0, IPC_RMID,pcm->semVal);
		return -1 ;
	}


	return 0;
}

void free_pcm_play(struct pcmConf *pcm)
{

	snd_pcm_drain(pcm->handle);
	snd_pcm_close(pcm->handle);
	free(pcm->buffer);

	semctl(pcm->semid, 0, IPC_RMID,pcm->semVal);

}


#else

int init_pcm_play(struct pcmConf *pcm)
{

	int ret;
	
	//打开声卡设备，并设置声卡播放参数，这些参数必须与声音文件参数一致
	//pcm->handle=open("/dev/audio", O_WRONLY);
	pcm->handle=open("/dev/dsp", O_WRONLY);

	if(pcm->handle == -1){
		perror("open Audio_Device fail");
		return -1;
	}

	ret=ioctl(pcm->handle,SOUND_PCM_WRITE_CHANNELS,&pcm->channels);
	if(ret==-1){
		perror("error from SOUND_PCM_WRITE_CHANNELS ioctl");
		return -1;
	}

	ret=ioctl(pcm->handle,SOUND_PCM_WRITE_BITS,&pcm->bit);
	if(ret==-1){
		perror("error from SOUND_PCM_WRITE_BITS ioctl");
		return -1;
	}

	ret=ioctl(pcm->handle,SOUND_PCM_WRITE_RATE,&pcm->sampleFrequency);
	if(ret==-1){
		perror("error from SOUND_PCM_WRITE_RATE ioctl");
		return -1;
	}
	pcm->size = pcm->frames * (pcm->datablock); /*4 代表数据快长度*/

	pcm->buffer =(char*)malloc(pcm->size);

	//创建一个命名信号量
	pcm->semid=semget(SEM_KEY,1,IPC_CREAT | 0666);//创建了一个权限为666的信号量
	pcm->semVal.val = 1;

	/***对0号信号量设置初始值***/
	ret =semctl(pcm->semid,0,SETVAL,pcm->semVal);

	if (ret < 0 ){
		perror("ctl sem error");
		semctl(pcm->semid, 0, IPC_RMID,pcm->semVal);
		return -1 ;
	}

	return 0;
}

void free_pcm_play(struct pcmConf *pcm)
{
	close(pcm->handle);

	free(pcm->buffer);
	semctl(pcm->semid, 0, IPC_RMID,pcm->semVal);
}


#endif
