/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:gpio.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <sys/mman.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/mman.h> 
#include <fcntl.h> 
//#include <asm/page.h>

#include "cw_gpio.h"



#define GPIO_CTL_BASE 0x56000000
#define rGPBCON 0x10 
#define rGPBDAT 0x14 
#define rGPBUP 0x18

#define MEM_LEN 0xbc

struct gpioOper *init_gpio(void){

	char dev_name[] = "/dev/mem";   

	struct gpioOper *gpio = calloc(1,sizeof(struct gpioOper));

	gpio->fd = open(dev_name, O_RDWR);   

	if(gpio->fd < 0){   
		printf("open %s is error\n",dev_name);   
		free(gpio);
		return NULL ;   
	}   

	gpio->gpioBase = mmap( 0, MEM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED,gpio->fd, GPIO_CTL_BASE );  

	if(gpio->gpioBase == NULL){   

		printf("gpio base mmap is error\n");   

		close(gpio->fd);   
		free(gpio);

		return NULL;   

	}
	gpio->GPBCON = (volatile unsigned int *) (gpio->gpioBase + rGPBCON); 
	gpio->GPBDAT = (volatile unsigned int *) (gpio->gpioBase + rGPBDAT); 
	gpio->GPBUP = (volatile unsigned int *) (gpio->gpioBase + rGPBUP);

	printf("calloc gpio successed!\n");
	return gpio;

}


void free_gpio(struct gpioOper *gpio)
{
	if(gpio == NULL){
		printf("invalid param\n");
		return;
	}

	munmap(0, MEM_LEN);

	close(gpio->fd);

	free(gpio);

	printf("free gpio successed!\n");
}
