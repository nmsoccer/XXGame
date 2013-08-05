/*
 * cs_module0.c
 *
 *  Created on: 2013-6-5
 *      Author: leiming
 */
#include <stdio.h>
#include"common.h"


int module_start(int type , runtime_env_t *pruntime_env , void *ptr)
{

	printf("module0 starts...\n");
	switch(type)
	{
	case MODULE_TYPE_CS:
	break;
	}






	return -1;
}
