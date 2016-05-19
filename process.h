/*******************************************************************************
 * copyright ,2015
 * FileName:process.h
 * Author: caijun.Li    Date:2016-05-11
 * Description: 
 *    a). 采样率设为32HZ时，即每秒采样32个点。刚好为一个int 型数据。
 *    b). STREAM_MAX_SECONDS, 一次流信息应当包含一个字，该值的大小将决定最低的PWM。
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/

#ifndef __PROCESS_H
#define __PROCESS_H

#include "play.h"

enum tmode {
		RTC_MODE,
		OTHER_MODE
};
#define  TRIG_FREQ   (32)  /* 触发频率设置为32Hz*/
//#define  TRIG_FREQ   (16)  /* 触发频率设置为16Hz*/

#define  INVALID_LEVEL  (1)   /* 无效电平值 */
#define  VALID_LEVEL  (0)     /* 有效电平值 */
#define  DIT_LEVL  (0)       /* DIT电平值 */
#define  DAH_LEVL  (0)       /* DAH电平值 */

#define  MORSE_CHAR_MAX    (1024)
#define  STREAM_BIT_MAX   (4000)
//#define  STREAM_MAX_SECONDS (60)     /* 1min 数据*/
#define  STREAM_MAX_SECONDS (10)     /* 10S 数据, 该值将确定最低的PWM (6PWM)*/

struct signalProcess {
	enum tmode timer_mode;
	int test_enable;
	int decode_enable;
	int verbose;   /* 输出详细信息 */

	int fd;     /* timer handle */
	int minCZ;  /* 最少连0个数 */
	int minCO;  /* 最少连1个数 */
	int curCinva;  /* 当前连1个数 */

	char morse_uff[MORSE_CHAR_MAX];
	int level_stream[STREAM_MAX_SECONDS];  /* 一个int 数据存储1s的数据 */
	
	unsigned int sampleFreq_Hz;     /* 采样速率设置，必须为2的幂, 默认值为 64HZ*/

	int (*get_signal_cb)(void);     /* 获取电信号接口 */

	int dithering_pass_enable;      /* 使能软件消抖动 */
	int (*soft_dithering_pass)(struct signalProcess *sp, char *flag);  /* 软件消抖动 */

	int transmit_enable;
	int transmit_pos;			/* 传输位置标记 */
	int (*transmit_cb)(char buff[], int len);

	void (*decode_process)();	 /* 电信号解码为 morse */
	void (*recovery_process)();  /* morse 恢复为电信号 */

	void (*setup_voice)(int hz, int amp);


};

struct signalProcess *require_signal_process(void);
int modify_signal_process(struct signalProcess *sp);
int destory_signal_process(struct signalProcess *sp);
int run_process(struct signalProcess *sp);


#define STOP_STATUS   (-1)

#define  NEED_TREIG_START(level,sp,start) if( VALID_LEVEL == level){ \
	if(sp->dithering_pass_enable && (sp->soft_dithering_pass != NULL)){\
		char flag = 0;\
		sp->soft_dithering_pass(sp, &flag); \
		if(!flag && (start > 0)){\
			level = INVALID_LEVEL;\
			start++;\
		}else if(!flag && (start == STOP_STATUS)){\
			level = INVALID_LEVEL;\
		}else if(flag && (start > STOP_STATUS)) start++;\
		else if(flag && (start == STOP_STATUS)) start = 1;\
	}else if(start == STOP_STATUS) {start = 1;} \
	else { start++; }\
}else if(start > 0){\
	start++;}
	

#endif
