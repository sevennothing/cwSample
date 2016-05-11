/*******************************************************************************
 * copyright ,2015,LongRaise Tech. Co., Ltd.
 * FileName:main.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include <stdio.h>
#include "cw_gpio.h"
#include "process.h"



int main(int argc, char **argv)
{

	struct gpioOper *cwGpio;
	int timerFd = 0;
	//TODO: param process
	
	cwGpio = init_gpio();


	timerFd = init_timer();
	test_process(timerFd, cwGpio);
	free_timer(timerFd);








	/**  free system */

	free_gpio(cwGpio);


	return 0;

}


