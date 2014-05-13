/*
 * A sample program for RESCH.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

int main(int argc, char* argv[])
{
	
	/* bannar. */
	//printf("sample program %s\n", argv[0]);
	int tmp = -1;
	tmp = rt_create_task();
	printf("%d\n", tmp);/**/
	//printf("task %d is created!\n", tmp);/**/
	return 0;
}

