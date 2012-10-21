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
#define MAX_CONNECT_CLIENTS 1024	/*进程能够支持打开的最多文件描述符*/

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
	CSPACKAGE cs_data;
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
/*链接到logic_server的bus*/
static bus_interface *bus_to_logic = NULL;

/*connect_server的ID*/
static u8 connect_server_id;
/*logic_server的ID*/
static u8 logic_server_id;
/*内存池*/
static xxmem_poll *poll;
//////////////////////////////////////////////FUNCTION/////////////////////////////////////
/*
 * 读取数据
 */
static int read_socket(int isock_fd , CSPACKAGE *pstcspackage);

/*
 * 发送数据
 */
static int write_socket(int isock_fd , CSPACKAGE *pstcspackage);

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

	struct sockaddr_in stserv_addr;
	struct sockaddr_in stcli_addr;
	int iaddr_len = 0;

	struct epoll_event stepoll_event;
	struct epoll_event astepoll_events[MAX_CONNECT_CLIENTS + 1];	/*一个FD对应一个event*/

	CSPACKAGE stcspackage_recv;	/*收发客户端的包*/
	CSPACKAGE stcspackage_send;

	SSPACKAGE sspackage_recv;	/*收发其他服务器进程的包*/
	SSPACKAGE sspackage_send;

//	CSPACKAGE_NODE *pstcspackage_node = NULL;

	struct sigaction stsig_act;		/*信号动作变量*/

