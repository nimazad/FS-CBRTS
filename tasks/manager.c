#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "../library/hsf_api.h"
//#include </home/nima/Downloads/glpk-4.54/src/glpk.h>
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#include <float.h>


int main(int argc, char* argv[])
{
	int app_id, i, j, Beta, Mu, res;
	struct Mnger_Data *mnger_data;
	
	/* Getting control data*/
	mnger_data = (struct Mnger_Data*) rt_get_mnger_data();
	printf("status %d\n", mnger_data->res);
	if(mnger_data->res == -1)
	{
	      printf("fetching mnger_data failed!\n");
	      return;
	}
	FILE *f = fopen("manager_log.txt", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}
	int value[max_number_of_apps];
	int index[max_number_of_apps];
	int total_bandwidth = 0;
	double target_slack = 0;
	for(i=0;i<max_number_of_apps;i++)
	{
	      value[i] = 0;
	      index[i] = i;
	}
	for(i=0;i<mnger_data->number_of_apps;i++)
	{
	      value[i] = mnger_data->importance[i] * mnger_data->alpha[i];
	      total_bandwidth += mnger_data->alpha[i];
	}
	my_print_array(value, index, mnger_data->number_of_apps);
	my_sort(value, index, mnger_data->number_of_apps);
	my_print_array(value, index, mnger_data->number_of_apps);
	
	target_slack = 1 - (double)total_bandwidth/(number_of_proc*100);
	printf("target_slack: %f\n", target_slack);
	
	
	double sp[max_number_of_apps][number_of_proc];
	double slacks[number_of_proc];
	for(j=0;j<number_of_proc;j++)
	{
	      slacks[j] = 1;
	      for(i=0;i<mnger_data->number_of_apps;i++)
		{/*initializing sp*/
		      sp[i][j] = 0;
		      /*if(mnger_data->sp[i][j]>0)
		      {
			    sp[i][j] = ((double)mnger_data->sp[i][j])/mnger_data->periods[i];
			    slacks[j] -= sp[i][j];
		      }*/
		}
	}
	my_print_double(slacks, index, number_of_proc);
	my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, sp, "sp befor partitioning");
	double alpha;
	for(i=0;i<mnger_data->number_of_apps;i++)
	{
	      alpha = ((double)mnger_data->alpha[index[i]])/100;
	      if(best_fit(mnger_data->number_of_apps, number_of_proc, sp, slacks, alpha, index[i], target_slack) == -1)
	      {
		    printf("we have to split %d\n", i);
		    split(mnger_data->number_of_apps, number_of_proc, sp, slacks, alpha, index[i]);
	      }
	}
	my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, sp, "sp after partitioning");
	
	int budget, server_id;
	for(j=0;j<number_of_proc;j++)
	{
	      for(i=0;i<mnger_data->number_of_apps;i++)
	      {
		    //printf("[%d][%d] sp %f server_ids: %d\n", i, j, sp[i][j], mnger_data->server_ids[i][j]);  
		    if(sp[i][j] > 0 && mnger_data->server_ids[i][j] >= 0)
		    {
			printf("have to update [%d][%d]\n", i, j);  
			budget = sp[i][j]*mnger_data->periods[i];
			fprintf(f, "have to update [%d][%d] period: %d , budget: %d\n", i, j, mnger_data->periods[i], budget);
			rt_set_server_param(mnger_data->server_ids[i][j], mnger_data->periods[i], mnger_data->periods[i], budget, mnger_data->priorities[i], j);
		    }
		    else if(sp[i][j] > 0 && mnger_data->server_ids[i][j] < 0)
		    {
			printf("have to create[%d][%d]\n", i, j);
			server_id = rt_create_server();
			budget = sp[i][j]*mnger_data->periods[i];
			fprintf(f, "have to create[%d][%d]period: %d , budget: %d\n", i, j, mnger_data->periods[i], budget);  
			res = rt_set_server_param(server_id, mnger_data->periods[i], mnger_data->periods[i], budget, mnger_data->priorities[i], j);
			rt_attach_server_to_app(server_id, i);
		    }
		    else if(sp[i][j] <= 0 && mnger_data->server_ids[i][j] >= 0)
		    {
			printf("have to detach[%d][%d]\n", i, j);  
			fprintf(f, "have to detach[%d][%d]\n", i, j); 
			rt_detach_server(mnger_data->server_ids[i][j]);
		    }
	      }
	}
	fclose(f);
	return 0;
}

