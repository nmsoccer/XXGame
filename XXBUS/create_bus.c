/*
 * create_bus.c
 *
 *  Created on: 2012-10-7
 *      Author: leiming
 */

/*
 * 创建各个本地进程之间通信的BUS
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "common.h"
#include "xx_bus.h"
#include "tool.h"

int main(int argc , char **argv){
	int ishm_id = -1;
	size_t size = 0;
	key_t key;
	bus_interface *pstbus_interface = NULL;
	char *src;

	proc_t connect_server_id;	/*链接进程*/
	proc_t logic_server_id;			/*逻辑进程*/
	proc_t log_server_id;			/*日志进程*/

	/*check*/
	if(argc < 2){
//		printf("error: argument is not enough!\n");
		write_log(LOG_ERR , "create_bus:argc < 2 , please input more information like: ./create_bus line1\n");
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

		write_log(LOG_ERR , "create_bus:illegal argument 1:%s , exit!" , argv[1]);
//		printf("illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}while(0);



	printf("create bus...\n");
	/*创建connect_server与logic_server 的BUS*/
/*	key = ftok(PATH_NAME , GAME_CONNECT_SERVER1 + GAME_LOGIC_SERVER1);
//	if(key < 0){
//		log_error("create_bus: FTOK of Line1 failed!");
//		return -1;
//	}
 */
	key = connect_server_id | logic_server_id;

	size = sizeof(bus_interface);
	printf("real size is: %x\n" , size);
	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍*/
		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
	}

	ishm_id = shmget(key , size , IPC_CREAT | IPC_EXCL | BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "create_bus: shmget bus between connect and logic failed!");
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);	/*设置该管道INTEFACE*/
	if(!pstbus_interface){
		write_log(LOG_ERR , "create_bus:shmat bus between connect and logic failed!");
		return -1;
	}

	memset(pstbus_interface , 0 , size);
	pstbus_interface->udwproc_id_recv_ch1 = connect_server_id;	/*管道1用于CONNECT收包*/
	pstbus_interface->udwproc_id_recv_ch2 = logic_server_id;		/*管道2用于LOGIC收包*/


	write_log(LOG_INFO , "create_bus:create bus of connect and logic success...");
#ifdef DEBUG
	printf("pstbus_interface: %x vs %x\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);
	printf("channel one: %x ~ %x\n" , pstbus_interface->channel_one.data , &pstbus_interface->channel_one.data[CHANNEL_MAX_PACKAGE]);
	printf("channel two: %x ~ %x\n" , pstbus_interface->channel_two.data , &pstbus_interface->channel_two.data[CHANNEL_MAX_PACKAGE]);
#endif
	return 0;
}
