/*
 * mempoll.h
 *
 *  Created on: 2012-10-9
 *      Author: leiming
 */

#ifndef MEMPOLL_H_
#define MEMPOLL_H_


#define ALLOC_BASE_SIZE 1024		/*分配内存的最基本大小为1K*/
#define MAX_MEM_INDEX	5	/*最大的单块为5K  (MAX_INDEX * ALLOC_BASE_SIZE)*/
//////////////////////////////DATA STRUCT/////////////////////////////////////

/*基本的分配节点*/
struct _xxmem_node{
	struct _xxmem_node *next;	/*下一个节点*/
	int real_size;	/*实际分配的内存大小，用于校验*/
	void *data;	/*分配给该节点的内存块。大小为ALLOC_BASE_SIZE的整数倍。应>=real_szie*/
};

typedef struct _xxmem_node xxmem_node;


/*每个相同大小区域内存的管理头，用于管理node链表*/
struct _xxmem_index{
	int index;	/*该头的index，用于校验*/

	int used_count;	/*使用的数量*/
	xxmem_node *used;	/*使用的节点链表*/

	int free_count;	/*空闲的数量*/
	xxmem_node *free;	/*空闲的节点链表*/
};

typedef struct _xxmem_index xxmem_index;

/*内存池。每个index通过分配内存块的大小定位*/
struct _xxmem_poll{
	xxmem_index all_index[MAX_MEM_INDEX + 1];	/*all_index[0]不使用 使用1～MAX_INDEX*/
};

typedef struct _xxmem_poll xxmem_poll;


//////////////////////////////////FUNC DELCARE///////////////////////////
/*创建内存池
 * 失败返回NULL
*/
extern xxmem_poll *create_mem_poll(void);

/*销毁内存池
 * @param poll: 内存池指针
 * @return: 0 成功
 * -1：失败
*/
extern int delete_mem_poll(xxmem_poll *poll);

/*获取size内存大小的函数
 * @param poll:内存池指针
 * @param size: 试图分配的内存大小
 * @return: 成功返回内存区指针
 * 失败返回NULL
 */
extern void *xx_alloc_mem(xxmem_poll *poll , int size);

/*回收内存
 * @param poll:内存池
 * @param ptr: 回收的内存区域
 * @param size: 回收的内存区域大小
 * @return: 成功返回0 失败返回-1
 */
extern int xx_free_mem(xxmem_poll *poll , void *ptr , int size);

#endif /* MEMPOLL_H_ */
