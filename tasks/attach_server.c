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
	int id1, id2 = -1;

	id1 = atoi(argv[1]);
	id2 = atoi(argv[2]);
	int res = rt_attach_server_to_server(id1,id2);
	printf("res = %d Server %d is attached to server %d!\n", res, id2, id1);/**/
	return 0;
}

