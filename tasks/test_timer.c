/*
 * A sample program for RESCH.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "../library/hsf_api.h"

int main(int argc, char* argv[])
{
	
	/* bannar. */
	//printf("sample program %s\n", argv[0]);
	int tmp = -1;
	//struct API_Data data;
	//data.ctrl = (struct ctrl_data *)malloc(sizeof(struct ctrl_data));
	tmp = rt_test_timer();
	printf("res: %d run!\n", tmp);/**/
	//printf("beta: %d !\n", data.ctrl->beta);/**/
	return 0;
}

