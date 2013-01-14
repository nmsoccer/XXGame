/*
 * connect_server.c
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "tool.h"
#include "mytypes.h"
#include "mempoll.h"

#include "XXBUS/xx_bus.h"

/*
 * CONNECT PORT
 */
#define CONNECT_PORT 8005

/*
 * MAX LISTEN CLIENT
 */
#define MAX_LISTEN_CLIENTS MAX_CONNECT_CLIENTS

/*用于实际处理客户端TCP SOCKET链接的循环。
 * 具体为接收客户端的包，然后转发消息包给具体逻辑进程
 * 同时接收其他逻辑进程发送的包给相应的客户端

void *connect_interface(void *arg);*/

///////////////////////////////////DATA STRUCTURE///////////////////////////////////////////
/*
 * 数据包队列节点。用于保存将要发送给客户端的数据包.关联到每一个客户端SOCK FD
 */
struct _cspackage_node{
	struct _cspackage_node *node_next;
	cspackage_t cs_data;
};
typedef struct _cspackage_node CSPACKAGE_NODE;

typedef struct{
	u8 recv_count;	/*接收队列里的包数目*/
	u8 send_count;	/*发送队列里的包数目*/
	CSPACKAGE_NODE *recv_head;	/*从客户端接收的包队列头；在BUS满时存储*/
	CSPACKAGE_NODE *recv_tail;	/*从客户端接收的包队列尾*/
	CSPACKAGE_NODE *send_head;	/*将要发送给客户端的包队列头；在不可写时存储*/
	CSPACKAGE_NODE *send_tail;	/*将要发送给客户端的包队列尾*/
}client_connect;

typedef struct{
	int max_active_fd;	/*有效的链接数量*/
	int active_fds[MAX_CONNECT_CLIENTS];	/*记录有效的链接fd值*/
	client_connect client_connects[MAX_CONNECT_CLIENTS];	/*每个fd对应的收发包链接缓冲区，定位数组为fd本身的值*/
}CLIENT;

///////////////////////////////////////////////LOCAL VAR////////////////////////////////////////////
/*
 * 每个socket fd 对应一个成员。成员在数组的位置就是其fd值
 */
//static client_connect client_connects[MAX_CONNECT_CLIENTS];
static CLIENT client;
/*标识信息*/
static int world_id;		/*世界ID*/
static int line_id;		/*线ID*/
static proc_t connect_server_id;	/*链接服务进程的全局ID*/
static proc_t logic_server_id;		/*逻辑服务器进程的全局ID*/
static proc_t log_server_id;			/*LOG服务器进程的全局ID*/
//static bus_interface *bus_to_logic = NULL;

/*connect_server的ID*/
//static u8 connect_server_id;
/*logic_server的ID*/
//static u8 logic_server_id;
/*内存池*/
static xxmem_poll *poll;
//////////////////////////////////////////////FUNCTION/////////////////////////////////////
/*
 * 读取数据
 */
static int read_socket(int isock_fd , cspackage_t *pstcspackage);

/*
 * 发送数据
 */
static int write_socket(int isock_fd , cspackage_t *pstcspackage);

/*
 * 发送某个具体fd的数据包队列
 * relay local var: client
 */
static int send_to_client(int client_fd);

/*
 * 尽可能多地读取与connect_server链接的所有bus数据，并尽可能多地将取得的包发送到对应的客户端；
 * 如果客户端socket写缓冲区满则将该包放入client该socket的发送队列
 * 一个管道最多读10个包然后换管道
 */
static int try_handle_buses(void);


/*
 * 轮询所有活跃链接的发送队列将其队列清空
 */
static int send_to_all_clients(void);

/*
 * 关闭某个客户端需要清除起收发队列里的包
 * relay local var: client
 */
static int close_client(int client_fd);

/*
 * 处理信号函数
 */
static void handle_signal(int sig_no);


