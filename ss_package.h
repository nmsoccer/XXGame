/*
 * ss_package.h
 *
 *  Created on: 2013-1-16
 *      Author: leiming
 */

#ifndef SS_PACKAGE_H_
#define SS_PACKAGE_H_

#include "common.h"
#include "cs_package.h"

/**************************SS服务器进程之间数据包头文件******************************/

/*=======================MACRO=========================================*/
#define SS_NORMAL	1	/*普通的服务器包类型*/
#define SS_BROADCAST 2	/*广播包*/

//#define SS_REQ_CLIENT 1 /*来自客户端*/
//#define SS_REPLY_CLIENT 2 /*答复客户端*/
#define SS_FROM_CLIENT 1 /*来自客户端包*/
#define SS_TO_CLIENT 2	/*发往客户端包*/


/*----------------------data里的不同类型数据----------------------*/
/*
struct _ss_req_client{
	cspackage_t cs_data;
};
typedef struct _ss_req_client ss_req_client_t;

struct _ss_reply_client{
	cspackage_t cs_data;
};
typedef struct _ss_reply_client ss_reply_client_t;
*/
/*=======================DATA STRUCT=========================================*/
/*
 * 服务器进程之间交互的数据包
 */
struct _sshead{	/*服务器进程之间包的包头*/
	u16  package_type;		/*服务器包类型*/
	int client_fd;	/*与客户端连接的socket fd*/
}sshead;
typedef struct _sshead sshead_t;

union _ssdata{
	cspackage_t cs_data;
};
typedef union _ssdata ssdata_t;

struct _sspackage{	/*服务器进程之间的包*/
	sshead_t sshead;
	ssdata_t data;
};
typedef struct _sspackage sspackage_t;

#endif /* SS_PACKAGE_H_ */
