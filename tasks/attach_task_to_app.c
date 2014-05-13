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
	int app_id, task_id = -1;

	task_id = atoi(argv[1]);
	app_id = atoi(argv[2]);
	int res = rt_attach_task_to_app(task_id, app_id);
	printf("res = %d task %d is attached to app %d!\n", res, task_id, app_id);/**/
	return 0;
}

