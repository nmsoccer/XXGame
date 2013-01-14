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
//#include "common.h"

#define INT_CONTAINER_LEN (MAX_SERV_COUNT * ((MAX_SERV_COUNT - 1) >> 1))	/*假设MAX_SERV_COUNT中任意两个进程建立链接C23 2*/

/*用于存储不同进程的bus_interface;*/
typedef struct _procs_interface{
	bus_interface *interface;		/*两个通信进程的bus_interface*/
	proc_t or_two_proc;			/*两个通信进程ID的或*/
}procs_interface;

static procs_interface interface_container[INT_CONTAINER_LEN] = {0};	/*通信进程interface的仓库*/

/*根据proc1与proc2来得到与之相关的interface在interface_container中的偏移
 * 成功返回index失败返回-1
 */
static int get_index_container(proc_t proc1 , proc_t proc2);

static bus_interface *attach_bus(proc_t proc1 , proc_t proc2);
static int detach_bus(bus_interface *bus);
static int send_bus(proc_t recv_proc , proc_t send_proc , bus_interface *bus , sspackage_t *package);
static int recv_bus(proc_t recv_proc , proc_t send_proc , bus_interface *bus , sspackage_t *package);

/*
 * 打开BUS，准备通信
 * 要依靠BUS收发包必须要打开BUS
 * @param proc1: 利用BUS通信的一个进程标志
 * @parm proc2: 利用BUS通信的另一个进程标志
 * @return:成功返回0；失败返回-1
 */
int open_bus(proc_t proc1 , proc_t proc2){
	int index = 0;
	proc_t or_procs = 0;
	bus_interface *interface_of_procs = NULL;
	int icount = 0;

	/*如果不同线则不能发送*/
	if(proc1 & GAME_LINE_MASK != proc2 & GAME_LINE_MASK){
		write_log(LOG_ERR , "open_bus failed!:proc1:%x < ->  proc2:%x in different line" , proc1 , proc2);
		return -1;
	}

	/*获得每个进程全局ID的进程号，然后相或*/
	or_procs = (proc1 & GAME_SERV_MASK) | (proc2 & GAME_SERV_MASK);

	/*获取与通信进程相关的interface在interface_container中的偏移*/
	/*找到一个空白项。首先hash之，如果有冲突则往后加*/
	index = or_procs % INT_CONTAINER_LEN;
	while(1){
		if(interface_container[index].or_two_proc == 0 && interface_container[index].interface == NULL){	/*该项还未被使用*/
			break;
		}
		if(interface_container[index].interface != NULL && interface_container[index].or_two_proc == or_procs){	/*BUS已经打开*/

#ifdef DEBUG
			printf("bus between %d <-> %d is opened already!!\n" , proc1 , proc2);
#endif
			return 0;
		}
		index = (index + 1) % INT_CONTAINER_LEN;

		icount++;
		if(icount >= INT_CONTAINER_LEN){/*用于避免死循环。这种情况理论上来说一定不出现*/
			write_log(LOG_ERR , "open bus some bad error happened!\n");
			return -1;
		}

	}

	/*将进程attach到共享内存BUS上*/
	interface_of_procs = attach_bus(proc1 , proc2);
	if(!interface_of_procs){
		printf("open bus failed!\n");
		return -1;
	}

	/*填充数组*/
	interface_container[index].interface = interface_of_procs;
	interface_container[index].or_two_proc = or_procs;

	return 0;
}

/*
 * 关闭BUS
 * 打开BUS收发包之后，如果不想再使用，则调用该函数关闭
 * @param proc1: 利用BUS通信的一个进程标志
 * @parm proc2: 利用BUS通信的另一个进程标志
 * @return:成功返回0；失败返回-1
 */
int close_bus(proc_t proc1 , proc_t proc2){
	int index = -1;
	bus_interface *interface_of_procs = NULL;


	/*如果不同世界则关闭失败*/
	if(proc1 & GAME_WORLD_MASK != proc2 & GAME_WORLD_MASK){
		write_log(LOG_ERR , "close_bus failed!:proc1:%x < ->  proc2:%x in different world" , proc1 , proc2);
		return -1;
	}

	/*如果不同线则关闭失败*/
	if(proc1 & GAME_LINE_MASK != proc2 & GAME_LINE_MASK){
		write_log(LOG_ERR , "close_bus failed!:proc1:%x < ->  proc2:%x in different line" , proc1 , proc2);
		return -1;
	}

	/*获取与通信进程相关的interface在interface_container中的偏移*/
	index = get_index_container(proc1 , proc2);
	if(index == -1){	/*通信BUS并未建立*/
		return 0;	/*未建立BUS也可以认为关闭成功*/
	}

	/*detach bus*/
	interface_of_procs = interface_container[index].interface;
	detach_bus(interface_of_procs);

	/*清空*/
	interface_container[index].interface = NULL;
	interface_container[index].or_two_proc = 0;

	return 0;
}


