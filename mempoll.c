/*
 * mempoll.c
 *
 *  Created on: 2012-10-9
 *      Author: leiming
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mempoll.h"

/*创建内存池
 * 失败返回NULL
*/
xxmem_poll *create_mem_poll(void){
	xxmem_poll *p;
	int i;
	/*只malloc内存池头部管理部分。具体的内存node当申请时再malloc且不立即释放，等销毁内存池时才统一释放*/
	p = (xxmem_poll *)malloc(sizeof(xxmem_poll));
	memset(p , 0 , sizeof(xxmem_poll));

	for(i=1; i<= MAX_MEM_INDEX; i++){
		p->all_index[i].index = i;
	}

	printf("create mem poll success!\n");
	return p;
}

/*销毁内存池
 * @param poll: 内存池指针
 * @return: 0 成功
 * -1：失败
*/
int delete_mem_poll(xxmem_poll *poll){
	xxmem_node *now = NULL;
	xxmem_node *tmp = NULL;
	int i;

	if(!poll){
		return -1;
	}

	/*依次删除node 队列*/
	for(i=1; i<=MAX_MEM_INDEX; i++){
		if(poll->all_index[i].index == 0){	/*该index为空*/
			continue;
		}
		if(poll->all_index[i].index != i){	/*校验失败*/
			return -1;
		}

		/*删除used node*/
		printf("delete: index %d: there are %d used blocks\n" , i , poll->all_index[i].used_count);
		now = poll->all_index[i].used;
		while(1){
			if(!now){
				break;
			}

			tmp = now->next;
			free(now->data);
			free(now);
			now = tmp;
		}

		/*删除free node*/
		printf("detle: index %d: there are %d free blocks\n" , i , poll->all_index[i].free_count);
		now = poll->all_index[i].free;
		while(1){
			if(!now){
				break;
			}

			tmp = now->next;
			free(now->data);
			free(now);
			now = tmp;
		}

	}	/*end for*/

	/*删除POLL*/
	free(poll);
	printf("delete mem poll success!\n");
	return 0;
}

/*获取size内存大小的函数
 * @param poll:内存池指针
 * @param size: 试图分配的内存大小
 * @return: 成功返回内存区指针
 * 失败返回NULL
 */
void *xx_alloc_mem(xxmem_poll *poll , int size){
	xxmem_node *now;
	int index = 0;

	if(!poll || size<=0){
		return NULL;
	}

	/*index根据size来决定*/
	if(size <= ALLOC_BASE_SIZE){	/*如果size不足ALLOC_BASE_SIZE*/
		index = 1;
	}else{	/*如果size >= index * ALLOC_BASE_SZE*/
		index = size / ALLOC_BASE_SIZE;
		if(size % ALLOC_BASE_SIZE != 0){	/*如果size > index * ALLOC_BASE_SZE*/
			index++;
		}
		/*size == index * ALLOC_BASE_SIZE*/
	}

	/*如果index超出了MAX_MEM_INDEX*/
	if(index > MAX_MEM_INDEX){
		return NULL;
	}

	printf("before alloc index %d there are %d free %d used\n" , index , poll->all_index[index].free_count , poll->all_index[index].used_count);
	/*如果没有空闲块则新分配*/
	if(poll->all_index[index].free_count == 0){
		printf("malloc new size: %d\n" , index * ALLOC_BASE_SIZE);

		now = (xxmem_node *)malloc(sizeof(xxmem_node));
		now->real_size = size;
		now->data = malloc(index * ALLOC_BASE_SIZE);	/*分配的内存块始终>=size且是ALLOC_BASE_SIZE的整数倍*/


		now->next = poll->all_index[index].used;	/*将其插入使用队列头部*/
		poll->all_index[index].used = now;
		poll->all_index[index].used_count++;

//		printf("after alloc index %d real size: %d there are %d free %d used\n" , index , now->real_size , poll->all_index[index].free_count , poll->all_index[index].used_count);
		return now->data;
	}

	printf("use free block! \n ");
	/*如果有空闲块则取出第一块*/
	now = poll->all_index[index].free;
	now->real_size = size;
	poll->all_index[index].free = now->next;
	poll->all_index[index].free_count--;

	now->next = poll->all_index[index].used;	/*将取出的添加到已经使用的队列中*/
	poll->all_index[index].used = now;
	poll->all_index[index].used_count++;

//	printf("after alloc index %d there are %d free %d used\n" , index , poll->all_index[index].free_count , poll->all_index[index].used_count);
	return now->data;
}

/*回收内存
 * @param poll:内存池
 * @param ptr: 回收的内存区域
 * @param size: 回收的内存区域大小
 * @return: 成功返回0 失败返回-1
 */
int xx_free_mem(xxmem_poll *poll , void *ptr , int size){
	xxmem_node *now = NULL;
	xxmem_node *prev = NULL;
	int index = 0;

	if(!poll || !ptr || size<=0){
		return -1;
	}

	/*index根据size来决定*/
	if(size <= ALLOC_BASE_SIZE){	/*如果size不足ALLOC_BASE_SIZE*/
		index = 1;
	}else{	/*如果size >= index * ALLOC_BASE_SZE*/
		index = size / ALLOC_BASE_SIZE;
		if(size % ALLOC_BASE_SIZE != 0){	/*如果size > index * ALLOC_BASE_SZE*/
			index++;
		}
		/*size == index * ALLOC_BASE_SIZE*/
	}

	/*找到目标节点*/
	if(poll->all_index[index].used->real_size == size && poll->all_index[index].used->data == ptr){	/*如果是第一个节点*/
		now = poll->all_index[index].used;
		poll->all_index[index].used = now->next;
		poll->all_index[index].used_count--;

		now->next = poll->all_index[index].free;	/*添加到free队列*/
		poll->all_index[index].free = now;
		poll->all_index[index].free_count++;
	}else{	/*如果不是第一个节点*/
		prev = poll->all_index[index].used;

		while(1){
			now = prev->next;
			if(now == NULL){
				return -1;	/*没有找到节点*/
			}

			if(now->real_size == size && now->data == ptr){	/*找到的情况*/
				prev->next = now->next;
				poll->all_index[index].used_count--;

				now->next = poll->all_index[index].free;
				poll->all_index[index].free = now;
				poll->all_index[index].free_count++;

				break;
			}

			/*此轮循环没有找到*/
			prev = now;
		}
	}

//	printf("FRee index %d there are %d free\n" , index , poll->all_index[index].free_count);
	return 0;
}

