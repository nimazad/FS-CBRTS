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
	int id, priority, proc_id;
	long period, deadline, budget;
	id = -1;
	
	id 		= atoi(argv[1]);
	period 		= atoi(argv[2]);
	deadline 	= atoi(argv[3]);
	budget 		= atoi(argv[4]);
	priority	= atoi(argv[5]);
	proc_id		= atoi(argv[6]);
	
	int res = rt_set_server_param(id, period, deadline, budget, priority, proc_id);
	printf("res = %d set server %d params!\n", res, id);/**/
	return 0;
}

