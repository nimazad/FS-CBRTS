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
	int task = atoi(argv[1]);;
	int tmp = rt_release_task(task);
	printf("res:%d task %d released!\n", tmp, task);/**/
	return 0;
}

