/*
 * common.h
 *
 *  Created on: 2012-10-5
 *      Author: leiming
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "mytypes.h"



#define CS_DATA_LEN 512

#define DEBUG

/////////////////////////MACRO////////////////////////////
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
 * 各个进程在游戏环境中的ID
 */
//#define GAME_CONNECT_SERVER 	0x01    /*00000001b*/
//#define GAME_LOGIC_SERVER			0x02    /*00000010b*/
#define GAME_CONNECT_SERVER1	11	/*1线*/
#define GAME_LOGIC_SERVER1			12

#define GAME_CONNECT_SERVER2	21	/*2线*/
#define GAME_LOGIC_SERVER2			22

#define GAME_CONNECT_SERVER3	31	/*3线*/
#define GAME_LOGIC_SERVER3	22	32

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
