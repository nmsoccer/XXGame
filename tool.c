/*
 * tool.c
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include "tool.h"
/*
 * 出错记录
 */
int log_error(char *str){
	printf("%s\n" , str);
	return 0;
}

/*
 * 打印信息
 */



/*
 * 将文件设置非阻塞
 */
int set_nonblock(int fd){
	int flag;

	flag = fcntl(fd ,  F_GETFL , 0);
	flag |= O_NONBLOCK;

	return fcntl(fd , F_SETFL , flag);
}

/*
 * 修改socket缓冲区大小
 * @sock_fd: 套接字描述符
 * @send_size: 发送缓冲区大小
 * @recv_size: 接收缓冲区大小
 */
int set_sock_buff_size(int sock_fd , int send_size , int recv_size){
	int s_size;
	int r_size;
	int opt_len;
	int iRet;

	opt_len = sizeof(int);

	/*设置缓冲区大小*/
	iRet = setsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &send_size , opt_len);
	if(iRet < 0){
		log_error("set sock send buff failed!");
		return -1;
	}
	iRet = setsockopt(sock_fd , SOL_SOCKET , SO_RCVBUF , &recv_size , opt_len);
	if(iRet < 0){
		log_error("set sock recv buff failed!");
		return -1;
	}

	/*打印设置之后的缓冲区长度*/
	iRet = getsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &s_size , &opt_len);
	if(iRet < 0){
		log_error("get sock send buff failed!");
		return -1;
	}
	iRet = getsockopt(sock_fd , SOL_SOCKET , SO_RCVBUF , &r_size , &opt_len);
	if(iRet < 0){
		log_error("get sock recv buff failed!");
		return -1;
	}
	printf("new send buff size: %dK; recv buff size: %dK\n" , s_size/1024 , r_size/1024);

	return 0;
}


/*
 * 自旋锁上锁
 *	如果锁变量没有上锁则由调用者为其上锁；
 *	如果锁变量已经上锁那么自旋直到解锁为止
 */
int get_spin_lock(spin_lock_t *lock){
	while(*lock !=  SPIN_UN_LOCKED);

	*lock = SPIN_LOCKED;
	return 0;
}

/*
 * 丢弃自己使用的自旋锁即为其解锁
 */
int drop_spin_lock(spin_lock_t *lock){
	*lock = SPIN_UN_LOCKED;
	return 0;
}
