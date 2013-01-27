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
#include <stdarg.h>
#include "tool.h"

static FILE *log_file = NULL;

/*
 * 出错记录
 */
//int log_error(char *str){
//	printf("%s\n" , str);
//	return 0;
//}

/*
 * 记录日志信息
 * 日志记录格式：
 * 时间 @ 日志类型 @ 日志消息
 * 注意
 * @param0:LOG级别
 * @param1:必须是输入的格式化字符串
 * @param2~n:以格式化字符串为基础的其他参数
 *
 * @return:0:成功；-1：失败
 */
int write_log(u32 type , ...){
	va_list arg_list;
	char *fmt;
	char buff[LOG_RECORD_SIZE] = {'\0'};
	char file_path[MAX_DIR_IN_PATH * DIR_PATH_LEN] = {'\0'};
	int index = 0;
	time_t tp;
	struct tm *tm;
//	FILE *log_file;
	int ret;

	/*记录时间*/
	STR_CURRENT_TIME(buff);
	index = strlen(buff);

	/*整合日志信息*/
	va_start(arg_list , type);	/*使arg_list指向type之后的第一个参数地址*/
	fmt = va_arg(arg_list , char *);	/*fmt为type之后的参数：格式化字符串*/
	if(!fmt){//	int world_id;	/*世界ID*/
		//	int line_id;		/*线ID*/
		return -1;
	}

	switch(type){
	case LOG_INFO:
		strncat(&buff[index] , "@LOG INFO@" , sizeof(buff) - index);	/*日志类型*/
		index = strlen(buff);
		vsnprintf(&buff[index] , sizeof(buff) - index , fmt , arg_list);	/*log信息*/

#ifdef DEBUG
		printf("tty:%s\n" , buff);
#endif
		break;
	case LOG_NOTIFY:
		strncat(&buff[index] , "@LOG NOTIFY@" , sizeof(buff) - index);	/*日志类型*/
		index = strlen(buff);
		vsnprintf(&buff[index] , sizeof(buff) - index , fmt , arg_list);	/*log信息*/

		printf("tty:%s\n" , buff);
		break;
	case LOG_ERR:
		strncat(&buff[index] , "@LOG ERR@" , sizeof(buff) - index);	/*日志类型*/
		index = strlen(buff);
		vsnprintf(&buff[index] , sizeof(buff) - index , fmt , arg_list);	/*log信息*/

		printf("tty:%s\n" , buff);
		break;
	case LOG_DUMP:
		strncat(&buff[index] , "@LOG DUMP@" , sizeof(buff) - index);	/*日志类型*/
		index = strlen(buff);
		vsnprintf(&buff[index] , sizeof(buff) - index , fmt , arg_list);	/*log信息*/
		printf("tty:!!:%s\n" , buff);
		exit(-1);
	default:
		return -1;
	}

	va_end(arg_list);
	/*写入日志文件。位于当前文件夹的LOG/年/月/日.log  日志存储文件目录表已经由log_server创建好*/
//	printf("log file:%x\n" , log_file);
	if(log_file == NULL){
		tp = time(0);
		tm = localtime(&tp);
		snprintf(file_path , sizeof(file_path) , "./LOG/%d/%d/%d.log" , tm->tm_year + 1900 , tm->tm_mon + 1 , tm->tm_mday);

		log_file = fopen(file_path , "a+");
		if(log_file == NULL){
			printf("!!write log error!! open %s failed\n" , file_path);
			return -1;
		}
//		printf("create log_file!\n");
	}

	buff[strlen(buff)] = '\n';
	fputs(buff , log_file);
//	fclose(log_file);	/*等进程关闭时再关闭文件句柄*/
	return 0;
}


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
int strsplit(const char *src_str , int c , char *sub_arrays , int sub_count , int sub_len){
	int count;
	char *start;
	char *end;

	if(!src_str || !sub_arrays || sub_count==0 || sub_len==0){
		return -1;
	}

	/*begin*/
	memset(sub_arrays , 0 , sub_count * sub_len);
	start = src_str;
	count = 0;
	while(1){
		if(count >= sub_count){	/*已经超过了最多容纳子串数目*/
			break;
		}

		end = strchr(start , c);
		if(end == NULL){	/*没有找到分隔符*/
			if(start[0] != 0){	/*初始不是0说明是一个不包含分隔符的有效字符串token，仍然放入*/
				if(strlen(start) <= sub_len - 1){
					memcpy(sub_arrays + count*sub_len , start , strlen(start));
				}else{
					memcpy(sub_arrays + count*sub_len , start , sub_len-1);
				}
				count++;
			}
			break;
		}

		/*start->end是一个token，目录名*/
		if(end - start <= sub_len - 1){
			memcpy(sub_arrays + count * sub_len , start , end - start);
		}else{
			memcpy(sub_arrays + count * sub_len , start , sub_len - 1);
		}
//		printf("in split :%s\n" , sub_arrays + count * sub_len);

		/*准备下一次*/
		start = end + 1;
		count++;
	}

	return count;
}






/*
 * 在权限许可的范围内创建目录。如果路径中有不存在的目录则创建之
 * 最多嵌套MAX_DIR_IN_PATH层
 * 路径长度不得大于DIR_PATH_LEN字节
 * 目录名长度小于DIR_NAME_LEN字节
 * @param path:目录路径
 * @param mod:目录权限
 * @return:成功返回0； 无权限返回-2 ， 其他情况返回-1
 */
