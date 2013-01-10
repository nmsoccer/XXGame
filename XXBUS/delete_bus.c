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
		write_log(LOG_ERR , "delete_bus:argc < 2 , please input more information like: ./delete_bus line1\n");
		return -1;
	}

	do{
		if(strcasecmp(argv[1] , "line1") == 0){	/*1线*/
			connect_server_id = GEN_WORLDID(1) | GEN_LINEID(1) | FLAG_SERV | GAME_CONNECT_SERVER;
			logic_server_id = GEN_WORLDID(1) | GEN_LINEID(1) | FLAG_SERV | GAME_LOGIC_SERVER;
			log_server_id = GEN_WORLDID(1) | GEN_LINEID(1) | FLAG_SERV | GAME_LOG_SERVER;
			break;
		}
		if(strcasecmp(argv[1] , "line2") == 0){	/*2线*/
			connect_server_id = GEN_WORLDID(1) | GEN_LINEID(2) | FLAG_SERV | GAME_CONNECT_SERVER;
			logic_server_id = GEN_WORLDID(1) | GEN_LINEID(2) | FLAG_SERV | GAME_LOGIC_SERVER;
			log_server_id = GEN_WORLDID(1) | GEN_LINEID(2) | FLAG_SERV | GAME_LOG_SERVER;
			break;
		}

		write_log(LOG_ERR , "delete_bus:illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}while(0);

	printf("delete bus...\n");
	/*删除connect_server与logic_server的BUS*/
	key = connect_server_id | logic_server_id;
	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "delete_bus: shmget bus between connect and logic failed!");
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);
	if(!pstbus_interface){
		write_log(LOG_ERR , "delete_bus:shmat bus between connect and logic failed!");
		return -1;
	}

#ifdef DEBUG
	printf("pstbus_interface: %x vs %x\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);
#endif

	iRet = shmctl(ishm_id , IPC_RMID , NULL);
	if(iRet < 0){
		write_log(LOG_ERR , "delete_bus: remove bus between conncet and logic failed!");
		return -1;
	}


	write_log(LOG_INFO , "delete bus between connect and logic success!\n");


	return 0;
}