my_print_array(int* array, int* index, int length)
{
      int i;
      printf("----------\n");
      for(i=0;i<length;i++)
	    printf("index %d is %d\n", index[i], array[i]);
      printf("----------\n");
}

my_print_double(double* array, int* index, int length)
{
      int i;
      printf("----------\n");
      for(i=0;i<length;i++)
	    printf("index %d is %f\n", index[i], array[i]);
      printf("----------\n");
}

int my_sort(int* array, int* index, int length)
{
  int i,j, tmp, tmp_indx;
  for(i=0;i<length;i++)
	for(j=i+1;j<length;j++)
	{
	      if(array[i] < array[j])
	      {
		    tmp = array[i];
		    tmp_indx = index[i];
		    array[i] = array[j];
		    array[j] = tmp;
		    index[i] = index[j];
		    index[j] = tmp_indx;
	      }
	}
}

int my_sort_double(double* array, int* index, int length)
{
  int i,j, tmp_indx;
  double tmp;
  for(i=0;i<length;i++)
	for(j=i+1;j<length;j++)
	{
	      if(array[i] < array[j])
	      {
		    tmp = array[i];
		    tmp_indx = index[i];
		    array[i] = array[j];
		    array[j] = tmp;
		    index[i] = index[j];
		    index[j] = tmp_indx;
	      }
	}
}
double my_abs(double a)
{
  if( a < 0)
	return -a;
  return a;
}
int best_fit(int apps, int procs, double sp[apps][procs], double slacks[procs], double alpha, int app_id)
{
      int i, best_proc;
      best_proc = -1;
      double max_slack = 0;//DBL_MIN;
      double current_slack = 0;
      printf("BEST_FIT for: %d alpha: %f\n", app_id, alpha);
      for(i=0;i<procs;i++)
      {
	    current_slack = slacks[i]-alpha;
	    printf("proc: %d distance: current_slack=%f - slack%f\n", i,  current_slack, slacks[i]);
	    if(max_slack < current_slack)
	    {
		  best_proc = i;
		  max_slack = current_slack;
		  printf("updating best_proc: %d distance: %f\n", i, max_slack);
	    }
      }
      if(best_proc >= 0)
      {
	    sp[app_id][best_proc] = alpha;
	    slacks[best_proc] -= alpha;
      }
      return best_proc;
}


int split(int apps, int procs, double sp[apps][procs], double slacks[procs], double alpha, int app_id)
{
      int i, proc_indx;
      double available;
      double tmp_slacks[procs];
      int tmp_indx[procs];
      printf("split starts for: %d", app_id);
      for(i=0;i<procs;i++)
      {
	    tmp_slacks[i] = slacks[i];
	    tmp_indx[i] = i;
      }
      //my_print_double(tmp_slacks, tmp_indx, procs);
      my_sort_double(tmp_slacks, tmp_indx, procs);
      //my_print_double(tmp_slacks, tmp_indx, procs);
      for(i=0;i<procs;i++)
      {
	    proc_indx = tmp_indx[i];
	    available = min(slacks[proc_indx], alpha);
	    sp[app_id][proc_indx] = available;
	    alpha -= available;
	    slacks[proc_indx] -= available;
	    printf("sp[%d][%d]=%f \n", app_id, proc_indx, available);
	    if(alpha == 0)
	    {
		  printf("split finished \n");
		  break;
	    }
      }
      return 0;
}

my_print_array_double_sp(int apps, int procs, double sp[apps][procs], char* array_name)
{
      int i, j;
      printf("----------\n %s \n----------", array_name);
      for(j=0;j<procs;j++)
      {
	    printf("\n");
	    for(i=0;i<apps;i++)
		  printf("%f	", sp[i][j]);
      }
      printf("\n----------\n");
}