/*
 * 通过BUS发送包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @param package: 发送包缓冲区
 * @return:
 * 0:发送成功；
 * -1：出现错误
 * -2: BUS满
 */
int send_bus_pkg(proc_t recv_proc , proc_t send_proc , sspackage_t *package){
	int index = -1;
	int iret = -1;

	/*如果不同世界不能发送*/
	if(recv_proc & GAME_WORLD_MASK != send_proc & GAME_WORLD_MASK){
		write_log(LOG_ERR , "send_bus_pkg failed!:send_proc:%x  ->  recv_proc:%x in different world" , send_proc , recv_proc);
		return -1;
	}

	/*如果不同线则不能发送*/
	if(recv_proc & GAME_LINE_MASK != send_proc & GAME_LINE_MASK){
		write_log(LOG_ERR , "send_bus_pkg failed!:send_proc:%x  ->  recv_proc:%x in different line" , send_proc , recv_proc);
		return -1;
	}

	/*获取与通信进程相关的interface在interface_container中的偏移*/
	index = get_index_container(recv_proc , send_proc);
	if(index == -1){	/*通信BUS并未建立*/
//		printf("send bus failed!\n");
		write_log(LOG_ERR , "send_bus_pkg:get_index_container index = -1");
		return -1;
	}

	/*send*/
	iret = send_bus(recv_proc , send_proc , interface_container[index].interface , package);
	return iret;
}

/*
 * 通过BUS接收包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @param package: 接收包缓冲区
 * @return:
 * 0:接收成功；
 * -1：出现错误
 * -2: BUS空
 */
int get_bus_pkg(u32 recv_proc , u32 send_proc , sspackage_t *package){
	int index = -1;
	int iret = -1;

	/*如果不同世界则不能接收*/
	if(recv_proc & GAME_WORLD_MASK != send_proc & GAME_WORLD_MASK){
		write_log(LOG_ERR , "recv_bus_pkg failed!:send_proc:%x  ->  recv_proc:%x in different world" , send_proc , recv_proc);
		return -1;
	}

	/*如果不同线则不能接收*/
	if(recv_proc & GAME_LINE_MASK != send_proc & GAME_LINE_MASK){
		write_log(LOG_ERR , "recv_bus_pkg failed!:send_proc:%x  ->  recv_proc:%x in different line" , send_proc , recv_proc);
		return -1;
	}

	/*获取与通信进程相关的interface在interface_container中的偏移*/
	index = get_index_container(recv_proc , send_proc);
	if(index == -1){	/*通信BUS并未建立*/
//		printf("recv bus failed!\n");
		write_log(LOG_ERR , "recv_bus_pkg:get_index_container index = -1");
		return -1;
	}

	/*recv*/
	iret = recv_bus(recv_proc , send_proc , interface_container[index].interface , package);
	return iret;
}

/*
 * 链接共享资源地址
 * @param res_type:资源类型
 * @world_id:世界ID
 * @line_id:线ID
 * @return:
 * 失败:NULL
 * 成功:资源地址
 */
void *attach_shm_res(int res_type , int world_id , int line_id){
	void *addr;
	int ishm_id;
	key_t key;

	/*CHECK*/
	if(res_type <= 0 || world_id <= 0 || line_id <= 0){
		return NULL;
	}

	/*HANDLE*/
	key = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_RES | res_type;

	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "attach_shm_res: shmget shm res %x failed!" , key);
		return NULL;
	}

	addr = shmat(ishm_id , NULL , 0);
	if(!addr){
		write_log(LOG_ERR , "attach_shm_res: shmat shm res %x failed!" , key);
		return NULL;
	}

	write_log(LOG_INFO , "attach_shm_res: attach shm res %x success!!" , key);
	return addr;
}

/*
 * 剥离该进程具有的共享资源
 * @param res_type:资源类型
 * @world_id:世界ID
 * @line_id:线ID
 * @return:
 * 失败:-1
 * 成功:0
 */
int detach_shm_res(int res_type , int world_id , int line_id){
	int ishm_id;
	key_t key;
	void *addr;
	int iret;

	/*CHECK*/
	if(res_type <= 0 || world_id <= 0 || line_id <= 0){
		return -1;
	}

	/*HANDLE*/
	key = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_RES | res_type;

	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "detach_shm_res: shmget shm res %x failed!" , key);
		return -1;
	}

	addr = shmat(ishm_id , NULL , 0);	/*获得该数据结构*/
	if(!addr){
		write_log(LOG_ERR , "detach_shm_res: shmat shm res %x failed!" , key);
		return -1;
	}

	iret = shmdt(addr);
	if(iret == -1){
		write_log(LOG_ERR , "detach_shm_res: shmdt shm res %x failed!" , key);
		return -1;
	}

	write_log(LOG_INFO , "detach_shm_res: detach shm res %x success!!" , key);
	return iret;
}


