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
	int id, importance;
	long period;
	int bandwidth;
	int priority;
	id = -1;
	
	id 		= atoi(argv[1]);
	period 		= atoi(argv[2]);
	importance	= atoi(argv[3]);
	bandwidth 	= atoi(argv[4]);
	priority 	= atoi(argv[5]);
	int res = rt_set_app_param(id, period, bandwidth, importance, priority);
	printf("id:%d, period:%ld, importance:%d, bandwidth:%d !\n", id, period, importance, bandwidth);/**/
	printf("res = %d set app %d params!\n", res, id);/**/
	return 0;
}

