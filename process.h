/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:process.h
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/

#ifdef __PROCESS_H
#define __PROCESS_H

#include "cw_gpio.h"

int init_timer(void);

int free_timer(int fd);

void get_signal(struct gpioOper *gpio);

void test_process(int fd, struct gpioOper *gpio);


#endif
