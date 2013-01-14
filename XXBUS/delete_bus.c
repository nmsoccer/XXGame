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
	online_players_t *online_players;
	char arg_segs[ARG_SEG_COUNT][ARG_CONTENT_LEN] = {0};
	int world_id;
	int line_id;
	int icount;

	proc_t connect_server_id;	/*链接进程*/
	proc_t logic_server_id;			/*逻辑进程*/
	proc_t log_server_id;			/*日志进程*/

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

	/************删除各个服务进程之间的通信BUS***********************/
	connect_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_CONNECT_SERVER;
	logic_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOGIC_SERVER;
	log_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOG_SERVER;

	printf("delete bus...\n");
	/*删除connect_server与logic_server的BUS*/
	key = connect_server_id | logic_server_id;
	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "delete_bus: shmget bus %x between connect and logic failed!" , key);
		return -1;
	}

	pstbus_interface = (bus_interface *)shmat(ishm_id , NULL , 0);
	if(!pstbus_interface){
		write_log(LOG_ERR , "delete_bus:shmat bus %x between connect and logic failed!" , key);
		return -1;
	}

#ifdef DEBUG
	printf("pstbus_interface: %x vs %x\n" , pstbus_interface ->udwproc_id_recv_ch1 , pstbus_interface ->udwproc_id_recv_ch2);
#endif

	iRet = shmctl(ishm_id , IPC_RMID , NULL);
	if(iRet < 0){
		write_log(LOG_ERR , "delete_bus: remove bus %x between conncet and logic failed!" , key);
		return -1;
	}
	write_log(LOG_INFO , "delete_bus: remove bus %x between connect and logic success!\n" , key);


	/************删除共享资源***********************/
	/****删除在线玩家信息结构*****/
	key = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_RES | GAME_ONLINE_PLAYERS;

	ishm_id = shmget(key , 0 , 0);
	if(ishm_id < 0 ){
		write_log(LOG_ERR , "delete_bus: shmget online_players failed!");
		return -1;
	}

	online_players = (online_players_t *)shmat(ishm_id , NULL , 0);	/*获得该数据结构*/
	if(!online_players){
		write_log(LOG_ERR , "delete_bus:shmat online_players failed!");
		return -1;
	}

	/*删除结构体*/
	iRet = shmctl(ishm_id , IPC_RMID , NULL);
	if(iRet < 0){
		write_log(LOG_ERR , "delete_bus: remove online_players %x failed!" , online_players->global_id);
		return -1;
	}
	write_log(LOG_INFO , "delete_bus:remove online_players %x success!\n" , online_players->global_id);


	return 0;
}
