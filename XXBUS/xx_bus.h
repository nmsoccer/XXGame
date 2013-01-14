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
	sspackage_t data[CHANNEL_MAX_PACKAGE];
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
 * 打开BUS，准备通信
 * 要依靠BUS收发包必须要打开BUS
 * @param proc1: 利用BUS通信的一个进程标志
 * @parm proc2: 利用BUS通信的另一个进程标志
 * @return:成功返回0；失败返回-1
 */
extern int open_bus(proc_t proc1 , proc_t proc2);

/*
 * 关闭BUS
 * 打开BUS收发包之后，如果不想再使用，则调用该函数关闭
 * @param proc1: 利用BUS通信的一个进程标志
 * @parm proc2: 利用BUS通信的另一个进程标志
 * @return:成功返回0；失败返回-1
 */
extern int close_bus(proc_t proc1 , proc_t proc2);


/*
 * 通过BUS发送包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @param package: 发送包缓冲区
 * @return:
 * 0:发送成功；
 * -1：出现错误
 * -2: BUS满
 */
extern int send_bus_pkg(proc_t recv_proc , proc_t send_proc , sspackage_t *package);

/*
 * 通过BUS接收包
 * @param recv_proc:接收进程标志
 * @param send_proc: 发送进程标志
 * @param package: 接收包缓冲区
 * @return:
 * 0:接收成功；
 * -1：出现错误
 * -2: BUS空
 */
extern int get_bus_pkg(proc_t recv_proc , proc_t send_proc , sspackage_t *package);


/*
 * 链接共享资源地址
 * @param res_type:资源类型
 * @world_id:世界ID
 * @line_id:线ID
 * @return:
 * 失败:NULL
 * 成功:资源地址
 */
extern void *attach_shm_res(int res_type , int world_id , int line_id);

/*
 * 剥离该进程具有的共享资源
 * @param res_type:资源类型
 * @world_id:世界ID
 * @line_id:线ID
 * @return:
 * 失败:-1
 * 成功:0
 */
extern int detach_shm_res(int res_type , int world_id , int line_id);


#endif /* XX_BUS_H_ */
