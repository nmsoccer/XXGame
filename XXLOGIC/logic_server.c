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
static u8 ctrl_msg;	/*收到控制消息标志位。 具体控制信息在游戏运行环境中*/


/////////////////////LOCAL FUNC////////////////////////////////
static void handle_signal(int sig_no);	/*服务器处理信号*/
static int handle_req_client(sspackage_t *sspackage);	/*处理客户端数据包*/
static int handle_ctrl_msg(runtime_env_t *pruntime_env);	/*处理控制信息，比如重新加载模块、卸载模块等工作*/

static int validate_player(int fd , cspackage_t *cs_data);		/*验证玩家信息*/


//static int (* cs_module1_start)(int , int , void *);
//static int (* cs_module2_start)(int , int , void *);
//static int (* cs_module3_start)(int , int , void *);

static MODULE_INT cs_module_starts[CS_PROTO_MODULE_COUNT];


/*
 * 处理主要游戏逻辑的逻辑服务器进程
 */
int main(int argc , char **argv){
	sspackage_t sspackage;
	char arg_segs[ARG_SEG_COUNT][ARG_CONTENT_LEN];	/*参数内容*/
	online_players_t *ponline_players;	/*保存在线玩家信息的结构，共享内存*/
	runtime_env_t *pruntime_env;	/*游戏运行时环境*/
	int index;
	u64 ticks = 0;	/*计时滴答*/

	struct sigaction sig_act;	/*信号动作*/
	int iret;
	int icount;
	int i;

	module_commond_t module_commond;
	void *handle = NULL;	/*动态加载模块句柄*/



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
	sigaction(SIGUSR1 , &sig_act , NULL);
	ctrl_msg = 0;

	/***链接BUS*/
	iret = open_bus(connect_server_id , logic_server_id);
	if(iret < 0){
		write_log(LOG_ERR , "logic_server:open bus connect <-> logic failed!");
		return -1;
	}

	/***获得游戏运行时环境*/
	pruntime_env = (runtime_env_t *)attach_shm_res(GAME_RT_ENV , world_id , line_id);
	if(!pruntime_env){
		write_log(LOG_ERR , "logic_server:attach runtime env failed!");
		return -1;
	}else{
		write_log(LOG_INFO , "logic_server:attach runtime env %x success!" , pruntime_env->basic.global_id);
	}
	//设置相关信息
	index = index_last_1bit(GAME_LOGIC_SERVER);
	pruntime_env->basic.proc_info[index].global_id = logic_server_id;
	pruntime_env->basic.proc_info[index].pid = getpid();
//	printf("proc_t:%x vs pid:%d\n" , pruntime_env->basic.proc_info[index].global_id , pruntime_env->basic.proc_info[index].pid);

	/***链接在线玩家共享内存*/
	ponline_players = (online_players_t *)attach_shm_res(GAME_ONLINE_PLAYERS , world_id , line_id);
	if(!ponline_players){
		return -1;
	}else{
		write_log(LOG_INFO , "logic_server:attach online_players %x success!" , ponline_players->global_id);
	}

	/***加载模块*/
	iret = load_modules(MODULE_TYPE_CS , -1 , cs_module_starts , CS_PROTO_MODULE_COUNT);
	if(iret < 0)
	{
		return -1;
	}
//	cs_module1_start(CS , 1 , NULL);
//	cs_module2_start(1 , 1 , NULL);
//	cs_module3_start(1 , 1 , NULL);

	module_commond.type = MODULE_COMMOND_RELOAD;
	module_commond.data.pmodule_dir_path = "../XXMODULE/";

	for(i=0; i<CS_PROTO_MODULE_COUNT; i++)
	{
		cs_module_starts[i](MODULE_TYPE_CS , NULL , &module_commond);
	}


	/***start success*/
	write_log(LOG_INFO , "start logic_server %s success!" , argv[1]);

	/***主循环*/
	while(1){
		ticks++;	/*循环一次加一次*/

		/***读包*/
		do{
			iret = get_bus_pkg(logic_server_id , connect_server_id , &sspackage);

			if(iret == -1){	/*读包出错*/
				write_log(LOG_ERR , "logic server: recv bus failed!");
				break;
			}
			if(iret == -2){	/*BUS为空*/
				break;
			}

			/*BUS有货*/
			switch(sspackage.sshead.package_type){
			case SS_FROM_CLIENT:
				handle_req_client(&sspackage);
				break;
			default:
				break;
			}
		}while(0);

		/***其他的定时处理机制*/
		//检测是否存在控制信息
		if(ticks % 5 == 0 && ctrl_msg){
			handle_ctrl_msg(pruntime_env);
		}

	}	/*end while*/

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////

static void handle_signal(int sig_no){
	int iret = -1;

	switch(sig_no){
		case SIGINT:
			write_log(LOG_NOTIFY , "logic server: receive sig int. ready to exit...");
			/*脱离BUS*/
			iret = close_bus(connect_server_id , logic_server_id);
			if(iret < 0){
				write_log(LOG_ERR , "logic server: detach bus to connect failed!");
			}
			write_log(LOG_NOTIFY , "logic server: detach bus to connect success!");

			/*脱离游戏运行时环境共享资源*/
			iret = detach_shm_res(GAME_RT_ENV , world_id , line_id);
			if(iret < 0){
				write_log(LOG_ERR , "logic server: detach runtime_env failed!");
			}
			write_log(LOG_NOTIFY , "logic server: detach runtime_env success!");

			/*脱离在线玩家共享资源*/
			iret = detach_shm_res(GAME_ONLINE_PLAYERS , world_id , line_id);
			if(iret < 0){
				write_log(LOG_ERR , "logic server: detach online_players failed!");
			}
			write_log(LOG_NOTIFY , "logic server: detach online_players success!");
			exit(0);
		break;

		/*设置重新加载模块标志*/
		case SIGUSR1:
			printf("recv sig usr1~\n");
			ctrl_msg = 1;
		break;
	default:
		break;
	}

}


/*处理来自客户端的数据包*/
static int handle_req_client(sspackage_t *sspackage){
	u32 proto_type;
	int client_fd;

	/****CHECK********/
	if(!sspackage){
		return -1;
	}

	proto_type = sspackage->data.cs_data.cshead.proto_type;
	client_fd = sspackage->sshead.client_fd;
	/*****HANDLE******/
	switch(proto_type){	/*客户包的类型*/
	case CS_PROTO_VALIDATE_PLAYER:
		validate_player(client_fd , &(sspackage->data.cs_data));
		break;
	default:
		strcat(sspackage->data.cs_data.data.acdata , "logic!");
		break;
	}

	return 0;
}

/*处理控制信息，比如重新加载模块、卸载模块等工作
 * 控制信息由logic_ctrl程序写入游戏运行时环境
 * @param pruntime_env 游戏运行时环境
 * @return:
 * 0:成功
 * -1:失败
 */
static int handle_ctrl_msg(runtime_env_t *pruntime_env){
	int index;
	if(!pruntime_env){
		return -1;
	}
	index = index_last_1bit(GAME_LOGIC_SERVER);
	printf("pid is:%d\n" , pruntime_env->basic.proc_info[index].pid);
	ctrl_msg = 0;	/*重置*/


	return 0;
}


static int validate_player(int client_fd , cspackage_t *cs_data){		/*验证玩家信息*/
	sspackage_t sspackage;
	reply_validate_info_t *reply_validate_info;
	req_validate_info_t *req_validate_info;
	int iret;

	/***INIT***/
	req_validate_info = &cs_data->data.req_validate_player;
	reply_validate_info = &sspackage.data.cs_data.data.reply_validate_player;

#ifdef DEBUG
	printf("Yes , i recv a validate pacakge %s : %s!\n" , req_validate_info->player_name , req_validate_info->player_passwd);
#endif

	/***CHECK***/
	do{
		/*检验版本号*/
		if(cs_data->cshead.major_version != MAJOR_VERSION || cs_data->cshead.minor_version != MINOR_VERSION){
			reply_validate_info->is_validate = NO_VALIDATE_ERRVERSION;
			break;
		}

		/*用户名与密码匹配*/
		if(strcmp(req_validate_info->player_name , req_validate_info->player_passwd) != 0){
			reply_validate_info->is_validate = NO_VALIDATE_ERRPASS;
			break;
		}

		reply_validate_info->is_validate = IS_VALIDATE;

	}while(0);

	/*SEND PACKAGE*/
	sspackage.sshead.package_type = SS_TO_CLIENT;
	sspackage.sshead.client_fd = client_fd;
	sspackage.data.cs_data.cshead.proto_type = CS_PROTO_VALIDATE_PLAYER;

	iret = send_bus_pkg(connect_server_id , logic_server_id , &sspackage);
	while(iret != 0){
//			PRINT("logic server sending bus...");
		send_bus_pkg(connect_server_id , logic_server_id , &sspackage);
		sleep(1);
	}

#ifdef 	DEBUG
	printf("send bus success!\n");
#endif

	return 0;
}
