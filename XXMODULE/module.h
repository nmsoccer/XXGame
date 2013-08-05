/*
 * module.h
 *
 *
 *	模块命名规则：
 *	每一个模块名如下： 类型_module号.o
 *	类型：该模块的类型
 * 号：该模块在该类型模块组中的号数，从0~n-1
 *	比如：cs_module0.o  cs_module3.o...
 *
 *  Created on: 2013-6-5
 *      Author: leiming
 */

#ifndef MODULE_H_
#define MODULE_H_

#include "../common.h"

//////////////////////////模块种类///////////////////////////////
#define MODULE_TYPE_CS		1		/*CS类型的模块*/



//////////////////////////CS 模块/////////////////////////////////
#define CS_PROTO_MODULE_SPACE		64		/*CS模块所能容纳的协议个数*/
#define CS_PROTO_MODULE_COUNT	3		/*CS模块数目*/



#define CS_PROTO_MODULE_1_BASE	0	/*CS协议处理模块1*/
#define CS_PROTO_MODULE_2_BASE	(CS_PROTO_MODULE_1_BASE + CS_PROTO_MODULE_SPACE)	/*模块2*/
#define CS_PROTO_MODULE_3_BASE	(CS_PROTO_MODULE_2_BASE + CS_PROTO_MODULE_SPACE)	/*模块3*/




////////////////////////FUNCTIONS/////////////////////////////
//extern int module_start(int type , runtime_env_t *pruntime_env , void *ptr)；

/*所有模块的接口函数*/
typedef int (* MODULE_INT)(int type, void* pruntime_env, void *data);

#endif /* MODULE_H_ */