////////////////////////////////////////////DEFINE//////////////////////////////////////////////
/*
 * MAIN
 */
int main(int argc , char **argv){
	int iserv_fd;	/*服务器端创建监听的套接字*/
	int iaccept_fd;	/*链接具体客户端的TCP套接字*/
	int iepoll_fd;	/*epoll的文件描述符*/
	int iactive_fds;	/*通过epoll活跃的fd数目*/
	int handle_fd;	/*循环中处理的fd*/
	char arg_segs[ARG_SEG_COUNT][ARG_CONTENT_LEN];	/*参数内容*/
	int icount;

	struct sockaddr_in stserv_addr;
	struct sockaddr_in stcli_addr;
	int iaddr_len = 0;

	struct epoll_event stepoll_event;
	struct epoll_event astepoll_events[MAX_CONNECT_CLIENTS + 1];	/*一个FD对应一个event*/

//	CSPACKAGE stcspackage_recv;	/*收发客户端的包*/
//	CSPACKAGE stcspackage_send;

	sspackage_t sspackage_recv;	/*收发其他服务器进程的包*/
//	sspackage_t sspackage_send;

//	CSPACKAGE_NODE *pstcspackage_node = NULL;

	struct sigaction stsig_act;		/*信号动作变量*/

//	pthread_t tid = 0;	/*开启接口线程的线程ID*/
	int iRet = -1;
	int i;

	u32 ticks = 0;	/*时间滴答*/

	/*检测参数*/
	if(argc < 2){
		write_log(LOG_ERR , "connect_server:argc < 2 , please input more information worldid.lineid.xxxx , like:1.1.xxxx");
		return -1;
	}

	/*解析参数字符串*/
	icount = strsplit(argv[1] , '.' , arg_segs , ARG_SEG_COUNT , ARG_CONTENT_LEN);
	if(icount < 2){
		write_log(LOG_ERR , "connect_server:illegal argument 1:%s , exit!" , argv[1]);
		return -1;
	}
	world_id = atoi(arg_segs[0]);	/*世界ID*/
	line_id = atoi(arg_segs[1]);	/*线ID*/

	if(world_id<=0 || world_id>MAX_WORLD_COUNT || line_id<=0 || line_id>MAX_LINE_COUNT){	/*ID不能小于0或超出最大*/
		write_log(LOG_ERR , "connect_server:illegal argument1:%s , exit!" , argv[1]);
		return -1;
	}

#ifdef DEBUG
	printf("world_id:%d , line_id:%d\n" , world_id , line_id);
#endif
	connect_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_CONNECT_SERVER;
	logic_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOGIC_SERVER;
	log_server_id = GEN_WORLDID(world_id) | GEN_LINEID(line_id) | FLAG_SERV | GAME_LOG_SERVER;

	/*Create sockfd*/
	iserv_fd = socket(PF_INET , SOCK_STREAM , 0 );
	if(iserv_fd < 0){
		write_log(LOG_ERR , "connect_server:create connect server socket failed!");
		return -1;
	}
	iaddr_len = sizeof(struct sockaddr_in);

	/*Bind*/
	memset(&stserv_addr , 0 , sizeof(stserv_addr));
	stserv_addr.sin_family = AF_INET;
	stserv_addr.sin_addr.s_addr = INADDR_ANY;
	stserv_addr.sin_port = htons(CONNECT_PORT);
	iRet = bind(iserv_fd , (struct sockaddr *)&stserv_addr , iaddr_len);
	if(iRet < 0){
		write_log(LOG_ERR , "connect_server:bind connect socket failed!");
		return -1;
	}

	/*Create epoll fd and set it*/
	iepoll_fd = epoll_create(MAX_CONNECT_CLIENTS + 1);	/*包括iserv_fd*/
	if(iepoll_fd < 0){
		write_log(LOG_ERR , "connect_server:epoll create failed!");
		return -1;
	}

	stepoll_event.events = EPOLLIN;	/*add serve fd to epoll*/
	stepoll_event.data.fd = iserv_fd;
	iRet = epoll_ctl(iepoll_fd , EPOLL_CTL_ADD , iserv_fd , &stepoll_event);
	if(iRet < 0){
		write_log(LOG_ERR , "connect_server: append serve fd to epoll failed!");
		return -1;
	}

	/*Listen*/
	listen(iserv_fd , MAX_LISTEN_CLIENTS);

	/*初始化客户端链接*/
	memset(&client , 0 , sizeof(CLIENT));

	/*处理信号*/
	memset(&stsig_act , 0 , sizeof(stsig_act));
	stsig_act.sa_handler = handle_signal;
	sigaction(SIGINT , &stsig_act , NULL);

	/*创建内存池*/
	poll = create_mem_poll();
	if(!poll){
		write_log(LOG_ERR , "connect_server:create mem poll failed!");
		return -1;
	}else{
		write_log(LOG_INFO , "connect_server:create mem poll success!");
	}

	/*链接BUS*/
	iRet = open_bus(connect_server_id , logic_server_id);
	if(iRet < 0){
		write_log(LOG_ERR , "connect_server:open bus connect <-> logic failed!");
		return -1;
	}

	/*start success*/
	write_log(LOG_INFO , "start connect_server %s success!" , argv[1]);

	/*主逻辑*/
	while(1){
		ticks++;		/*循环一次加一个滴答*/

		iactive_fds = epoll_wait(iepoll_fd , astepoll_events , MAX_CONNECT_CLIENTS+1 , 200);
		if(iactive_fds < 0){
			write_log(LOG_ERR , "connect_server:epoll wait error!");
			return -1;
		}

		/*检测每个FD*/
		for(i=0; i<iactive_fds; i++){
			/*-----------监听FD----------------*/
			if(astepoll_events[i].data.fd == iserv_fd){	/*如果是监听socket，则accept*/
				memset(&stcli_addr , 0 , sizeof(stcli_addr));
				iaccept_fd = accept(iserv_fd , (struct sockaddr *)&stcli_addr , &iaddr_len);

				write_log(LOG_INFO , "connect_server:accept a new client: %d\n" , iaccept_fd);
				/*设置接收的socket属性*/
				set_nonblock(iaccept_fd);	/*设置为非阻塞*/
				set_sock_buff_size(iaccept_fd , 10 * sizeof(cspackage_t) , 5 * sizeof(cspackage_t));	/*设置socket缓冲区大小*/

				/*将该socket加入client*/
				client.active_fds[client.max_active_fd] = iaccept_fd;
				client.max_active_fd++;

				/*将该socket设置入epoll监视*/
				stepoll_event.data.fd = iaccept_fd;
				stepoll_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				iRet = epoll_ctl(iepoll_fd ,  EPOLL_CTL_ADD , iaccept_fd , &stepoll_event);
				if(iRet < 0){
					write_log(LOG_ERR , "connect_server:epoll_ctl %d failed!" , iaccept_fd);
				}
				continue;
			}

			/*--------断开链接---------------*/
			if(astepoll_events[i].events & EPOLLRDHUP){
				write_log(LOG_INFO , "connect_server:client %d closed!" , astepoll_events[i].data.fd);
				close_client(astepoll_events[i].data.fd);
				continue;
			}

			/*-----------可以读---------------*/
			if(astepoll_events[i].events & EPOLLIN){
				handle_fd = astepoll_events[i].data.fd;
				/*read client*/
				memset(&sspackage_recv , 0 , sizeof(sspackage_t));
				sspackage_recv.sshead.client_fd = handle_fd;
				iRet = read_socket(handle_fd , &sspackage_recv.cs_data);

				if(iRet == sizeof(cspackage_t)){
#ifdef DEBUG
					printf(">>>read package!\n");
#endif
				}
				else
				if(iRet > 0){	/*读的数据小于一个包长*/
					write_log(LOG_ERR , "connect server: read pakcage bytes len %d < cspackage size %d!" , iRet , sizeof(cspackage_t));
				}
				else{	/*没有数据*/
					write_log(LOG_INFO , "connect server: nothing to read!");
				}

				/*将读取到的包通过BUS发送给logic*/
				iRet = send_bus_pkg(logic_server_id , connect_server_id , &sspackage_recv);
				if(iRet == 0){	/*发送成功*/
#ifdef DEBUG
//					printf("connect server: send bus success!");
#endif
				}
				else
				if(iRet == -2){	/*BUS满*/
					write_log(LOG_ERR , "connect server: send bus to logic full!");
				}
				else
				if(iRet == -1){	/*发送失败*/
					write_log(LOG_ERR , "connect server: send to logic failed!");
				}

				/*同时检测该fd是否有需要发的包*/
				send_to_client(handle_fd);
			}

		}	/*end for*/

		/*-----在处理了socket 接口之后的其他操作--------*/
		try_handle_buses();

		if(ticks % 5 == 0){	/*每隔五个滴答*/
			send_to_all_clients();
		}

	}	/*end while*/

	return 0;
}


