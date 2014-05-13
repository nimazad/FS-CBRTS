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
	int tmp = -1;
	tmp = rt_stop();
	printf("res: %d stop!\n", tmp);/**/
	return 0;
}

