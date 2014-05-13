#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "../library/hsf_api.h"
int main(int argc, char* argv[])
{
	int app_id, i, j;
	struct Ctrl_Data *ctrl_data;
	struct Mnger_Data *mnger_data;
	
	app_id 		= atoi(argv[1]);
	
	ctrl_data = (struct Ctrl_Data*) rt_get_ctrl_data(app_id);
	printf("status %d\n", ctrl_data->res);
	for(i=0;i<number_of_proc;i++){
	    printf("server:%d == beta:%d mu:%d dl_miss:%d importance: %d slack%d\n", ctrl_data->server_id[i], ctrl_data->beta, ctrl_data->mu, ctrl_data->dl_miss, ctrl_data->importance, ctrl_data->slacks[i]);
	}
	mnger_data = (struct Mnger_Data*) rt_get_mnger_data();
	for(i=0;i<mnger_data->number_of_apps;i++){
	    printf("app:%d == alpha:%d importance: %d period %d\n", i, mnger_data->alpha[i], mnger_data->importance[i], mnger_data->periods[i]);
	}
	for(i=0;i<mnger_data->number_of_apps;i++)
	      for(j=0;j<number_of_proc;j++)
	      {
		  printf("sp[%d][%d]: %f\n", i, j, mnger_data->sp[i][j]/(double)mnger_data->periods[i]);
	      }
	return 0;
}

