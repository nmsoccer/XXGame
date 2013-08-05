/*
 * cs_module1.c
 *
 *  Created on: 2013-6-5
 *      Author: leiming
 */
#include <stdio.h>
#include"common.h"


int module_start(int type , runtime_env_t *pruntime_env , void *ptr)
{
	printf("module1 starts...\n");
	return -1;
}
