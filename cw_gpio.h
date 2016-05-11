/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:cw_gpio.h
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/

#ifndef __CW_GPIO_H
#define __CW_GPIO_H

struct gpioOper {
	int fd;
	void *gpioBase;
	volatile unsigned int *GPBCON;
	volatile unsigned int *GPBDAT;
	volatile unsigned int *GPBUP;

};


struct gpioOper *init_gpio(void);

void free_gpio(struct gpioOper *gpio);

#endif
