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
	size_t size = 0;
	key_t key;
	int iRet = -1;
	bus_interface *pstbus_interface;

	printf("delete bus...\n");
	/*删除connect_server与logic_server的BUS*/
	key = ftok(PATH_NAME , GAME_CONNECT_SERVER1 + GAME_LOGIC_SERVER1);
	if(key < 0){
		log_error("delet_bus: FTOK of Line1 failed!");
		return -1;
	}

	size = sizeof(bus_interface);
	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍*/
		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
	}

	ishm_id = shmget(key , size , BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		log_error("delete_bus: Get bus between connect1 and logic1 failed!");
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);
	printf("pstbus_interface: %d vs %d\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);

/*
	str = (char *)pstbus_interface;
	printf("herre!\n");
	printf("it is:%s\n" , str);
	memcpy(str , "GGGGGGGGGGGGGGGGGG" , 10);
	printf("it si:%s\n" , str);
*/
//	memcpy(pstbus_interface->channel_two.start_addr , "aaaaaaaaa" , 9);
//	printf("it is:%s\n" , pstbus_interface->channel_two.start_addr);

/*
	addr = shmat(ishm_id , NULL , 0);
	printf("addr: %x %d vs %d\n" , addr , addr->udwproc_id_recv_ch1 , addr->udwproc_id_recv_ch2);
*/
	iRet = shmctl(ishm_id , IPC_RMID , NULL);
	if(iRet < 0){
		log_error("delete_bus: Remove bus between conncet1 and logic1 failed!");
		return -1;
	}
	PRINT("delete bus between connect1 and logic1 success!");

	return 0;
}
