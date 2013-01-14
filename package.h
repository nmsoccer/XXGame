/*
 * package.h
 *
 *  Created on: 2013-1-13
 *      Author: leiming
 */

#ifndef PACKAGE_H_
#define PACKAGE_H_


#include "player_info.h"


/**************************数据包头文件******************************/

/*=======================MACRO=========================================*/
#define CS_DATA_LEN 512	/*CS包数据长度*/
/*
 * proto type
 */
#define CS_PROTO_VALIDATE_PLAYER	1	/*验证用户信息*/
#define CS_PROTO_TEST 255		/*测试类型包*/





/*=======================DATA STRUCT=========================================*/
#define IS_VALIDATE 1
#define NO_VALIDATE 0
/*验证用户信息数据*/
struct _validate_info{
	u8 is_validate;	/*是否通过认证*/
	u16 proto_type;	/*validate*/
	char player_name[PLAYER_NAME_LEN];
	char player_passwd[PLAYER_PASSWD_LEN];

};

typedef struct _validate_info validate_info_t;



/*
 * 客户与服务端交互数据包
 */
struct _cshead{	/*CS包头*/
	u32 proto_type;
};

typedef struct _cshead cshead_t;

typedef struct{
	cshead_t cshead;

	union _data{			/*CS包内容*/
		validate_info_t validate_player;
		char acdata[CS_DATA_LEN];
	} data;
}cspackage_t;


/*
 * 服务器进程之间交互的数据包
 */
#define SS_NORMAL	1	/*普通的服务器包类型*/
#define SS_BROADCAST 2	/*广播包*/

struct _sshead{	/*服务器进程之间包的包头*/
	u32  package_type;		/*服务器包类型*/
	int client_fd;	/*与客户端连接的socket fd*/
}sshead;
typedef struct _sshead sshead_t;

struct _sspackage{	/*服务器进程之间的包*/
	sshead_t sshead;
	cspackage_t cs_data;	/*读入或者发送的客户端数据包*/
};
typedef struct _sspackage sspackage_t;



#endif /* PACKAGE_H_ */
