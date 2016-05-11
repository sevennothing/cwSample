/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:process.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:
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

#include "process.h"
#include "cw_gpio.h"


#define  TRIG_FREQ   (64)  /* 触发频率设置为64Hz*/


int init_timer(void)
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

	/*Set the freq as 64Hz*/  
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

int free_timer(int fd)
{
	/* Disable periodic interrupts */  
	ioctl(fd, RTC_PIE_OFF, 0);  
	close(fd);  

	return 0;  
}


void get_signal(struct gpioOper *gpio)
{
	int val;

	val = get_gpio_value(gpio);
	printf("get io : %d\n", val);


}

void test_process(int fd, struct gpioOper *gpio)
{
	int ret = 0;
	int i = 0;
	struct timespec ts;
	struct timespec last_ts;
	int runtime = 0;

	clock_gettime(CLOCK_MONOTONIC,&last_ts);
	//lastTime = ts.tv_sec * 1000 + ts.tv_nsec/1000000;
	set_timer(fd, 64);
//	set_timer(fd, 128);

//	for( i = 0; i < 100; i++){
	while(1) {
		ret = wait_for_timer(fd);

		get_signal(gpio);
		if(ret < 0){

		}
		clock_gettime(CLOCK_MONOTONIC,&ts);
		runtime = (ts.tv_sec - last_ts.tv_sec) * 1000 + (ts.tv_nsec - last_ts.tv_nsec)/1000000;
		last_ts = ts;


		printf("%d\n",runtime );
	}

}
