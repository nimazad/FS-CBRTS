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
	int app_id = -1;

	app_id = atoi(argv[1]);
	int res = rt_intra_app_sched_event(app_id);
	printf("res = %d app %d scheduling event!\n", res, app_id);/**/
	return 0;
}

