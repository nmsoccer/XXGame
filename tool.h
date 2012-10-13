/*
 * tool.h
 *
 *  Created on: 2012-10-2
 *      Author: leiming
 */

#ifndef TOOL_H_
#define TOOL_H_


/*
 * 出错记录
 */
int log_error(char *str);

/*
 * 将文件设置非阻塞
 */
int set_nonblock(int fd);


#endif /* TOOL_H_ */
