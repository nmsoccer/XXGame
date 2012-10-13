/*
 * tool.c
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * 出错记录
 */
int log_error(char *str){
	printf("%s\n" , str);
	return 0;
}


/*
 * 将文件设置非阻塞
 */
int set_nonblock(int fd){
	int flag;

	flag = fcntl(fd ,  F_GETFL , 0);
	flag |= O_NONBLOCK;

	return fcntl(fd , F_SETFL , flag);
}
