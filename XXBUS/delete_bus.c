/*
 * delete_bus.c
 *
 *  Created on: 2012-10-7
 *      Author: leiming
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "common.h"
#include "xx_bus.h"
#include "tool.h"

/*
 * 删除各个进程之间的通信BUS
 *
 */


int main(int argc , char **argv){
	int ishm_id = -1;
	key_t key;
	int iRet = -1;
	bus_interface *pstbus_interface;

	proc_t connect_server_id;	/*链接进程*/
	proc_t logic_server_id;			/*逻辑进程*/
	proc_t log_server_id;			/*日志进程*/

	/*check*/
	if(argc < 2){
		printf("error: argument is not enough!\n");
		return -1;
	}

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

	printf("delete bus...\n");
	/*删除connect_server与logic_server的BUS*/

//	key = ftok(PATH_NAME , GAME_CONNECT_SERVER1 + GAME_LOGIC_SERVER1);
//	if(key < 0){
//		log_error("delet_bus: FTOK of Line1 failed!");
//		return -1;
//	}
	key = connect_server_id | logic_server_id;


/*	size = sizeof(bus_interface);
//	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍
//		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
//	}

//	ishm_id = shmget(key , size , BUS_MODE_FLAG);
*/
	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		log_error("delete_bus: Get bus between connect and logic failed!");
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);
	printf("pstbus_interface: %d vs %d\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);


	iRet = shmctl(ishm_id , IPC_RMID , NULL);
	if(iRet < 0){
		log_error("delete_bus: Remove bus between conncet and logic failed!");
		return -1;
	}
	PRINT("delete bus between connect and logic success!");

	return 0;
}
