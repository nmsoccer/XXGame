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


#define DIR_PATH_LEN	1024	/*创建目录路径长度*/
#define DIR_NAME_LEN	64		/*目录名最大长度*/
#define MAX_DIR_IN_PATH 16	 /*路径中允许的最多目录层数*/

//////////////////////////////FUNCTION DECLARE//////////////////////////
/*
 *
 */



/*
 * 出错记录
 */
//extern int log_error(char *str);
/*
 * 记录日志信息
 * 注意
 * @param0:LOG级别
 * @param1:必须是输入的格式化字符串
 * @param2~n:以格式化字符串为基础的其他参数
 *
 * @return:0:成功；-1：失败
 */
extern int write_log(u32 type , ...);

/*
 * 获得当前时间的字符串
 */
extern char *str_current_time(void);


/*
 * 在权限许可的范围内创建目录。如果路径中有不存在的目录则创建之
 * 最多嵌套MAX_DIR_IN_PATH层
 * 路径长度不得大于DIR_PATH_LEN字节
 * 目录名长度小于DIR_NAME_LEN字节
 * @param path:目录路径
 * @param mod:目录权限
 * @return:成功返回0； 无权限返回-2 ， 其他情况返回-1
 */
extern int create_dir(const char *path , int mod);

/*
 * 字符串划分
 * 将字符串src_str根据c划分成token装入dest_arrays
 * @param src_str:源字符串
 * @param c:划分依据
 * @param sub_arrays:划分后的字符串数组
 * @param sub_count:最多容纳的子串数目
 * @param sub_len:子串最大长度
 *
 * @return:成功返回子串数目；失败返回-1
 */
int strsplit(const char *src_str , int c , char *sub_arrays , int sub_count , int sub_len);


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
extern int set_spin_lock(spin_lock_t *lock);

/*
 * 丢弃自己使用的自旋锁即为其解锁
 */
extern int drop_spin_lock(spin_lock_t *lock);

/*
 * 加上读锁
 * 如果写锁已经加上则加读锁失败
 * 如果读锁已经加上可以加读锁
 *
 * @param plock:读写锁指针
 * @param seconds: 加锁失败的处理情况：>0 强制加锁持续秒数; 0 直接返回；<0 尝试持续加锁直到上锁为止
 * @return:
 * S_LOCKED:加锁成功
 * E_LOCKED:已经有写锁
 * E_ERROR:错误
 */
extern int set_read_lock(rdwr_lock_t *plock , int seconds);

/*
 *解读锁
 *如果读锁已经上锁，则减少读锁数目；如果读锁数目为0则将锁设置为完全解开；
 *如果写锁上锁则失败（这种情况一般不会出现。出现则是因为逻辑错误）
 *@return:
 *S_UNLOCK:解锁成功
 * E_LOCKED:已经有写锁
 * E_ERROR:错误
 */
extern int drop_read_lock(rdwr_lock_t *plock);

/*
 * 加上写锁
 * 如果写锁已经加上则加写锁失败
 * 如果读锁已经加上则加写锁失败
 *
 * @param plock:读写锁指针
 * @param seconds: 加锁失败的处理情况：>0 强制加锁持续秒数; 0 直接返回；<0 尝试持续加锁直到上锁为止
 * @return:
 * S_LOCKED:加锁成功
 * E_LOCKED:已经有锁
 * E_ERROR:错误
 */
extern int set_write_lock(rdwr_lock_t *plock , int seconds);

/*
 *解写锁
 *如果写锁已经上锁，则将锁设置为完全解开；
 *如果读锁上锁则失败（这种情况一般不会出现。出现则是因为逻辑错误）
 *@return:
 *S_UNLOCK:解锁成功
 * E_LOCKED:已经有写锁
 * E_ERROR:错误
 */
extern int drop_write_lock(rdwr_lock_t *plock);

/*=======================INLINE FUNCTIONS=========================================*/
/*
 * 获得最右0的个数也就是最右方bit1的偏移
 * 比如011000 返回3
 */
inline int index_last_1bit(int _il1b_number);

#endif /* TOOL_H_ */
