/*
 * XXClient.c
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#include  <unistd.h>
#include  <sys/types.h>       /* basic system data types */
#include  <sys/socket.h>      /* basic socket definitions */
#include  <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>       /* inet(3) functions */
#include <netdb.h> /*gethostbyname function */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include "common.h"
#include "mempoll.h"




static int validate_player(int connfd);
static int print_menu(void);
static int handle_move(int connfd);
void handle(int connfd , char *str);




int main(int argc, char **argv)
{
    char * servInetAddr = "127.0.0.1";
    int servPort = 8005;
    int connfd;
    struct sockaddr_in servaddr;
    xxmem_poll *poll = NULL;
    char *test;
    char time_str[1024] = {'\0'};
    char str_array[10][128] = {0};
    char i;
    int count;

    if (argc < 2) {
        printf("argc < 2!\n");
        return -1;
    }

    connfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(servPort);
    inet_pton(AF_INET, servInetAddr, &servaddr.sin_addr);

    if (connect(connfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        return -1;
    }
    printf("welcome to echoclient\n");

    handle(connfd , argv[1]);     /* do it all */

    close(connfd);
    printf("exit\n");
    exit(0);
}

void handle(int sockfd , char *str)
{
	int iret;
    int n;
    cspackage_t package;
    int option;

    validate_player(sockfd);

    while(1){
    	/*validate player info*/
    	option = print_menu();
    	handle_move(sockfd);




    }











/*
    for (;;) {
    	memset(&stcspackage , 0 , sizeof(CSPACKAGE));
    	fgets(stcspackage.data.acdata , CS_DATA_LEN , stdin);
    	stcspackage.cshead.proto_type = XX_PROTO_TEST;
         if(strcmp(stcspackage.data.acdata , "exit\n") == 0){
        	 break;
         }

        n = write(sockfd, &stcspackage , sizeof(CSPACKAGE));

    	memset(&stcspackage , 0 , sizeof(CSPACKAGE));
        n = read(sockfd, &stcspackage , sizeof(CSPACKAGE));
        if (n == 0) {
            printf("echoclient: server terminated prematurely\n");
            break;
        }
        printf("from server: prototype: %d content:%s\n" , stcspackage.cshead.proto_type , stcspackage.data.acdata);
        //如果用标准库的缓存流输出有时会出现问题
        //fputs(recvline, stdout);
    }*/
}


static int validate_player(int connfd){
	cspackage_t cspackage;
	validate_info_t *info;
	int i = 0;

	info = &cspackage.data.validate_player;
	while(1){
		cspackage.cshead.proto_type = CS_PROTO_VALIDATE_PLAYER;
		memset(info , 0 , sizeof(validate_info_t));

		printf("WELCOME TO XX GAME WORLD!\n");
		printf(">>>>Please input your name soldier: ");
		fgets(info->player_name , PLAYER_NAME_LEN , stdin);
		printf(">>>>your passwd: ");
		fgets(info->player_passwd , PLAYER_PASSWD_LEN , stdin);

		info->player_name[strlen(info->player_name) - 1] = 0;	/*delete \n*/
		info->player_passwd[strlen(info->player_passwd) - 1] = 0;

		/*send*/
		printf("your name is:%s , passwd is:%s\n" , info->player_name , info->player_passwd);
		write(connfd, &cspackage , sizeof(cspackage_t));

		/*read*/
    	memset(&cspackage , 0 , sizeof(cspackage_t));
        read(connfd, &cspackage , sizeof(cspackage_t));

        /*check*/
        if(cspackage.data.validate_player.is_validate == IS_VALIDATE){	/*验证通过*/
        	break;
        }

	}	/*end while*/

	return 0;
}

static int print_menu(void){
	int option;

	printf("Please choose function: \n");
	printf("1. Move\n");
	printf("2.Fight\n");
	printf("your input: ");
	scanf("%d" , &option);

	return option;
}

static int handle_move(int connfd){
	cspackage_t cspackage;

	printf("please input: ");
	memset(&cspackage , 0 , sizeof(cspackage_t));
	fgets(cspackage.data.acdata , CS_DATA_LEN , stdin);
	cspackage.cshead.proto_type = CS_PROTO_TEST;
     if(strcmp(cspackage.data.acdata , "exit\n") == 0){
    	 exit(0);
     }

    write(connfd, &cspackage , sizeof(cspackage_t));

	memset(&cspackage , 0 , sizeof(cspackage_t));
    read(connfd, &cspackage , sizeof(cspackage_t));

    printf("from server: prototype: %d content:%s\n" , cspackage.cshead.proto_type , cspackage.data.acdata);
    return 0;
}