int create_dir(const char *path , int mod){
	char dir_names[MAX_DIR_IN_PATH][DIR_NAME_LEN];
	char path_tmp[MAX_DIR_IN_PATH * DIR_NAME_LEN] = {'\0'};
	int i;
	int nr_splits;
	int absolute_path = 0;	/*路径类型 0:相对路径；1：绝对路径*/
	int iret;

	/*init*/
	if(!path){
		return -1;
	}

	/*确定路径类型*/
	if(path[0] == '/'){	/* 以/开头的为绝对路径*/
		absolute_path = 1;
		printf("it is abs path!\n");
	}


	/*split*/
	nr_splits = strsplit(path , '/' , dir_names , MAX_DIR_IN_PATH , DIR_NAME_LEN);
	if(nr_splits == -1){
		return -1;
	}

	/*创建目录*/
	for(i=0; i<nr_splits; i++){
		if(absolute_path && i==0){	/*绝对路径的第一个为根目录*/
			path_tmp[0] = '/';
			continue;
		}

		/*获得创建目录的当前路径*/
		strncat(path_tmp , dir_names[i] , sizeof(path_tmp) - strlen(path_tmp));
		path_tmp[strlen(path_tmp)] = '/';
//		printf("tmp path %d: %s\n" , i , path_tmp);

		/*创建目录*/
		iret = mkdir(path_tmp , mod);
		if(iret == -1){
			switch(errno){
			case EEXIST:/*如果是因为目录已经存在而创建失败则继续创建*/
				break;
			case EACCES:/*如果是因为权限问题则返回-2*/
				printf("access denied!\n");
				return -2;
			default:	/*其他问题返回-1*/
				return -1;
			}

		}

	}

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
		write_log(LOG_ERR , "set sock send buff failed!");
		return -1;
	}
	iRet = setsockopt(sock_fd , SOL_SOCKET , SO_RCVBUF , &recv_size , opt_len);
	if(iRet < 0){
		write_log(LOG_ERR , "set sock recv buff failed!");
		return -1;
	}

	/*打印设置之后的缓冲区长度*/
	iRet = getsockopt(sock_fd , SOL_SOCKET , SO_SNDBUF , &s_size , &opt_len);
	if(iRet < 0){
		write_log(LOG_ERR , "get sock send buff failed!");
		return -1;
	}
	iRet = getsockopt(sock_fd , SOL_SOCKET , SO_RCVBUF , &r_size , &opt_len);
	if(iRet < 0){
		write_log(LOG_ERR , "get sock recv buff failed!");
		return -1;
	}

#ifdef DEBUG
//	printf("new send buff size: %dK; recv buff size: %dK\n" , s_size/1024 , r_size/1024);
#endif

	return 0;
}


/*
 * 自旋锁上锁
 *	如果锁变量没有上锁则由调用者为其上锁；
 *	如果锁变量已经上锁那么自旋直到解锁为止
 */
int set_spin_lock(spin_lock_t *lock){
	while(*lock !=  SPIN_UN_LOCK);

	*lock = SPIN_LOCK;
	return 0;
}

/*
 * 丢弃自己使用的自旋锁即为其解锁
 */
int drop_spin_lock(spin_lock_t *lock){
	*lock = SPIN_UN_LOCK;
	return 0;
}

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
int set_read_lock(rdwr_lock_t *plock , int seconds){
	int circles = 0;

	/*check*/
	if(!plock){
		return E_LOCK;
	}

	/*handle*/
	/*如果写锁未上，则直接加读锁*/
	if(plock->write_lock == WRITE_UNLOCK){
		plock->rd_lock_count++;	/*加读锁数量*/
		plock->read_lock = READ_LOCK;
		return S_LOCKED;
	}

	/*写锁已经加上，则等待解锁*/
	if(seconds == 0){	/*直接返回*/
		return E_WRLOCKED;
	}

	if(seconds > 0){	/*等待seconds*/
		while(1){
			if(circles >= seconds){
				return E_WRLOCKED;
			}

			if(plock->write_lock == WRITE_UNLOCK){	/*如果写锁解开*/
				plock->read_lock = READ_LOCK;
				plock->rd_lock_count++;
				return S_LOCKED;
			}

			sleep(1);
			circles++;
		}
	}

	/*等待直到解锁*/
	while(1){
		if(plock->write_lock == WRITE_UNLOCK){	/*如果写锁解开*/
			plock->read_lock = READ_LOCK;
			plock->rd_lock_count++;
			return S_LOCKED;
		}
	}

	return E_LOCK;
}

/*
 *解读锁
 *如果读锁已经上锁，则减少读锁数目；如果读锁数目为0则将锁设置为完全解开；
 *如果写锁上锁则失败（这种情况一般不会出现。出现则是因为逻辑错误）
 *@return:
 *S_UNLOCK:解锁成功
 * E_LOCKED:已经有写锁
 * E_ERROR:错误
 */
int drop_read_lock(rdwr_lock_t *plock){
	return 0;
}

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
int set_write_lock(rdwr_lock_t *plock , int seconds){
	return 0;
}

/*
 *解写锁
 *如果写锁已经上锁，则将锁设置为完全解开；
 *如果读锁上锁则失败（这种情况一般不会出现。出现则是因为逻辑错误）
 *@return:
 *S_UNLOCK:解锁成功
 * E_LOCKED:已经有写锁
 * E_ERROR:错误
 */
int drop_write_lock(rdwr_lock_t *plock){
	return 0;
}

/*
 * 获得最右0的个数也就是最右方bit1的偏移
 * 比如011000 返回3
 */
inline int index_last_1bit(int _il1b_number){
		u32 _il1b_index = 0;
		u8 max_bits = sizeof(int) * 8;

		while(_il1b_index<max_bits){
			/*如果找到bit1则返回*/
			if((u32)1<<_il1b_index & _il1b_number){
				break;
			}
			_il1b_index++;
		}
	return _il1b_index;
}
