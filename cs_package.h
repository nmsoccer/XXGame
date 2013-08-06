/*
 * package.h
 *
 *  Created on: 2013-1-13
 *      Author: leiming
 */

#ifndef PACKAGE_H_
#define PACKAGE_H_


#include "player_info.h"
#include "XXMODULE/module.h"
/*每一种协议在客户端都会对应与之相关的两种数据类型 req_xx; reply_xx;原因是客户端发送与接收所需要的消息未必是一致的*/

/**************************CS数据包头文件******************************/
#define MAJOR_VERSION 0
#define MINOR_VERSION 1

/*=======================MACRO=========================================*/
#define CS_DATA_LEN 512	/*CS包数据长度*/



/*
 * proto type CS主协议号
 *
 * 某个协议所在的模块号为:
 * 协议号 % CS_PROTO_MODULE_SPACE
 */

#define CS_PROTO_VALIDATE_PLAYER	(CS_PROTO_MODULE_1_BASE + 1)	/*验证用户信息*/
#define IS_VALIDATE 1
#define NO_VALIDATE_NOUSR 0	/*不是有效用户*/
#define NO_VALIDATE_ERRPASS 2	/*密码错误*/
#define NO_VALIDATE_ERRVERSION 3	/*版本错误*/

#define CS_PROTO_TEST 65530	/*测试类型包*/

///////////////////////////////////////////各种用户数据包///////////////////////////////////////
/*验证用户信息数据*/
struct _req_validate_info
{
	u8 is_validate;	/*是否通过认证*/
	u16 proto_type;	/*validate*/
	char player_name[PLAYER_NAME_LEN];
	char player_passwd[PLAYER_PASSWD_LEN];

};
typedef struct _req_validate_info req_validate_info_t;

struct _reply_validate_info{
	u8 is_validate;	/*是否通过认证*/
};
typedef struct _reply_validate_info reply_validate_info_t;



/*=======================DATA STRUCT=========================================*/
/*
 * 客户与服务端交互数据包
 */
struct _cshead{	/*CS包头*/
	u32 seq;	/*包的序号*/
	u16 proto_type;
	u16 validate_seq;	/*验证序号; validate_seq = (seq >> major_version + minor_version) % proto_type */
	u8 major_version;	/*主版本号*/
	u8 minor_version;	/*副版本号*/
};
typedef struct _cshead cshead_t;


union _csdata{
	req_validate_info_t req_validate_player;
	reply_validate_info_t reply_validate_player;
	char acdata[CS_DATA_LEN];
};
typedef union _csdata csdata_t;

typedef struct{
	cshead_t cshead;

	csdata_t data;/*CS包内容*/
}cspackage_t;





#endif /* PACKAGE_H_ */