/*
 * 读取数据
 * @return: -1 没有数据
 * >0 返回读取的数据长度
 */
static int read_socket(int isock_fd , cspackage_t *cspackage){
	if(isock_fd < 0  || !cspackage){
		return -1;
	}

	return recv(isock_fd , cspackage , sizeof(cspackage_t) , 0);
}

/*
 * 发送数据
 * @return: > 0发送的数据长度
 * -1:发送失败
 */
static int write_socket(int isock_fd , cspackage_t *cspackage){
	if(isock_fd < 0  || !cspackage){
		return -1;
	}

	return send(isock_fd , cspackage, sizeof(cspackage_t) , 0);
}

/*
 * 发送某个具体fd的数据包队列
 * relay local var: client
 */
static int send_to_client(int client_fd){
	CSPACKAGE_NODE *pstnode = NULL;
	CSPACKAGE_NODE *pstnodetmp = NULL;
	client_connect *pstconnect = NULL;	/*具体某个客户端链接*/

	int iRet;

	pstconnect = &client.client_connects[client_fd];
	pstnode = pstconnect->send_head;

	while(1){
		if(pstnode == NULL){
			break;
		}

		iRet = write_socket(client_fd , &pstnode->cs_data);
		if(iRet == -1 || iRet != sizeof(cspackage_t)){	/*如果发送失败或者发送数据小于一个包长，则退出待以后重新发送*/
			write_log(LOG_ERR , "connect_server:send to client %d # write_socket failed!" , client_fd);
			break;
		}

		pstconnect->send_count--;
		pstnodetmp = pstnode;
		pstnode = pstnode->node_next;
		xx_free_mem(poll , pstnodetmp , sizeof(CSPACKAGE_NODE));
	}

	/*调整connect的发送队列首尾指针*/
	if(pstconnect->send_count == 0){	/*发送队列已空*/
		pstconnect->send_head = NULL;
		pstconnect->send_tail = NULL;
	}else{	/*还有未发送的包*/
		pstconnect->send_head = pstnode;
	}

	return 0;
}


