/*
 * xx_bus.h
 *
 *  Created on: 2012-10-7
 *      Author: leiming
 */

#ifndef XX_BUS_H_
#define XX_BUS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../common.h"

////////////////////////////////MACRO DEFINE////////////////////////////////
#define CHANNEL_MAX_PACKAGE 40	/*bus单通道的包容量*/

#define BUS_MODE_FLAG	S_IRWXU | S_IRWXG | S_IRWXO

#define PATH_NAME "/etc/passwd"

//////////////////////////////////DATA STRUCTURE//////////////////////////////
/*
 * BUS_CHANNEL
 * 每个BUS两条CHANNEL分别用于两个进程接收。
 * 如果CHANNEL A用于A进程接收则表示对于B是发送管道；
 * 同理CHANNEL B用于B进程接收则表示对于A是发送管道；
 */
struct _bus_channel{
	SSPACKAGE data[CHANNEL_MAX_PACKAGE];
	u8 package_num;	/*包的数量*/
	u8 head;		/*队首地址，用于收包*/
	u8 tail;			/*队尾地址，用于发包*/
	spin_lock_t spin_lock;	/*该channel的自旋锁*/
};

typedef struct _bus_channel bus_channel;

/*
 * BUS INTERFACE
 * 每个bus端的接口
 */
struct stbus_interface{
	u32 udwproc_id_recv_ch1;	/*从通道1收包的进程ID(通道2发包)*/
	bus_channel channel_one;

	u32 udwproc_id_recv_ch2;	/*从通道2收包的进程ID(通道1发包)*/
	bus_channel channel_two;
};

typedef struct stbus_interface bus_interface;

///////////////////////////FUNCTION//////////////////////////////////////
/*
 * 链接上BUS
 * @proc1: 利用BUS通信的一个进程标志
 * @proc2:利用BUS通信的另一个进程标志
 * @return:成功返回该BUS接口；失败返回NULL
 */
extern bus_interface *attach_bus(u8 proc1 , u8 proc2);

/*
 * 脱离BUS
 * @bus: bus的接口
 * @return:成功返回0；失败返回-1
 */
extern int detach_bus(bus_interface *bus);


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
extern int send_bus(u8 recv_proc , u8 send_proc , bus_interface *bus , SSPACKAGE *package);


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
extern int recv_bus(u8 recv_proc , u8 send_proc , bus_interface *bus , SSPACKAGE *package);

#endif /* XX_BUS_H_ */
