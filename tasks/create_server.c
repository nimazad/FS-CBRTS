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
	int tmp = -1;
	tmp = rt_create_server();
	printf("%d\n", tmp);/**/
	//printf("Server %d is created!\n", tmp);/**/
	return 0;
}

