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
static proc_t connect_server_id;	/*该线链接服务进程的全局ID*/
static proc_t logic_server_id;		/*该线逻辑服务器进程的全局ID*/
static proc_t log_server_id;			/*该线LOG服务器进程的全局ID*/

/////////////////////LOCAL FUNC////////////////////////////////
static void handle_signal(int sig_no);

/*
 * 处理主要游戏逻辑的逻辑服务器进程
 */
int main(int argc , char **argv){
	SSPACKAGE sspackage;

	struct sigaction sig_act;	/*信号动作*/



	int iret;

	/*检测参数*/
	if(argc < 2){
		printf("argc < 2 , please input more information like: logic_server LOGIC1\n");
		return -1;
	}

	/*注册信号*/
	memset(&sig_act , 0 , sizeof(sig_act));
	sig_act.sa_handler = handle_signal;
	sigaction(SIGINT , &sig_act , NULL);


	/*链接BUS*/
	do{
		if(strcasecmp(argv[1] , "line1") == 0){	/*1线*/
			connect_server_id = GAME_LINE_1 | GAME_CONNECT_SERVER;
			logic_server_id = GAME_LINE_1 | GAME_LOGIC_SERVER;
			log_server_id = GAME_LINE_1 | GAME_LOG_SERVER;
			break;
		}
		if(strcasecmp(argv[1] , "line2") == 0){	/*2线*/
			connect_server_id = GAME_LINE_2 | GAME_CONNECT_SERVER;
			logic_server_id = GAME_LINE_2 | GAME_LOGIC_SERVER;
			log_server_id = GAME_LINE_2 | GAME_LOG_SERVER;
			break;
		}

		printf("illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}while(0);

	iret = open_bus(connect_server_id , logic_server_id);
	if(iret < 0){
		printf("open bus failed!\n");
	}

	PRINT("logic server starts...");

	/*主循环*/
	while(1){
//		iret = recv_bus(logic_server_id , connect_server_id , bus_to_connect , &sspackage);
		iret = get_bus_pkg(logic_server_id , connect_server_id , &sspackage);
		if(iret == -1){	/*读包出错*/
			log_error("logic server: recv bus failed!");
		}
		if(iret == -2){	/*BUS为空*/
//			PRINT("bus is empty!");
			sleep(1);
			continue;
		}
		PRINT("recv connect bus success~!");


		strcat(sspackage.cs_data.data.acdata , "logic!");
//		iret = send_bus(connect_server_id , logic_server_id , bus_to_connect , &sspackage);
		iret = send_bus_pkg(connect_server_id , logic_server_id , &sspackage);
		while(iret != 0){
			PRINT("logic server sending bus...");
//			send_bus(connect_server_id , logic_server_id , bus_to_connect , &sspackage);
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
				log_error("logic server: detach bus to connect failed!");
			}
			PRINT("logic server: detach bus to connect success!");
		break;
	default:
		break;
	}

	exit(0);
}