/*
 * 关闭某个客户端需要清除起收发队列里的包
 * relay local var: client
 */
static int close_client(int client_fd){
	client_connect *pstconnect = NULL;
	CSPACKAGE_NODE *pstnode = NULL;
	CSPACKAGE_NODE *pstnodetmp = NULL;
	int i;

	pstconnect = &client.client_connects[client_fd];
	/*清除发送队列*/
	pstnode = pstconnect->send_head;
	while(1){
		if(pstnode == NULL){
			break;
		}

		pstnodetmp = pstnode;
		pstnode = pstnode->node_next;
//		free(pstnodetmp);
		xx_free_mem(poll , pstnodetmp , sizeof(CSPACKAGE_NODE));
	}

	/*清除接收队列*/
	pstnode = pstconnect->recv_head;
	while(1){
		if(pstnode == NULL){
			break;
		}

		pstnodetmp = pstnode;
		pstnode = pstnode->node_next;
//		free(pstnodetmp);
		xx_free_mem(poll , pstnodetmp , sizeof(CSPACKAGE_NODE));
	}

	memset(pstconnect , 0 , sizeof(client_connect));

	/*更改激活的fd*/
	close(client_fd);

	for(i=0; i<client.max_active_fd; i++){
		if(client.active_fds[i] == client_fd)
			break;
	}

	for(; i<=client.max_active_fd - 1 - 1; i++){	/*激活数组前移*/
		client.active_fds[i] = client.active_fds[i + 1];
	}
	client.active_fds[i] = 0;	/*设置数组最后一个为0*/

	client.max_active_fd--;

	return 0;
}


