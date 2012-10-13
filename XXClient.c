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

#include "common.h"

void handle(int connfd , char *str);

int main(int argc, char **argv)
{
    char * servInetAddr = "127.0.0.1";
    int servPort = 8005;
    int connfd;
    struct sockaddr_in servaddr;
    CSPACKAGE stcspackage;


    if (argc < 2) {
        printf("argc < 2!\n");
        return -1;
    }
/*
    if (argc == 3) {
        servInetAddr = argv[1];
        servPort = atoi(argv[2]);
    }
    if (argc > 3) {
        printf("usage: echoclient <IPaddress> <Port>\n");
        return -1;
    }
*/

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
    int n;
    CSPACKAGE stcspackage;

    memset(&stcspackage , 0 , sizeof(CSPACKAGE));	/*发送验证包*/
    stcspackage.uwproto_type = XX_PROTO_VALIDATE;
    strcpy(stcspackage.data.stplayer_info.szname , str);
    write(sockfd , &stcspackage , sizeof(CSPACKAGE));

    memset(&stcspackage , 0 , sizeof(CSPACKAGE));	/*读取信息*/
    read(sockfd, &stcspackage , sizeof(CSPACKAGE));
    printf("from server: prototype: %d content:%s\n" , stcspackage.uwproto_type , stcspackage.data.acdata);

    for (;;) {
    	memset(&stcspackage , 0 , sizeof(CSPACKAGE));
    	fgets(stcspackage.data.acdata , CS_DATA_LEN , stdin);
    	stcspackage.uwproto_type = XX_PROTO_TEST;
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
        printf("from server: prototype: %d content:%s\n" , stcspackage.uwproto_type , stcspackage.data.acdata);
        //如果用标准库的缓存流输出有时会出现问题
        //fputs(recvline, stdout);
    }
}
