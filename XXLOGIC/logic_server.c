/*
 * logic_server.c
 *
 *  Created on: 2012-10-9
 *      Author: leiming
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "tool.h"

#include "xxmap.h"

#include "XXBUS/xx_bus.h"

//////////////////////LOCAL VAR/////////////////////////////////
static int world_id;		/*世界ID*/
static int line_id;		/*线ID*/
static proc_t connect_server_id;	/*链接服务进程的全局ID*/
static proc_t logic_server_id;		/*逻辑服务器进程的全局ID*/
static proc_t log_server_id;			/*LOG服务器进程的全局ID*/


/////////////////////LOCAL FUNC////////////////////////////////
static void handle_signal(int sig_no);
static int validate_player(validate_info_t *validate_info);		/*验证玩家信息*/
/*
 * 处理主要游戏逻辑的逻辑服务器进程
 */
int main(int argc , char **argv){
	sspackage_t sspackage;
	char arg_segs[ARG_SEG_COUNT][ARG_CONTENT_LEN];	/*参数内容*/
	online_players_t *online_players;	/*保存在线玩家信息的结构，共享内存*/

	struct sigaction sig_act;	/*信号动作*/
	int iret;
	int icount;

	/*检测参数*/
	if(argc < 2){
		write_log(LOG_ERR , "logic_server:argc < 2 , please input more information worldid.lineid.xxxx , like:1.1.xxxx");
		return -1;
	}

	/*解析参数字符串*/
	icount = strsplit(argv[1] , '.' , arg_segs , ARG_SEG_COUNT , ARG_CONTENT_LEN);
	if(icount < 2){
		write_log(LOG_ERR , "logic_server:illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}
	world_id = atoi(arg_segs[0]);	/*世界ID*/
	line_id = atoi(arg_segs[1]);	/*线ID*/

	if(world_id<=0 || world_id>MAX_WORLD_COUNT || line_id<=0 || line_id>MAX_LINE_COUNT){	/*ID不能小于0或超出最大*/
		write_log(LOG_ERR , "logic_server:illegal argument1:%s , exit!" , argv[1]);
		return -1;
	}

#ifdef DEBUG
	printf("world_id:%d , line_id:%d\n" , world_id , line_id);
#endif
	connect_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_CONNECT_SERVER;
	logic_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOGIC_SERVER;
	log_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOG_SERVER;

	/*注册信号*/
	memset(&sig_act , 0 , sizeof(sig_act));
	sig_act.sa_handler = handle_signal;
	sigaction(SIGINT , &sig_act , NULL);


	/*链接BUS*/
	iret = open_bus(connect_server_id , logic_server_id);
	if(iret < 0){
		write_log(LOG_ERR , "logic_server:open bus connect <-> logic failed!");
		return -1;
	}

	/*链接共享资源*/
	online_players = (online_players_t *)attach_shm_res(GAME_ONLINE_PLAYERS , world_id , line_id);
	if(!online_players){
		return -1;
	}

	/*start success*/
	write_log(LOG_INFO , "start logic_server %s success!" , argv[1]);

	/*主循环*/
	while(1){
		iret = get_bus_pkg(logic_server_id , connect_server_id , &sspackage);
		if(iret == -1){	/*读包出错*/
			write_log(LOG_ERR , "logic server: recv bus failed!");
		}
		if(iret == -2){	/*BUS为空*/
			sleep(1);
			continue;
		}
//		PRINT("recv connect bus success~!");

		switch(sspackage.cs_data.cshead.proto_type){
		case CS_PROTO_VALIDATE_PLAYER:
			validate_player(&sspackage.cs_data.data.validate_player);
			break;
		default:
			strcat(sspackage.cs_data.data.acdata , "logic!");
			break;
		}

		/*SEND PACKAGE*/
		iret = send_bus_pkg(connect_server_id , logic_server_id , &sspackage);
		while(iret != 0){
//			PRINT("logic server sending bus...");
			send_bus_pkg(connect_server_id , logic_server_id , &sspackage);
			sleep(1);
		}
		PRINT("send bus success!\n");

	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////

static void handle_signal(int sig_no){
	int iret = -1;

	switch(sig_no){
		case SIGINT:
			PRINT("logic server: receive sig int. ready to exit...");
			/*脱离BUS*/
			iret = close_bus(connect_server_id , logic_server_id);
			if(iret < 0){
				write_log(LOG_ERR , "logic server: detach bus to connect failed!");
			}
			PRINT("logic server: detach bus to connect success!");

			/*脱离共享资源*/
			iret = detach_shm_res(GAME_ONLINE_PLAYERS , world_id , line_id);

		break;
	default:
		break;
	}

	exit(0);
}


static int validate_player(validate_info_t *validate_info){		/*验证玩家信息*/
#ifdef DEBUG
	printf("Yes , i recv a validate pacakge %s : %s!\n" , validate_info->player_name , validate_info->player_passwd);
#endif

	if(strcmp(validate_info->player_name , validate_info->player_passwd) != 0){
		validate_info->is_validate = NO_VALIDATE;
	}else{
		validate_info->is_validate = IS_VALIDATE;
	}
	return 0;
}