/*
 * 尽可能多地读取与connect_server链接的所有bus数据，并尽可能多地将取得的包发送到对应的客户端；
 * 如果客户端socket写缓冲区满则将该包放入client该socket的发送队列
 * 一个管道最多读10个包然后换管道
 */
static int try_handle_buses(void){
	sspackage_t sspackage;

	bus_interface *bus = NULL;
	CSPACKAGE_NODE *pstnode = NULL;

	int icount = 0;	/*记录成功发送的包数目*/
	int isend_bytes = 0;	/*发送的字节数目*/
	int iret;

	/*获得connect与logic的bus包*/
	while(1){
		/*一次不能超过10个包*/
		if(icount > 10){
			break;
		}

		/*读BUS*/
		iret = get_bus_pkg(connect_server_id , logic_server_id , &sspackage);
		if(iret == -1){	/*读BUS出错*/
			write_log(LOG_ERR , "connect server: try handle buses: bus_to_logic , read bus failed!");
			break;
		}
		if(iret == -2){	/*BUS空*/
			break;
		}

		/*发包*/
		isend_bytes = write_socket(sspackage.sshead.client_fd , &sspackage.cs_data);
		if(isend_bytes != sizeof(cspackage_t)){		/*如果发送失败或者发送字节小于一个包长则将其放入发送队列中*/
//			pstnode = (CSPACKAGE_NODE *)malloc(sizeof(CSPACKAGE));
			pstnode = (CSPACKAGE_NODE *)xx_alloc_mem(poll , sizeof(cspackage_t));

			memcpy(&pstnode->cs_data , &sspackage.cs_data , sizeof(cspackage_t));
			pstnode->node_next = NULL;
			client.client_connects[sspackage.sshead.client_fd].send_tail->node_next = pstnode;
			client.client_connects[sspackage.sshead.client_fd].send_count++;

			break;
		}

		/*发送成功，继续取包*/
		icount++;
	}

	return 0;
}

static int send_to_all_clients(void){
	int i;

	for(i=0; i<client.max_active_fd; i++){
		send_to_client(client.active_fds[i]);
	}

	return 0;

}

/*
 * 处理信号函数
 * relay local var: bus_to_logic
 */
static void handle_signal(int sig_no){
	int iret;

	switch(sig_no){
		case SIGINT:
			write_log(LOG_NOTIFY , "receive SIGINT,ready to exit...");
			/*脱离BUS*/
			iret = close_bus(connect_server_id , logic_server_id);
			if(iret < 0){
				write_log(LOG_ERR , "connect server: detach bus to connect failed!");
			}
			write_log(LOG_NOTIFY , "connect server: detach bus to connect success!");
			/*销毁内存池*/
			iret = delete_mem_poll(poll);
			if(iret < 0){
				write_log(LOG_ERR , "connect server: delete_mem_poll failed!");
			}
			write_log(LOG_NOTIFY , "connect server: delete_mem_poll success!");
			exit(0);
		break;

		default:
		break;
	}
}


