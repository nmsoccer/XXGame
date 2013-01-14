/*
 * player_info.h
 *
 *  Created on: 2013-1-13
 *      Author: leiming
 */

#ifndef PLAYER_INFO_H_
#define PLAYER_INFO_H_

/**************************玩家信息头文件******************************/

/*=======================MACRO=========================================*/
#define PLAYER_NAME_LEN	64	/*角色名*/
#define PLAYER_PASSWD_LEN	32/*密码长度*/




/*=======================DATA STRUCT=========================================*/
struct _player_info{
	u32 global_id;	/*玩家的全局ID*/
	char player_name[PLAYER_NAME_LEN];
	char player_passwd[PLAYER_PASSWD_LEN];
};

typedef struct _player_info player_info_t;





#endif /* PLAYER_INFO_H_ */
