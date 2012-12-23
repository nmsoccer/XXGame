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

	size = sizeof(bus_interface);
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
//	PRINT("sending bus...");
	bus_channel *channel = NULL;	/*发送管道*/
	char *dest = 0xb784b4888;

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

//	printf("c1:%x; c2:%x; connect:%x head\n" , &bus->channel_one , &bus->channel_two , channel);
	/*使用channel发送*/
	if(channel->package_num == CHANNEL_MAX_PACKAGE){	/*发送管道已满*/
		return -2;
	}

 	/*将包拷入队列尾部*/
	/*不用上锁的原因在于最后才增加包数量；如果提前增加包数量再拷贝包的话则可能出现当head与tail指向同一块区域时，
	 * 读管道进程将读到一个空包
	 */
//	get_spin_lock(&channel->spin_lock);	/*上锁*/
//	memcpy(channel->tail , "helloworld" , 9);
	memcpy(&channel->data[channel->tail] , package , sizeof(SSPACKAGE));
	channel->tail = (channel->tail + 1) % CHANNEL_MAX_PACKAGE;
	channel->package_num++;
//	drop_spin_lock(&channel->spin_lock);	/*解锁*/

	return 0;

}

/*
 * 通过BUS接收包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @bus: 使用的BUS
 * @package: 接收包缓冲区
 * @return:
 * 0:接收成功；
 * -1：出现错误
 * -2: BUS空
 */
int recv_bus(u8 recv_proc , u8 send_proc , bus_interface *bus , SSPACKAGE *package){
	bus_channel *channel = NULL;	/*接收管道*/

	if(!bus || !package){
		return -1;
	}

	/*检验BUS与收发进程ID是否一致*/
	if(recv_proc == bus->udwproc_id_recv_ch1){	/*如果接收进程使用channel1，发送进程使用channel2*/
		if(send_proc != bus->udwproc_id_recv_ch2){
			return -1;
		}
		channel = &bus->channel_one;	/*接收进程使用channel1接收*/

	}else if(recv_proc == bus->udwproc_id_recv_ch2){ /*如果接收进程使用channel2，发送进程使用channel1*/
		if(send_proc != bus->udwproc_id_recv_ch1){
			return -1;
		}
		channel = &bus->channel_two;	/*接收进程使用channel2接收*/
	}else{
		return -1;
	}

	/*使用channel发送*/
	/*使用channel发送*/
	if(channel->package_num == 0){		/*接收管道为空*/
	  	return -2;
	}


 	/*从队头取包*/
	/*不用上锁的原因在于最后才减少包数量；如果提前减少包数量再拷贝包的话则可能出现当head与tail指向同一块区域时，
	 * 再读出该包之前写管道进程已经将其覆盖
	 */
//	get_spin_lock(&channel->spin_lock);	/*上锁*/
	memcpy(package ,  &channel->data[channel->head] , sizeof(SSPACKAGE));
	channel->head = (channel->head + 1) % CHANNEL_MAX_PACKAGE;
	channel->package_num--;
//	drop_spin_lock(&channel->spin_lock);	/*解锁*/

	return 0;
}
