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
	int server_id, task_id, server_type = -1;

	server_id	= atoi(argv[1]);
	task_id		= atoi(argv[2]);
	server_type	= atoi(argv[3]);
	int res = rt_attach_task_to_server(server_id, task_id, server_type);
	printf("res = %d task %d is attached to server %d!\n", res, task_id, server_id);/**/
	return 0;
}

