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

	printf("create bus...\n");
	/*创建connect_server1与logic_server1的BUS*/
	key = ftok(PATH_NAME , GAME_CONNECT_SERVER1 + GAME_LOGIC_SERVER1);
	if(key < 0){
		log_error("create_bus: FTOK of Line1 failed!");
		return -1;
	}


	size = sizeof(bus_interface);
	printf("real size is: %x\n" , size);
	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍*/
		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
	}
	printf("alloc size is: %x\n" , size);

	ishm_id = shmget(key , size , IPC_CREAT | IPC_EXCL | BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		log_error("create_bus: Create bus between connect1 and logic1 failed!");
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);	/*设置该管道INTEFACE*/
	if(!pstbus_interface){
		log_error("Attach bus between connect and logic failed!");
		return -1;
	}

	memset(pstbus_interface , 0 , size);
	pstbus_interface->udwproc_id_recv_ch1 = GAME_CONNECT_SERVER1;	/*管道1用于CONNECT收包*/
	pstbus_interface->udwproc_id_recv_ch2 = GAME_LOGIC_SERVER1;		/*管道2用于LOGIC收包*/


	printf("create bus of connect1 and logic1 success...%x:\n" , pstbus_interface);
	printf("pstbus_interface: %d vs %d\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);
	printf("channel one: %x ~ %x\n" , pstbus_interface->channel_one.data , &pstbus_interface->channel_one.data[CHANNEL_MAX_PACKAGE]);
	printf("channel two: %x ~ %x\n" , pstbus_interface->channel_two.data , &pstbus_interface->channel_two.data[CHANNEL_MAX_PACKAGE]);

	PRINT("create bus from connect to logic success!");
/*
	printf("here!\n");
	memcpy(pstbus_interface->channel_two.start_addr , "helloworld" , 9);
	printf("it is:%s\n" , pstbus_interface->channel_two.start_addr);
*/
	return 0;
}
