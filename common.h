/*
 * common.h
 *
 *  Created on: 2012-10-5
 *      Author: leiming
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mytypes.h"
#include <string.h>
#include <errno.h>

extern int errno;

#define CS_DATA_LEN 512

#define DEBUG

/*=======================MACRO=========================================*/
#ifdef DEBUG
#define PRINT(e) do{ 			\
		printf("%s\n" , e); 			\
}while(0)

#else
#define PRINT(e)
#endif


#define PAGE_SIZE 1024	/*内存页面大小*/

/*
 * proto type
 */
#define XX_PROTO_VALIDATE	1	/*验证用户信息*/
#define XX_PROTO_TEST 255		/*测试类型包*/


/*
 * 打开文件的权限
 */
#define MODE_READ_FILE		S_IRUSR | S_IXUSR
#define MODE_WRITE_FILE		S_IWUSR | S_IXUSR
#define MODE_RDWR_FILE		(S_IRUSR | S_IWUSR | S_IXUSR)


/*
 * 各个进程在游戏环境中的ID
 * 各服务进程的ID为32位，前8位是线号；后24位是服务进程号
 * 所以最多支持线的数目是8线；单线里最多24个服务进程
 */
//#define GAME_CONNECT_SERVER 	0x01    /*00000001b*/
//#define GAME_LOGIC_SERVER			0x02    /*00000010b*/
#define MAX_LINE_COUNT	8
#define MAX_SERV_PROC	24

#define GAME_LINE_1	0x01000000		/*00000001 ...b*/
#define GAME_LINE_2	0x02000000	/*00000010 ...b*/
#define GAME_LINE_3	0x04000000	/*00000100 ...b*/
#define GAME_LINE_4	0x07000000	/*00001000 ...b*/
#define GAME_LINE_5	0x10000000		/*00010000 ...b*/
#define GAME_LINE_6	0x20000000	/*00100001 ...b*/
#define GAME_LINE_7	0x40000000	/*01000001 ...b*/
#define GAME_LINE_8	0x70000000	/*10000001 ...b*/


#define GAME_CONNECT_SERVER	0x00000001		/*链接客户端服务进程*/
#define GAME_LOGIC_SERVER			0x00000002	/*处理游戏主逻辑的逻辑进程*/
#define GAME_LOG_SERVER			0x00000004	/*处理日志的日志服务器*/


//#define GAME_CONNECT_SERVER1	11	/*1线*/
//#define GAME_LOGIC_SERVER1			12
//#define GAME_LOG_SERVER2			13

//#define GAME_CONNECT_SERVER2	21	/*2线*/
//#define GAME_LOGIC_SERVER2			22

//#define GAME_CONNECT_SERVER3	31	/*3线*/
//#define GAME_LOGIC_SERVER3	32

/*
 * 记录各种LOG事件类型
 */
#define LOG_RECORD_SIZE		1024	/*一条日志记录的长度*/
#define LOG_INFO 1 /*一些发生的重要信息*/
#define LOG_ERR	2 /*发生的普通错误信息，但不会引起服务器DOWN机*/
#define LOG_DUMP 3 /*发生严重的错误，服务器无法启动或者DOWN掉*/


/*
 * 获得当前时间的字符串
 */
#define STR_CURRENT_TIME(_sct_time_str)		do{ \
																	time_t _sct_tp;				\
																	struct tm *_sct_tm;		\
																	_sct_tp = time(0);							\
																	_sct_tm = localtime(&_sct_tp);					\
																	snprintf(_sct_time_str , sizeof(_sct_time_str) , "%d,%d,%d,%d:%d:%d" , 	\
																	_sct_tm->tm_year + 1900 , _sct_tm->tm_mon + 1 , _sct_tm->tm_mday , _sct_tm->tm_hour , _sct_tm->tm_min , _sct_tm->tm_sec);																								\
															}while(0)


/*=======================DATA STRUCT=========================================*/
/*
 * PLAYER INFO
 */
#define PLAYER_NAME_LEN	64	/*角色名*/
#define PLAYER_PASSWD_LEN	32/*密码长度*/

struct stplayer{
	char szname[PLAYER_NAME_LEN];
	char szpasswd[PLAYER_PASSWD_LEN];
};


/*
 * 客户与服务端交互数据包
 */
typedef struct{
	struct _cshead{		/*CS包头*/
		/*协议类型*/
		u16 proto_type;
	}cshead;

	union _data{			/*CS包内容*/
		struct stplayer stplayer_info;
		char acdata[CS_DATA_LEN];
	} data;
}CSPACKAGE;


/*
 * 服务器进程之间交互的数据包
 */
#define SS_NORMAL	1	/*普通的服务器包类型*/
#define SS_BROADCAST 2	/*广播包*/

typedef struct{
	struct _sshead{
		u8  package_type;		/*服务器包类型*/
		int client_fd;	/*与客户端连接的socket fd*/
	}sshead;
	CSPACKAGE cs_data;	/*读入或者发送的客户端数据包*/
}SSPACKAGE;


#endif /* COMMON_H_ */
