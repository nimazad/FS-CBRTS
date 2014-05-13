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
	int app_id, server_id = -1;

	server_id = atoi(argv[1]);
	app_id = atoi(argv[2]);
	int res = rt_attach_server_to_app(server_id, app_id);
	printf("res = %d Server %d is attached to app %d!\n", res, server_id, app_id);/**/
	return 0;
}

