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
	int server_id;
	server_id 		= atoi(argv[1]);
	
	int tmp = -1;
	tmp = rt_print_log(server_id);
	printf("%d\n", tmp);/**/
	printf("Application %d log is printed to dmesg!\n", server_id);/**/
	return 0;
}

