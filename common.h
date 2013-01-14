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
#include <string.h>
#include <errno.h>

#include "mytypes.h"
#include "package.h"
#include "player_info.h"

extern int errno;
#define DEBUG

/*=======================MACRO=========================================*/
#ifdef DEBUG
#define PRINT(e) do{ 			\
		printf("%s\n" , e); 			\
}while(0)

#else
#define PRINT(e)
#endif

/*内存页面大小*/
#define PAGE_SIZE 1024

/*进程能够支持最多同时接入用户的数目*/
#define MAX_CONNECT_CLIENTS 1024


/*
 * 打开文件的权限
 */
#define MODE_READ_FILE		S_IRUSR | S_IXUSR
#define MODE_WRITE_FILE		S_IWUSR | S_IXUSR
#define MODE_RDWR_FILE		(S_IRUSR | S_IWUSR | S_IXUSR)


/*
 * 参数字符串的分割
 */
#define ARG_SEG_COUNT	10	/*参数字符串最多段数*/
#define ARG_CONTENT_LEN	8	/*参数字符串每段最大长度*/

/*
 * 各个进程在游戏环境中的ID
 * 一个游戏全局ID分为以下部分：
 * WORLD_ID      LINE_ID    FLAG   SERVER_PROC_ID
 * 31--    --28     27----24     23     22---         --- 0
 * 世界ID占据28～31位   一共支持16个世界
 * 线ID占据24～27位     每个世界共支持16个线
 * FLAG占据23位   用于标志低23位是服务进程ID还是资源ID
 * 如果FLAG被设置为 FLAG_SERV 则0～22位为服务进程  每条线共支持23个服务进程
 * 如果FLAG被设置为FLAG_RES 则0~22位为资源ID 一共支持2exp(23)种资源类型
 */
#define MAX_WORLD_COUNT 15	/*一共支持15个世界 1~15*/
#define GAME_WORLD_MASK 0xF0000000	/*世界掩码*/
#define GEN_WORLDID(n) (n << 28 & GAME_WORLD_MASK)	/*生成世界编号*/

#define MAX_LINE_COUNT	15/*每个世界最多15条线 1~15*/
#define GAME_LINE_MASK 0x0F000000	/*获得线号的掩码*/
#define GEN_LINEID(n) (n << 24 & GAME_LINE_MASK)	/*生成线编号*/


#define FLAG_SERV	0x00000000 /*服务标志:低0~22位为服务进程*/
#define FLAG_RES		0x00800000 /*资源标志:低0~22位为资源ID*/
#define GAME_FLAG_MASK 0x00800000	/*杂项掩码*/
//#define GEN_FLAG(n) (n << 23 & GAME_FLAG_MASK)

//进程标识
#define MAX_SERV_COUNT	23	/*每条线支持23个服务进程*/
#define GAME_CONNECT_SERVER	0x00000001		/*链接客户端服务进程*/
#define GAME_LOGIC_SERVER			0x00000002	/*处理游戏主逻辑的逻辑进程*/
#define GAME_LOG_SERVER			0x00000004	/*处理日志的日志服务器*/
#define GAME_SERV_MASK 0x007FFFFF	/*获得进程号的掩码*/

//资源标识
#define GAME_RT_ENV	0x00000001		/*runtime enviroment用于各个进程之间共享的运行时环境*/
#define GAME_ONLINE_PLAYERS 0x00000010	/*在线玩家信息*/

//#define GAME_SHARE_SPACE



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
#define LOG_INFO 1 /*一些发生的一般信息。在非DEBUG版不会打印信息在TTY*/
#define LOG_NOTIFY 2 /*发生的重要信息。会打印信息在TTY*/
#define LOG_ERR	3 /*发生的普通错误信息，但不会引起服务器DOWN机*/
#define LOG_DUMP 4 /*发生严重的错误，服务器无法启动或者DOWN掉*/


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
 * 在线玩家信息
 */
#define MAX_ONLINE_PLAYERS MAX_CONNECT_CLIENTS	/*最多支持在线人数*/

struct _online_players{
	u32 global_id;	/*资源全局ID*/
	player_info_t info[MAX_ONLINE_PLAYERS];	/*在线玩家的信息*/
};
typedef struct _online_players online_players_t;






#endif /* COMMON_H_ */
