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
static bus_interface *bus_to_connect = NULL;	/*与connect_server链接的bus*/

static u8 connect_server_id;	/*connect_server的ID*/

static u8 logic_server_id;	/*logic_server的ID*/

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
	if(strcmp(argv[1] , "LOGIC1") == 0){	/*logic_server1*/
		connect_server_id = GAME_CONNECT_SERVER1;
		logic_server_id = GAME_LOGIC_SERVER1;

		bus_to_connect = attach_bus(connect_server_id , logic_server_id);

		if(!bus_to_connect){
			log_error("logic server1: attach to connect failed!\n");
			return -1;
		}
		printf("attach: %x %d vs %d\n" , bus_to_connect , bus_to_connect->udwproc_id_recv_ch1 , bus_to_connect->udwproc_id_recv_ch2);
	}else{
		log_error("logic server1: illegal param1");
		log_error(argv[1]);
		return -1;
	}

	PRINT("logic server starts...");

	/*主循环*/
	while(1){
		iret = recv_bus(logic_server_id , connect_server_id , bus_to_connect , &sspackage);
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
		iret = send_bus(connect_server_id , logic_server_id , bus_to_connect , &sspackage);
		while(iret != 0){
			PRINT("logic server sending bus...");
			send_bus(connect_server_id , logic_server_id , bus_to_connect , &sspackage);
			sleep(1);
		}
		PRINT("send bus success!\n");

	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////

static void handle_signal(int sig_no){
	int iRet = -1;

	switch(sig_no){
		case SIGINT:
			PRINT("logic server: receive sig int. ready to exit...");
			iRet = detach_bus(bus_to_connect);
			if(iRet < 0){
				log_error("logic server: detach bus to connect failed!");
			}
			PRINT("logic server: detach bus to connect success!");
		break;
	default:
		break;
	}

	exit(0);
}


