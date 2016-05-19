/*******************************************************************************
 * copyright ,2015
 * FileName:trans.c
 * Author: caijun.Li    Date:2016-05-11
 * Description:  use UDP to transmit package
 * History:
 *     <author>        <time>        <desc>
 *
 ******************************************************************************/
#include   <stdio.h>


#ifdef SELF_TEST_FOR_PIPE
extern int pipe_fd[2];
#endif

static char rbuff[500]; /* 仅用于测试 */
static char rlen;
int create_connect()
{


	return 0;
}


int close_connet()
{
	return 0;
}



int transmit_packet( char buff[], int len)
{
	
	int i = 0;
#if 0
	printf("transmit stream info(%d):", len);
	for(i=0; i<len; i++){
		printf(" %02x", (unsigned char)buff[i]);
	}
	printf("\n");
	fflush(stdout);
#endif

	// 注意字节序问题
	/* todo: 通过UDP 传输数据*/



#ifdef SELF_TEST_FOR_PIPE
	/* 下面做简单测试*/
	write(pipe_fd[1], buff, len); 
#endif
		
	return 0;
}


int recv_packet( char *buff, int len)
{
	int ret = 0;

#ifdef SELF_TEST_FOR_PIPE
	/* 下面做简单测试*/
	ret = read(pipe_fd[0], buff, len); 
#endif

	return ret;

}


