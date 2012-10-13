/*
 * xx_bus.c
 *
 *  Created on: 2012-10-9
 *      Author: leiming
 */

#include <stdlib.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>

#include "xx_bus.h"

/*
 * 链接上BUS
 * @proc1: 利用BUS通信的一个进程标志
 * @proc2:利用BUS通信的另一个进程标志
 * @return:成功返回该BUS接口；失败返回NULL
 */
bus_interface *attach_bus(u8 proc1 , u8 proc2){
	int ishm_id = -1;
	size_t size = 0;
	key_t key;
	bus_interface *pstbus_interface = NULL;

	printf("attach bus...\n");
	/*为该进程链接proc1与proc2的BUS*/
	key = ftok(PATH_NAME , proc1 + proc2);
	if(key < 0){
		log_error("attach_bus: FTOK failed!");
		return NULL;
	}

	size = sizeof(bus_interface) + sizeof(SSPACKAGE) * PACKAGE_NR_CONN_LOG * 2;	/*双通道*/
	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍*/
		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
	}

	ishm_id = shmget(key , size , BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		log_error("attach_bus: attach bus failed!");
		return NULL;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);	/*设置该管道INTEFACE*/
	if(!pstbus_interface){
		log_error("attach_bus: failed!");
		return NULL;
	}

	return pstbus_interface;
}

/*
 * 脱离BUS
 * @bus: bus的接口
 * @return:成功返回0；失败返回-1
 */
int detach_bus(bus_interface *bus){
	if(!bus){
		return -1;
	}

	return shmdt(bus);
}



/*
 * 通过BUS发送包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @bus: 使用的BUS
 * @package: 发送包缓冲区
 * @return:
 * 0:发送成功；
 * -1：出现错误
 * -2: BUS满
 */
int send_bus(u8 recv_proc , u8 send_proc , bus_interface *bus , SSPACKAGE *package){
	bus_channel *channel = NULL;	/*发送管道*/

	if(!bus || !package){
		return -1;
	}

	/*检验BUS与收发进程ID是否一致*/
	if(recv_proc == bus->udwproc_id_recv_ch1){	/*如果接收进程使用channel1，发送进程使用channel2*/
		if(send_proc != bus->udwproc_id_recv_ch2){
			return -1;
		}
		channel = &bus->channel_one;	/*发送进程使用channel1发送*/

	}else if(recv_proc == bus->udwproc_id_recv_ch2){ /*如果接收进程使用channel2，发送进程使用channel1*/
		if(send_proc != bus->udwproc_id_recv_ch1){
			return -1;
		}
		channel = &bus->channel_two;	/*发送进程使用channel2发送*/
	}else{
		return -1;
	}

	/*使用channel发送*/





}
