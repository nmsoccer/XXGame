/*
 * create_bus.c
 *
 *  Created on: 2012-10-7
 *      Author: leiming
 */

/*
 * 创建服务进程需要的共享内存环境，包括：
 * 1.创建各个本地进程之间通信的BUS
 * 2.创建各种资源类型的共享内存
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
	char arg_segs[ARG_SEG_COUNT][ARG_CONTENT_LEN] = {0};	/*用于解析输入的argv[1] ID字符串*/
	int world_id;
	int line_id;
	int icount;
	int i;

	proc_t connect_server_id;	/*链接进程*/
	proc_t logic_server_id;			/*逻辑进程*/
	proc_t log_server_id;			/*日志进程*/

	runtime_env_t *pruntime_env = NULL;
	online_players_t *ponline_players = NULL;


	/*check*/
	if(argc < 2){
		write_log(LOG_ERR , "create_bus:argc < 2 , please input more information worldid.lineid.0.0 , like:1.1.0.0");
		return -1;
	}

	/*解析参数字符串*/
	icount = strsplit(argv[1] , '.' , arg_segs , ARG_SEG_COUNT , ARG_CONTENT_LEN);
	if(icount < 2){
		write_log(LOG_ERR , "create_bus:illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}
	world_id = atoi(arg_segs[0]);	/*世界ID*/
	line_id = atoi(arg_segs[1]);	/*线ID*/

	if(world_id<=0 || world_id>MAX_WORLD_COUNT || line_id<=0 || line_id>MAX_LINE_COUNT){	/*ID不能小于0或超出最大*/
		write_log(LOG_ERR , "create_bus:illegal argument1:%s , exit!" , argv[1]);
		return -1;
	}

#ifdef DEBUG
	printf("world_id:%d , line_id:%d\n" , world_id , line_id);
#endif


	/************创建各个服务进程之间的通信BUS***********************/
	connect_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_CONNECT_SERVER;
	logic_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOGIC_SERVER;
	log_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOG_SERVER;

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
//	printf("real size is: %x\n" , size);
//	if(size % PAGE_SIZE != 0){	/*必须是页面的整数倍*/
//		size = (size / PAGE_SIZE + 1) * PAGE_SIZE;
//	}

	ishm_id = shmget(key , size , IPC_CREAT | IPC_EXCL | BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "create_bus: shmget bus %x between connect and logic failed!" , key);
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);	/*设置该管道INTEFACE*/
	if(!pstbus_interface){
		write_log(LOG_ERR , "create_bus:shmat bus %x between connect and logic failed!" , key);
		return -1;
	}

	memset(pstbus_interface , 0 , size);
	pstbus_interface->udwproc_id_recv_ch1 = connect_server_id;	/*管道1用于CONNECT收包*/
	pstbus_interface->udwproc_id_recv_ch2 = logic_server_id;		/*管道2用于LOGIC收包*/


	write_log(LOG_INFO , "create_bus:create bus %x of connect and logic success..." , key);
#ifdef DEBUG
	printf("pstbus_interface: %x vs %x\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);
	printf("channel one: %x ~ %x\n" , pstbus_interface->channel_one.data , &pstbus_interface->channel_one.data[CHANNEL_MAX_PACKAGE]);
	printf("channel two: %x ~ %x\n" , pstbus_interface->channel_two.data , &pstbus_interface->channel_two.data[CHANNEL_MAX_PACKAGE]);
#endif

	/************创建共享资源***********************/
	/****创建游戏运行时环境*****/
	key = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_RES | GAME_RT_ENV;
	size = sizeof(runtime_env_t);

	ishm_id = shmget(key , size , IPC_CREAT | IPC_EXCL | BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "create_bus: shmget runtime_env %x failed!" , key);
		return -1;
	}

	pruntime_env = (runtime_env_t *)shmat(ishm_id , NULL , 0);	/*获得该数据结构*/
	if(!pruntime_env){
		write_log(LOG_ERR , "create_bus:shmat runtime_env %x failed!" , key);
		return -1;
	}

	/*初始化结构体*/
	pruntime_env->basic.global_id = key;	/*标识，用于验证*/
	pruntime_env->rdwr_lock.read_lock = READ_UNLOCK;
	pruntime_env->rdwr_lock.write_lock = WRITE_UNLOCK;
	pruntime_env->rdwr_lock.rd_lock_count = 0;
//	pruntime_env->basic.world_id = world_id;
//	pruntime_env->basic.line_id = line_id;
	write_log(LOG_INFO , "create_bus:create runtime_env %x success..." , pruntime_env->basic.global_id);

	/****创建在线玩家信息结构*****/
	key = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_RES | GAME_ONLINE_PLAYERS;
	size = sizeof(online_players_t);

	ishm_id = shmget(key , size , IPC_CREAT | IPC_EXCL | BUS_MODE_FLAG);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "create_bus: shmget online_players %x failed!" , key);
		return -1;
	}

	ponline_players = (online_players_t *)shmat(ishm_id , NULL , 0);	/*获得该数据结构*/
	if(!ponline_players){
		write_log(LOG_ERR , "create_bus:shmat online_players %x failed!" , key);
		return -1;
	}

	/*初始化结构体*/
	ponline_players->global_id = key;	/*标识，用于验证*/
	for(i=0; i<MAX_ONLINE_PLAYERS; i++){		/*不用全部memset为0,只需要全局ID赋值为0*/
		ponline_players->info[i].global_id = 0;
	}
	write_log(LOG_INFO , "create_bus:create online_players %x success..." , ponline_players->global_id);


	return 0;
}
