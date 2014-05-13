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
	int server = atoi(argv[1]);;
	int tmp = rt_release_server(server);
	printf("res:%d server %d released!\n", tmp, server);/**/
	return 0;
}

