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
 * 服务器包队列节点。用于保存将要发送给客户端的数据包.关联到每一个客户端SOCK FD
 */
struct stsspackage_node{
	struct stsspackage_node *pstnode_next;
	SSPACKAGE stss_data;
};

typedef struct stsspackage_node SSPACKAGE_NODE;

/*
 * 一个客户端的链接结构
 */
typedef struct{
	u8 ucnode_count;
	SSPACKAGE_NODE stnode_head;
}client_connect;


///////////////////////////////////////////////DATA////////////////////////////////////////////
/*
 * 每个socket fd 对应一个成员。成员在数组的位置就是其fd值
 */
static client_connect client_connects[MAX_CONNECT_CLIENTS];

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
 * 处理信号函数
 */
static void handle_signal(int sig_no);


//////////////////////////////////////////////STATIC VAR///////////////////////////////////////
static bus_interface *bus_to_logic = NULL;	/*链接到logic_server的bus*/

////////////////////////////////////////////DEFINE//////////////////////////////////////////////
/*
 * MAIN
 */
int main(int argc , char **argv){
	int iserv_fd;	/*服务器端创建监听的套接字*/
	int iaccept_fd;	/*链接具体客户端的TCP套接字*/
	int iepoll_fd;	/*epoll的文件描述符*/
	int iactive_fds;	/*通过epoll活跃的fd数目*/

	struct sockaddr_in stserv_addr;
	struct sockaddr_in stcli_addr;
	int iaddr_len = 0;

	struct epoll_event stepoll_event;
	struct epoll_event astepoll_events[MAX_CONNECT_CLIENTS + 1];	/*一个FD对应一个event*/

	CSPACKAGE stcspackage_recv;
	CSPACKAGE stcspackage_send;

	struct sigaction stsig_act;		/*信号动作变量*/

	pthread_t tid = 0;	/*开启接口线程的线程ID*/
	int iRet = -1;
	int i;

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

	/*DEAL MISC*/
	for(i=0; i<MAX_CONNECT_CLIENTS; i++){
		client_connects[i].ucnode_count = 0;
	}

	/*处理信号*/
	memset(&stsig_act , 0 , sizeof(stsig_act));
	stsig_act.sa_handler = handle_signal;
	sigaction(SIGINT , &stsig_act , NULL);

	/*链接BUS*/
	if(strcmp(argv[1] , "CONNECT1") == 0){	/*connect_server1*/
		bus_to_logic = attach_bus(GAME_CONNECT_SERVER1 , GAME_LOGIC_SERVER1);

		if(!bus_to_logic){
			log_error("attach to logic failed!\n");
			return -1;
		}
		printf("attach: %x %d vs %d\n" , bus_to_logic , bus_to_logic->udwproc_id_recv_ch1 , bus_to_logic->udwproc_id_recv_ch2);
	}else{
		printf("illegal param1:%s\n");
		return -1;
	}

	/*主逻辑*/
	while(1){
		iactive_fds = epoll_wait(iepoll_fd , astepoll_events , MAX_CONNECT_CLIENTS+1 , 2000);
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

				printf("{}accept a new client: %d\n" , iaccept_fd);
				set_nonblock(iaccept_fd);	/*设置为非阻塞*/
				stepoll_event.data.fd = iaccept_fd;
				stepoll_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
				iRet = epoll_ctl(iepoll_fd ,  EPOLL_CTL_ADD , iaccept_fd , &stepoll_event);
				if(iRet < 0){
					log_error("accept failed!\n");
				}
				continue;
			}

			/*断开链接*/
			if(astepoll_events[i].events & EPOLLRDHUP){
				printf("client exit!\n");
				close(astepoll_events[i].data.fd);
				continue;
			}

			/*可以读*/
			if(astepoll_events[i].events & EPOLLIN){
				/*read*/
				memset(&stcspackage_recv , 0 , sizeof(CSPACKAGE));
				read_socket(astepoll_events[i].data.fd , &stcspackage_recv);

				memset(&stcspackage_send , 0 , sizeof(CSPACKAGE));
				strcpy(stcspackage_send.data.acdata , "GOOD MAN");
				write_socket(astepoll_events[i].data.fd , &stcspackage_send);
			}

			/*可以写*/
			if(astepoll_events[i].events & EPOLLOUT){
				/*write*/
			}

		}	/*end for*/

		/*在处理了socket 接口之后的其他操作*/
		printf("nice!\n");

	}	/*end while*/


//	pthread_join(tid , NULL);
	return 0;
}


/*
 * 读取数据
 */
static int read_socket(int isock_fd , CSPACKAGE *pstcspackage){
	int inumber;

	if(isock_fd < 0  || !pstcspackage){
		return -1;
	}

	inumber = recv(isock_fd , pstcspackage , sizeof(CSPACKAGE) , 0);
//	printf(">>>read %d  bytes ", inumber);
	if(pstcspackage->uwproto_type == XX_PROTO_VALIDATE){
		printf(">>>>%d read: prototype: %d name: %s\n" , isock_fd , pstcspackage->uwproto_type , pstcspackage->data.stplayer_info.szname );
	}else{
		printf(">>>>%d read: prototype: %d content: %s" , isock_fd , pstcspackage->uwproto_type , pstcspackage->data.acdata );
	}


	if(inumber == -1){
	}
	return 0;
}

/*
 * 发送数据
 */
static int write_socket(int isock_fd , CSPACKAGE *pstcspackage){
	int inumber;

	if(isock_fd < 0  || !pstcspackage){
		return -1;
	}

	inumber = send(isock_fd , pstcspackage, sizeof(CSPACKAGE) , 0);
	printf("write %s packagelen:%d  \n\n:", pstcspackage->data.acdata , inumber);
	return 0;
}

/*
 * 处理信号函数
 */
static void handle_signal(int sig_no){
	switch(sig_no){
		case SIGINT:
			printf("receive SIGINT,ready to exit...\n");
			if(bus_to_logic){
				detach_bus(bus_to_logic);	/*脱离BUS*/
			}
			exit(0);
		break;

		default:
		break;
	}
}


