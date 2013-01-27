/*
 * mytypes.h
 *
 *  Created on: 2012-10-4
 *      Author: leiming
 */

#ifndef MYTYPES_H_
#define MYTYPES_H_

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long ul32;
typedef unsigned long long u64;
//typedef unsigned double u64;
typedef unsigned int proc_t;

typedef unsigned char spin_lock_t;	/*自旋锁*/

struct _rdwr_lock{	/*读写锁*/
	volatile u8 read_lock;	/*读锁*/
	volatile u8 write_lock;	/*写锁*/
	u8 rd_lock_count;	/*读锁数量*/
};
typedef struct _rdwr_lock rdwr_lock_t;

#endif /* MYTYPES_H_ */
