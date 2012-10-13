/*
 * mempoll.h
 *
 *  Created on: 2012-10-9
 *      Author: leiming
 */

#ifndef MEMPOLL_H_
#define MEMPOLL_H_

/*创建内存池*/


/*获取size内存大小的函数*/
extern void *xx_alloc_mem(int size);

/*回收内存*/
extern int xx_free_mem(void *ptr);

#endif /* MEMPOLL_H_ */
