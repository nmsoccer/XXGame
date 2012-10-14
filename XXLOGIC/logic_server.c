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

#include "XXBUS/xx_bus.h"

//////////////////////LOCAL VAR/////////////////////////////////
static bus_interface *bus_to_connect = NULL;	/*与connect_server链接的bus*/

/////////////////////LOCAL FUNC////////////////////////////////
static void handle_signal(int sig_no);

/*
 * 处理主要游戏逻辑的逻辑服务器进程
 */
int main(int argc , char **argv){
	struct sigaction sig_act;	/*信号动作*/
	int iRet;

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
		bus_to_connect = attach_bus(GAME_CONNECT_SERVER1 , GAME_LOGIC_SERVER1);

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


	/*主循环*/
	while(1){
		PRINT("LOGIC1...");
		sleep(1);
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