//	pthread_t tid = 0;	/*开启接口线程的线程ID*/
	int iRet = -1;
	int i;

	u32 ticks = 0;	/*时间滴答*/

	/*检测参数*/
	if(argc < 2){
		printf("argc < 2 , please input more information like: connect_server CONNECT1\n");
		return -1;
	}

	/*Create sockfd*/
	iserv_fd = socket(PF_INET , SOCK_STREAM , 0 );
	if(iserv_fd < 0){
		log_error("Create connect server socket failed!");
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
		log_error("Bind connect socket failed!");
		return -1;
	}

	/*Create epoll fd and set it*/
	iepoll_fd = epoll_create(MAX_CONNECT_CLIENTS + 1);	/*包括iserv_fd*/
	if(iepoll_fd < 0){
		log_error("epoll create failed!");
		return -1;
	}

	stepoll_event.events = EPOLLIN;	/*add serve fd to epoll*/
	stepoll_event.data.fd = iserv_fd;
	iRet = epoll_ctl(iepoll_fd , EPOLL_CTL_ADD , iserv_fd , &stepoll_event);
	if(iRet < 0){
		log_error("append serve fd to epoll failed!");
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
		log_error("crate mem poll failed!");
		return -1;
	}

	/*链接BUS*/
	if(strcmp(argv[1] , "CONNECT1") == 0){	/*connect_server1*/
		bus_to_logic = attach_bus(GAME_CONNECT_SERVER1 , GAME_LOGIC_SERVER1);

		if(!bus_to_logic){
			log_error("connect_server1: attach to logic failed!");
			return -1;
		}

		connect_server_id =GAME_CONNECT_SERVER1;
		logic_server_id = GAME_LOGIC_SERVER1;
		printf("attach: %x %d vs %d\n" , bus_to_logic , bus_to_logic->udwproc_id_recv_ch1 , bus_to_logic->udwproc_id_recv_ch2);

//		memcpy(bus_to_logic->channel_two.start_addr , "helloworld" , 9);
//		printf("it is:%s\n" , bus_to_logic->channel_two.start_addr);

	}else{
		log_error("connect server1: illegal param1:");
		log_error(argv[1]);
		return -1;
	}

	/*主逻辑*/
	while(1){
		ticks++;		/*循环一次加一个滴答*/

		iactive_fds = epoll_wait(iepoll_fd , astepoll_events , MAX_CONNECT_CLIENTS+1 , 200);
		if(iactive_fds < 0){
			log_error("epoll wait error!");
			return -1;
		}

		/*检测每个FD*/
		for(i=0; i<iactive_fds; i++){
			/*监听FD*/
			if(astepoll_events[i].data.fd == iserv_fd){	/*如果是监听socket，则accept*/
				memset(&stcli_addr , 0 , sizeof(stcli_addr));
				iaccept_fd = accept(iserv_fd , (struct sockaddr *)&stcli_addr , &iaddr_len);

				printf("-----------accept a new client: %d\n" , iaccept_fd);
				/*设置接收的socket属性*/
				set_nonblock(iaccept_fd);	/*设置为非阻塞*/
				set_sock_buff_size(iaccept_fd , 10 * sizeof(CSPACKAGE) , 5 * sizeof(CSPACKAGE));	/*设置socket缓冲区大小*/

				/*将该socket加入client*/
				client.active_fds[client.max_active_fd] = iaccept_fd;
				client.max_active_fd++;

				/*将该socket设置入epoll监视*/
				stepoll_event.data.fd = iaccept_fd;
				stepoll_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				iRet = epoll_ctl(iepoll_fd ,  EPOLL_CTL_ADD , iaccept_fd , &stepoll_event);
				if(iRet < 0){
					log_error("accept failed!");
				}
				continue;
			}

			/*断开链接*/
			if(astepoll_events[i].events & EPOLLRDHUP){
				PRINT("client exit!");
				close_client(astepoll_events[i].data.fd);
				continue;
			}

			/*可以读*/
			if(astepoll_events[i].events & EPOLLIN){
				handle_fd = astepoll_events[i].data.fd;
				/*read client*/
				memset(&sspackage_recv , 0 , sizeof(SSPACKAGE));
				sspackage_recv.sshead.client_fd = handle_fd;
				iRet = read_socket(handle_fd , &sspackage_recv.cs_data);

				if(iRet == sizeof(CSPACKAGE)){
					PRINT(">>>read package!");
				}else
				if(iRet > 0){	/*读的数据小于一个包长*/
					log_error("connect server: read bytes < CSPACKAGE!");
				}else{	/*没有数据*/
					log_error("connect server: nothing to read!");
				}

				/*将读取到的包通过BUS发送给logic*/
				iRet = send_bus(logic_server_id , connect_server_id , bus_to_logic , &sspackage_recv);/*BUS发送*/
				if(iRet == 0){	/*发送成功*/
					PRINT("connect server: send bus success!");
				}else
				if(iRet == -2){	/*BUS满*/
					log_error("connect server: bus to logic full!");
				}else
				if(iRet == -1){	/*发送失败*/
					log_error("connect server: send to logic failed!");
				}

				/*同时检测该fd是否有需要发的包*/
				send_to_client(handle_fd);
			}

		}	/*end for*/

		/*在处理了socket 接口之后的其他操作*/
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
static int read_socket(int isock_fd , CSPACKAGE *pstcspackage){
	if(isock_fd < 0  || !pstcspackage){
		return -1;
	}

	return recv(isock_fd , pstcspackage , sizeof(CSPACKAGE) , 0);
}

/*
 * 发送数据
 * @return: > 0发送的数据长度
 * -1:发送失败
 */
static int write_socket(int isock_fd , CSPACKAGE *pstcspackage){
	if(isock_fd < 0  || !pstcspackage){
		return -1;
	}

	return send(isock_fd , pstcspackage, sizeof(CSPACKAGE) , 0);
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

//	printf("there are %d packages in send_queue" , pstconnect->send_count);

	while(1){
		if(pstnode == NULL){
			break;
		}

		iRet = write_socket(client_fd , &pstnode->cs_data);
		if(iRet == -1 || iRet != sizeof(CSPACKAGE)){	/*如果发送失败或者发送数据小于一个包长，则退出待以后重新发送*/
			break;
		}

		pstconnect->send_count--;
		pstnodetmp = pstnode;
		pstnode = pstnode->node_next;
//		free(pstnodetmp);
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
	SSPACKAGE sspackage;

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
		iret = recv_bus(connect_server_id , logic_server_id , bus_to_logic , &sspackage);
		if(iret == -1){	/*读BUS出错*/
			log_error("connect server: try handle buses: bus_to_logic , read bus failed!");
			break;
		}
		if(iret == -2){	/*包空*/
			break;
		}

		/*发包*/
		isend_bytes = write_socket(sspackage.sshead.client_fd , &sspackage.cs_data);
		if(isend_bytes != sizeof(CSPACKAGE)){		/*如果发送失败或者发送字节小于一个包长则将其放入发送队列中*/
//			pstnode = (CSPACKAGE_NODE *)malloc(sizeof(CSPACKAGE));
			pstnode = (CSPACKAGE_NODE *)xx_alloc_mem(poll , sizeof(CSPACKAGE));

			memcpy(&pstnode->cs_data , &sspackage.cs_data , sizeof(CSPACKAGE));
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
	switch(sig_no){
		case SIGINT:
			PRINT("receive SIGINT,ready to exit...");
			if(bus_to_logic){
				detach_bus(bus_to_logic);	/*脱离BUS*/
			}
			delete_mem_poll(poll);
			exit(0);
		break;

		default:
		break;
	}
}


