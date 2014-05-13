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
	int id, priority;
	long period, deadline, exec_time;
	id = -1;
	
	id 		= atoi(argv[1]);
	period 		= atoi(argv[2]);
	deadline 	= atoi(argv[3]);
	exec_time	= atoi(argv[4]);
	priority	= atoi(argv[5]);
	
	int res = rt_set_task_param(id, period, deadline, exec_time, priority);
	printf("res = %d set task %d params!\n", res, id);/**/
	return 0;
}

