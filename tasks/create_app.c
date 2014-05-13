#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
//#include "../library/hsf_api.h"
int main(int argc, char* argv[])
{
	int queue_type;
	queue_type 		= atoi(argv[1]);
	
	int tmp = -1;
	tmp = rt_create_app(queue_type);
	printf("%d\n", tmp);/**/
	//printf("App %d is created!\n", tmp);/**/
	return 0;
}

