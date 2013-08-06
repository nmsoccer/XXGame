/*
 * cs_module0.c
 *
 *  Created on: 2013-6-5
 *      Author: leiming
 */
#include <stdio.h>
#include"common.h"
#include <dlfcn.h>

int (*test)(void);

int module_start(int type , runtime_env_t *pruntime_env , void *ptr)
{
	void *handle = NULL;
	char module_path[256] = "\0";
	module_commond_t *pcommond = (module_commond_t *)ptr;


	printf("module0 starts...\n");
	switch(type)
	{
	case MODULE_TYPE_CS:
	break;
	}

	switch(pcommond->type)
	{
	case MODULE_COMMOND_RELOAD:
		strcpy(module_path , pcommond->data.pmodule_dir_path);	/*模块目录*/
		strcat(module_path , "cs_module0.so");	/*模块文件自己是知道的*/
		printf("~~! reload %s\n" , module_path);

		handle = dlopen(module_path, RTLD_LAZY);
		if(!handle)
		{
			printf("load module %s failed!\n" , module_path);
			return -1;
		}
		test = dlsym(handle , "show_time");

		test();

		break;
	default:
		break;
	}




	return -1;
}


int show_time(void)
{
	printf("Yes,Show Time Now!\n");
	return 0;
}