/////////////////////////////////////////////////////LOCAL FUNC//////////////////////////////////////

/*根据proc1与proc2来得到与之相关的interface在interface_container中的偏移
 * 成功返回index失败返回-1
 */
static int get_index_container(proc_t proc1 , proc_t proc2){
	int index = 0;
	proc_t or_procs = 0;
	int icount = 0;

	/*获得每个进程全局ID的进程号，然后相或*/
	or_procs = (proc1 & GAME_SERV_MASK) | (proc2 & GAME_SERV_MASK);

	/*获取与通信进程相关的interface在interface_container中的偏移*/
	/*首先hash之，如果有冲突则往后加*/
	index = or_procs % INT_CONTAINER_LEN;
	while(1){
		if(interface_container[index].interface != NULL && interface_container[index].or_two_proc == or_procs){	/*找到对应项*/
			break;
		}

		index = (index + 1) % INT_CONTAINER_LEN;
		icount++;	/*用于避免死循环*/
		if(icount >= INT_CONTAINER_LEN){	/*说明没有找到*/
			return -1;
		}
	}

	return index;
}


/*
 * 链接上BUS
 * @proc1: 利用BUS通信的一个进程标志
 * @proc2:利用BUS通信的另一个进程标志
 * @return:成功返回该BUS接口；失败返回NULL
 */
static bus_interface *attach_bus(proc_t proc1 , proc_t proc2){
	int ishm_id = -1;
	key_t key;
	bus_interface *pstbus_interface = NULL;

	printf("attach bus...\n");
	/*为该进程链接proc1与proc2的BUS*/
	key = proc1 | proc2;

	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		return NULL;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);	/*设置该管道INTEFACE*/
	if(!pstbus_interface){
		return NULL;
	}

	return pstbus_interface;
}

/*
 * 脱离BUS
 * @bus: bus的接口
 * @return:成功返回0；失败返回-1
 */
static int detach_bus(bus_interface *bus){
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
static int send_bus(proc_t recv_proc , proc_t send_proc , bus_interface *bus , sspackage_t *package){
	bus_channel *channel = NULL;	/*发送管道*/

	if(!bus || !package){
		return -1;
	}

	/*检验BUS与收发进程ID是否一致*/
	if((recv_proc | send_proc) != (bus->udwproc_id_recv_ch1 | bus->udwproc_id_recv_ch2)){
		return -1;
	}

	/*寻找发送管道*/
	do{
		if(recv_proc == bus->udwproc_id_recv_ch1){	/*接收进程使用channel1则使用channel1发送*/
			channel = &bus->channel_one;
			break;
		}

		if(recv_proc == bus->udwproc_id_recv_ch2){	/*接收进程使用channel2则使用channel2发送*/
			channel = &bus->channel_two;
			break;
		}

		return -1;

	}while(0);


/*
	if(recv_proc == bus->udwproc_id_recv_ch1){	如果接收进程使用channel1，发送进程使用channel2
		if(send_proc != bus->udwproc_id_recv_ch2){
			return -1;
		}
		channel = &bus->channel_one;	/*发送进程使用channel1发送

	}else if(recv_proc == bus->udwproc_id_recv_ch2){ /*如果接收进程使用channel2，发送进程使用channel1
		if(send_proc != bus->udwproc_id_recv_ch1){
			return -1;
		}
		channel = &bus->channel_two;	/*发送进程使用channel2发送
	}else{
		return -1;
	}*/

	/*使用channel发送*/
	if(channel->package_num == CHANNEL_MAX_PACKAGE){	/*发送管道已满*/
		return -2;
	}

 	/*将包拷入队列尾部*/
	/*不用上锁的原因在于最后才增加包数量；如果提前增加包数量再拷贝包的话则可能出现当head与tail指向同一块区域时，
	 * 读管道进程将读到一个空包
	 */
//	get_spin_lock(&channel->spin_lock);	/*上锁*/
	memcpy(&channel->data[channel->tail] , package , sizeof(sspackage_t));
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
static int recv_bus(u32 recv_proc , u32 send_proc , bus_interface *bus , sspackage_t *package){
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
	memcpy(package ,  &channel->data[channel->head] , sizeof(sspackage_t));
	channel->head = (channel->head + 1) % CHANNEL_MAX_PACKAGE;
	channel->package_num--;
//	drop_spin_lock(&channel->spin_lock);	/*解锁*/

	return 0;
}
