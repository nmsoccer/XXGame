/*
 * tool.h
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#ifndef TOOL_H_
#define TOOL_H_


#include "common.h"

///////////////////////////////MACRO DEFINE///////////////////////////////
#define SPIN_LOCKED	1	/*自旋锁上锁*/
#define SPIN_UN_LOCKED	0	/*解锁*/

//////////////////////////////FUNCTION DECLARE//////////////////////////
/*
 * 出错记录
 */
extern int log_error(char *str);

/*
 * 将文件设置非阻塞
 */
extern int set_nonblock(int fd);

/*
 * 修改socket缓冲区大小
 * @sock_fd: 套接字描述符
 * @send_size: 发送缓冲区大小
 * @recv_size: 接收缓冲区大小
 */
extern int set_sock_buff_size(int sock_fd , int send_size , int recv_size);


/*
 * 自旋锁上锁
 *	如果锁变量没有上锁则由调用者为其上锁；
 *	如果锁变量已经上锁那么自旋直到解锁为止
 */
extern int get_spin_lock(spin_lock_t *lock);

/*
 * 丢弃自己使用的自旋锁即为其解锁
 */
extern int drop_spin_lock(spin_lock_t *lock);

#endif /* TOOL_H_ */
