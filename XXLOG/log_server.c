/*
 * log_server.c
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <error.h>


#include "common.h"
#include "tool.h"
#include "mytypes.h"
#include "mempoll.h"


int main(int argc , char **argv){
	char dir_path[DIR_PATH_LEN] = {'\0'};
	char buff[512];
	FILE *file_setting;
	time_t tp;
	struct tm *tm;
	int ret;

	/*创建本地日志文件夹*/
	tp = time(0);
	tm = localtime(&tp);
	snprintf(dir_path , sizeof(dir_path) , "./LOG/%d/%d/" , tm->tm_year + 1900 , tm->tm_mon + 1);
	ret = create_dir(dir_path , MODE_RDWR_FILE);
	if(ret == 0){
		write_log(LOG_INFO , "log server:create %s of log server success!\n" , dir_path);
	}else{
		write_log(LOG_DUMP , "log server:create %s of log server failed!\n" , dir_path);
	}

	/*open setting*/
	file_setting = fopen("./log_setting" , "r");
	if(!file_setting){
		printf("open log_\n");
		return -1;
	}

	/*read setting and create dir*/
	while(1){
		memset(dir_path , 0 , sizeof(dir_path));
		if(fgets(dir_path , sizeof(dir_path) , file_setting) == NULL){
			break;
		}
		if(dir_path[0] != '+'){	/*有效行必须是以+开头*/
			continue;
		}

		/*为每一行配置创建日志目录*/
		dir_path[strlen(dir_path) - 1] = 0;		/*消除换行符*/

		memset(buff , 0 , sizeof(buff));
		snprintf(buff , sizeof(buff) , "%d/%d/" , tm->tm_year + 1900 , tm->tm_mon + 1);	/*根据当前日期创建日志目录*/
		strncat(dir_path , buff , sizeof(dir_path) - strlen(dir_path));	/*组合起来获得完整路径*/
//		printf("dir_path: %s\n" , dir_path);

		ret = create_dir(&dir_path[1] , MODE_RDWR_FILE);	/*创建目录*/
		do{
			if(ret == 0){
				write_log(LOG_INFO , "log server:create %s success!\n" , &dir_path[1]);
				break;
			}
			if(ret == -1){
				write_log(LOG_ERR , "log server:create %s failed! Please Check!\n" , &dir_path[1]);
				break;
			}
			if(ret == -2){
				write_log(LOG_ERR , "log server:create %s failed! Access Denied\n" , &dir_path[1]);
				break;
			}
		}while(0);

	}

	fclose(file_setting);
	return 0;

}
