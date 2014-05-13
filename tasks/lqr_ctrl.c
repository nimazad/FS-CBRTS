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
#include "lqr_ctrl.h"

int overloads;
double integral_states[2];
double return_alpha(int apps, int procs, double sp[apps][procs], int app_id)
{
      double alpha=0;
      int i;
      for(i=0;i<procs;i++)
	    alpha += sp[app_id][i];
      return alpha;
}
int main(int argc, char* argv[])
{
	int app_id;
	long i, j;
	struct timeval tv_start, tv_end;
	int task_id, no_jbs;
	
	task_id		= atoi(argv[1]);
	no_jbs 		= atoi(argv[2]);
	app_id		= atoi(argv[3]);
	int tmp;
	FILE *f = fopen(log_file[app_id], "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}
	
	tmp = rt_attach_task_to_mod(task_id);
	printf("res: %d task %d controller of app :%d attached to the module!\n", tmp, task_id, app_id);
	if(tmp != 0)
	{
	    printf("attaching failed\n");
	    return 0;
	}
	
	i = 0;
	j =0;
	printf("task %d start!\n", task_id);
	
	integral_states[0] = 0;
	integral_states[1] = 0;
	overloads = 0;
	while(j<no_jbs)
	{
	    gettimeofday(&tv_start, (struct timezone*)0);
	    if(j != 0)
		if(control(app_id, f) == -1)
		{
		    fclose(f);
		    return -1;
		}
	    printf("task <%d> job %ld -- controller of app :%d on core %d  !\n", task_id, j, app_id, sched_getcpu());
	    gettimeofday(&tv_end, (struct timezone*)0);
	    fprintf(f, "task <%d>job %d Elapsed time: %f \n", task_id, i, (double) ((tv_end.tv_sec - tv_start.tv_sec)*1000000 + (tv_end.tv_usec - tv_start.tv_usec))/1000000.0);
	    if(rt_task_finish_job(task_id) == -1)
	    {
		printf("task %d aborted!!!\n", task_id);
		fclose(f);
		return;
	    }
	    j++;
	}
	
	fclose(f);
	printf("task %d finished!\n", task_id);
	
	rt_detach_task(task_id);
	return 0;
}
int control(int app_id, FILE *f)
{
	int i, j, Beta, Mu, res;
	struct Ctrl_Data *ctrl_data;
	struct Mnger_Data *mnger_data;
	
	double total_slack = 0;
	/* Getting control data*/
	ctrl_data = (struct Ctrl_Data*) rt_get_ctrl_data(app_id);
	printf("status %d\n", ctrl_data->res);
	if(ctrl_data->res == -1)
	{
	      printf("fetching ctrl_data failed!\n");
	      return -1;
	}
	
	Beta = 0;
	Mu = 0;
	/*Calculating Beta and Mu*/
	for(i=0;i<number_of_proc;i++)
	{
	      //printf("core %d proc_list: %d\n", i, ctrl_data->proc_list[i]);
	      if(ctrl_data->server_id[i] >= 0)
	      {
		  total_slack += ctrl_data->slacks[i];
	      }
	}
	Beta 	= ctrl_data->beta;
	Mu 	= ctrl_data->mu;
	/* LQR control */
	double normalBeta, normalMu, e1, e2, bandwidth_change, period_change, new_bandwidth, new_period;
	int sampleTime = 400;
	int new_budget;
	normalBeta 	= (double) Beta/sampleTime;
	normalMu 	= (double) Mu/sampleTime;
	e1 = r1[app_id] - 100*(normalBeta - normalMu);
	e2 = r2[app_id] - ctrl_data->dl_miss;
	printf("e1: %f, e2:%f\n", e1, e2);
	integral_states[0] += e1;
	integral_states[1] += e2;
	bandwidth_change 	= -e1_gain[app_id][0]*e1 - e1_gain[app_id][1]*e2 - e1_gain[app_id][2]*integral_states[0] - e1_gain[app_id][3]*integral_states[1];
	period_change 		= -e2_gain[app_id][0]*e1 - e2_gain[app_id][1]*e2 - e2_gain[app_id][2]*integral_states[0] - e2_gain[app_id][3]*integral_states[1];
	new_bandwidth = operating_band[app_id] + bandwidth_change;
	new_period = operating_period[app_id] + period_change;
	printf("=============================== app: %d ===============================\n", app_id);
	printf("bandwidth_change: %f, period_change:%f, new_bandwidth %f, new_period %f \n", bandwidth_change, period_change, new_bandwidth, new_period);
	//fprintf(f, "e1: %f , e2: %f e3: %f ,e4: %f , alpha: %f , T: %f ,B: %d\n", e1, e2, integral_states[0], integral_states[1], new_bandwidth, new_period, ((int)(new_bandwidth*new_period)/100));
	
	//Range control
	if(new_bandwidth > alpha_max[app_id])
	{
	    new_bandwidth = alpha_max[app_id];
	    printf("new_bandwidth: %f\n", new_bandwidth);
	}
	else if(new_bandwidth < alpha_min[app_id])
	{
	    new_bandwidth = alpha_min[app_id];
	    printf("new_bandwidth: %f\n", new_bandwidth);
	}
	if(new_period < T_min[app_id])
	{
	    new_period = T_min[app_id];
	    printf("new_period: %f\n", new_period);
	}
	else if(new_period > T_max[app_id])
	{
	    new_period = T_max[app_id];
	    printf("new_period: %f\n", new_period);
	}
	/* allocating based on new alpha and P*/
	int allocated = 0;
	/*
	 * sort
	 */
	int index[number_of_proc];
	int new_proc_list[number_of_proc]; // list of processors that component will be allocated
	for(i=0;i<number_of_proc;i++)
	{
	      index[i] = i;
	      new_proc_list[i] = 0;
	}
	int available_band[number_of_proc];
	for(i=0;i<number_of_proc;i++)
	{
	      available_band[i] = 0;
	      available_band[i]+= ctrl_data->slacks[i] +  ((double)ctrl_data->sp[i]/(double)(ctrl_data->period))*100;
	}
	my_print_array(&ctrl_data->slacks, index, number_of_proc);
	my_print_array(available_band, index, number_of_proc);
	my_sort(available_band, &index, number_of_proc);
	my_print_array(available_band, index, number_of_proc);
	/*
	 * loop in current processors: trying to allocate
	 */
	double total_available = 0;
	for(i=0;i<number_of_proc;i++)
	{
	      if(ctrl_data->proc_list[index[i]])
	      {
		    new_proc_list[index[i]] = 1;
		    total_available += available_band[i];
		    printf("available on this proc[%d]: %d total_available: %f, sp %f, slack %d\n", index[i], available_band[i], total_available, 100*((double)ctrl_data->sp[index[i]]/(double)(ctrl_data->period))  , ctrl_data->slacks[i]);		    
		    if(new_bandwidth <= total_available)
		    {
			  allocated = 1;
			  printf("allocation finished! current processors\n");
			  break;
		    }
	      }
	}
	/*
	 * loop in all except current: trying to allocate using all processors
	 */
	if(allocated == 0)
	{
		for(i=0;i<number_of_proc;i++)
		{
		      if(ctrl_data->proc_list[index[i]] == 0)
		      {
			    new_proc_list[index[i]] = 1;
			    total_available += available_band[i];
 			    printf("available on this proc[%d] %d total_available: %f, sp %f, slack %d\n", index[i], available_band[i], total_available, 100*((double)ctrl_data->sp[index[i]]/(double)(ctrl_data->period))  , ctrl_data->slacks[i]);
			    if((int)new_bandwidth*new_period <= (int)total_available*new_period)
			    {
				  allocated = 1;
				  printf("allocation finished\n");
				  break;
			    }
		      }
		}
	}
	
	if(allocated)
	{
	      for(i=0;i<number_of_proc;i++)
	      {
		    // Application has an active server on this processor
		    if(new_proc_list[i] && ctrl_data->proc_list[i])//(ctrl_data->server_id[i] >= 0)
		    {
			  printf("server %d on proc %d has to be reconfigured\n", ctrl_data->server_id[i], i);
			  new_budget = ( (available_band[index[i]]/total_available)*(int)(new_bandwidth*new_period) )/100;
			  printf("server id: %d, new_period: %ld new_deadline: %ld, new_budget %ld priority %d proc_id %d\n", ctrl_data->server_id[i], (long) new_period, (long) new_period, (long) new_budget, ctrl_data->priority, i);
			  fprintf(f, "proc %d , new_budget %d new_period %f\n", i, new_budget, new_period);
			  res = rt_set_server_param(ctrl_data->server_id[i], (long) new_period, (long) new_period, (long) new_budget, ctrl_data->priority, i);
			  if(res == -1)
			      return -1;
		    }
		    else if(new_proc_list[i]  && ctrl_data->proc_list[i] == 0) //This proc is newly assigned
		    {
			  printf("new server on proc %d has to be added\n", i);
			  int server_id = rt_create_server();
			  new_budget = ( (available_band[index[i]]/total_available)*(int)(new_bandwidth*new_period) )/100;
			  printf("server id: %d, new_period: %ld new_deadline: %ld, new_budget %ld priority %d proc_id %d\n", server_id, (long) new_period, (long) new_period, (long) new_budget, ctrl_data->priority, i);
			  fprintf(f, "proc %d , new_budget %d new_period %f\n", i, new_budget, new_period);
			  res = rt_set_server_param(server_id, (long) new_period, (long) new_period, (long) new_budget, ctrl_data->priority, i);
			  rt_attach_server_to_app(server_id, app_id);
			  rt_release_server(server_id);
		    }
		    else if(new_proc_list[i] == 0 && ctrl_data->proc_list[i]) //This has to be removed
		    {
			  printf("server %d on proc %d has to be detached\n", ctrl_data->server_id[i], i);
			  rt_detach_server(ctrl_data->server_id[i]);
		    }
	      }
	      printf("setting app params new_period %ld, new_bandwidth: %d\n", (long)new_period, (int)new_bandwidth);
	      rt_set_app_param(app_id, (long)new_period, (int)new_bandwidth, (int)ctrl_data->importance, ctrl_data->priority);
	      printf("setting app params finished\n");
	      return 0;
	}
	
	/*
	 * overload: allocation failed, have to do compression
	 */
	if(allocated == 0)
	{
		overloads++;
		printf("\n\n *************\n*************\n OVERLOAD %d \n*************\n*************\n\n", overloads);
		fprintf(f, "app:[%d]\n\n *************\n*************\n OVERLOAD %d \n*************\n*************\n\n", app_id, overloads);
		double new_bandwidths[number_of_proc];
		double sp[max_number_of_apps][number_of_proc], sp_old[max_number_of_apps][number_of_proc], density_old[max_number_of_apps][number_of_proc], density[max_number_of_apps][number_of_proc];
		int sp_indx[max_number_of_apps][number_of_proc];
		// Allocated even if utilization is more than one
		for(i=0;i<number_of_proc;i++)
		{
		      new_bandwidths[index[i]] = 0;
		      if(new_proc_list[index[i]] == 1)
		      {
			     new_bandwidths[index[i]] =  ( (available_band[index[i]]/total_available)*(new_bandwidth));
			     printf("allocating %f on proc %d\n", new_bandwidths[index[i]], index[i]);
		      }
		}
		//struct Mnger_Data *mnger_data;
		mnger_data = (struct Mnger_Data*) rt_get_mnger_data();
		for(i=0;i<mnger_data->number_of_apps;i++){
		    printf("app:%d == alpha:%d importance: %d period %d\n", i, mnger_data->alpha[i], mnger_data->importance[i], mnger_data->periods[i]);
		}
		for(i=0;i<mnger_data->number_of_apps;i++)
		      for(j=0;j<number_of_proc;j++)
		      {/*initializing sp*/
			  sp[i][j] = mnger_data->sp[i][j]/(double)mnger_data->periods[i];
			  sp_old[i][j] = sp[i][j];
			  if(i == app_id)
				sp[i][j] = new_bandwidths[j]/100;
			  sp_indx[i][j] = i;
			  if(sp[i][j] != 0)
				density[i][j] =  mnger_data->importance[i] / sp[i][j] ;
			  else
				density[i][j] = 0;
			  density_old[i][j] = density[i][j];
		      }
		my_print_array_sp_index(mnger_data->number_of_apps, number_of_proc, sp_indx);
		my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, sp, "sp before");
		my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, density, "density before");
		my_sort_double_sp(mnger_data->number_of_apps, number_of_proc, sp_indx, density);
		double remaining = 1;
		int app_indx;
		/*
		 * Compression
		 */
		for(j=0;j<number_of_proc;j++)
		{
		      remaining = 1;
		      for(i=0;i<mnger_data->number_of_apps;i++)
		      {
			      //From high density to low
			      app_indx = sp_indx[i][j];
			      sp[app_indx][j] = min(sp[app_indx][j], remaining);
			      remaining -= sp[app_indx][j];
			      // New densities
			      if(sp[app_indx][j] != 0)
				    density[app_indx][j] =  mnger_data->importance[app_indx] / sp[app_indx][j] ;
			      else
				    density[app_indx][j] = 0;
		      }
		}
		my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, sp, "sp after");
		my_print_array_sp_index(mnger_data->number_of_apps, number_of_proc, sp_indx);
		/*
		 * Constraint on alpha_epsilon
		 */
		double alpha;
		int donor_app_id, donor_proc_id;
		for(i=0;i<mnger_data->number_of_apps;i++)
		{
		      alpha = 0;
		      for(j=0;j<number_of_proc;j++)
		      {
			    alpha += sp[i][j]; 
		      }
		      printf("alpha[%d]: %f alpha_epsilon: %f\n", i, alpha, ((double)alpha_epsilon)/100);
		      while(alpha < ((double)alpha_epsilon)/100)//TODO: while
		      {
			    find_donor(mnger_data->number_of_apps, number_of_proc, sp_indx, density, density_old, &donor_app_id, &donor_proc_id, i);
			    double donation;
			    donation = min(min(((double)alpha_epsilon)/100-alpha, sp[donor_app_id][donor_proc_id]), return_alpha(mnger_data->number_of_apps, number_of_proc, sp, donor_app_id));
			    printf("app: %d proc: %d donation: %f\n", donor_app_id, donor_proc_id, donation);
			    
			    alpha += donation;
			    sp[i][donor_proc_id] += donation;
			    sp[donor_app_id][donor_proc_id] -= donation;
			    if(sp[i][donor_proc_id] != 0)
				density[i][donor_proc_id] =  mnger_data->importance[i] / sp[i][donor_proc_id] ;
			  else
				density[i][donor_proc_id] = 0;
			  if(sp[donor_app_id][donor_proc_id] != 0)
				density[donor_app_id][donor_proc_id] =  mnger_data->importance[donor_app_id] / sp[donor_app_id][donor_proc_id] ;
			  else
				density[donor_app_id][donor_proc_id] = 0;
			  my_print_array_double_sp(mnger_data->number_of_apps, number_of_proc, sp, "sp after donation");
			  if(alpha<0 || donation == 0)
			  {
				printf("ERROR!!! alpha<0 or donation=0\n");
				return -1;
			  }
		      }
		}
		//Updating servers
		long tmp_period, budget;
		for(i=0;i<mnger_data->number_of_apps;i++)
		{
		      if(i == app_id)
			    tmp_period = new_period;
		      else
			    tmp_period = mnger_data->periods[i];
		      for(j=0;j<number_of_proc;j++)
		      {
			    if(sp_old[i][j] != sp[i][j])
			    {
				  if(sp_old[i][j] == 0)
				  {
					int server_id = rt_create_server();
					budget = sp[i][j]*tmp_period;
					res = rt_set_server_param(server_id, tmp_period, tmp_period, budget, mnger_data->priorities[i], j);
					rt_attach_server_to_app(server_id, app_id);
					printf("have to create a new server: [%d][%d] newperiod %lu new_budget %lu\n", i, j, tmp_period, budget);
					fprintf(f, "have to create a new server: [%d][%d] newperiod %lu new_budget %lu\n", i, j, tmp_period, budget);
				  }
				  else if(sp[i][j] == 0)
				  {
					printf("have to detach server: [%d][%d] server: %d\n", i, j, mnger_data->server_ids[i][j]);
					fprintf(f, "have to detach server: [%d][%d] server: %d\n", i, j, mnger_data->server_ids[i][j]);
					rt_detach_server(mnger_data->server_ids[i][j]);
				  }
				  else
				  {
					budget = sp[i][j]*tmp_period;
					res = rt_set_server_param(mnger_data->server_ids[i][j], tmp_period, tmp_period, budget, mnger_data->priorities[i], j);
					printf("have to update: [%d][%d] server: %d newperiod %lu new_budget %lu\n", i, j, mnger_data->server_ids[i][j], tmp_period, budget);
					fprintf(f, "have to update: [%d][%d] server: %d newperiod %lu new_budget %lu\n", i, j, mnger_data->server_ids[i][j], tmp_period, budget);
				  }
			    }
		      }
		}
		printf("importance %d\n", mnger_data->importance[app_id]);
		int bandwidth = (int)100*return_alpha(mnger_data->number_of_apps, number_of_proc, sp, app_id);
		//rt_set_app_param(int app_id, long period, int bandwidth, int importance, int priority)
		if(rt_set_app_param(app_id, (long)new_period, bandwidth, (int)mnger_data->importance[app_id], mnger_data->priorities[app_id]) == -1)
		    return -1;
		mnger_data = (struct Mnger_Data*) rt_get_mnger_data();
		if(mnger_data->res == -1)
		    return -1;
		printf("----------\n final sp*** \n----------");
		for(j=0;j<number_of_proc;j++)
		{
		      printf("\n");
		      for(i=0;i<mnger_data->number_of_apps;i++)
			    printf("%f	(%d / %d)   ", ((double)mnger_data->sp[i][j]) / mnger_data->periods[i], mnger_data->sp[i][j], mnger_data->periods[i]);
		}
		printf("\n----------\n");
		return 0;
	}
	printf("alpha %d\n", ctrl_data->alpha);

	return 0;
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
my_print_array(int* array, int* index, int length)
{
      int i;
      printf("----------\n");
      for(i=0;i<length;i++)
	    printf("index %d is %d\n", index[i], array[i]);
      printf("----------\n");
}

int my_sort_double_sp(int apps, int procs, int index[apps][procs], double sp[apps][procs])
{
  int i,j, proc, tmp_indx;
  double tmp;
  for(proc=0;proc < procs;proc++)
	for(i=0;i<apps;i++)
	{
	      for(j=i+1;j<apps;j++)
	      {
		    if(sp[i][proc] < sp[j][proc])
		    {
			  tmp = sp[i][proc];
			  tmp_indx = index[i][proc];
			  sp[i][proc] = sp[j][proc];
			  sp[j][proc] = tmp;
			  index[i][proc] = index[j][proc];
			  index[j][proc] = tmp_indx;
		    }
		    //printf("swithching sp[%d][%d] (%f) with sp[%d][%d] (%f) \n", i, proc, tmp, j, proc, sp[i][proc]);
	      }
	      //printf("sp[%d][%d] is %f\n", i, proc, sp[i][proc]);
	}
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
my_print_array_sp_index(int apps, int procs, int sp_indx[apps][procs])
{
      int i, j;
      printf("----------\n sp_indx \n----------");
      for(j=0;j<procs;j++)
      {
	    printf("\n");
	    for(i=0;i<apps;i++)
		  printf("%d	", sp_indx[i][j]);
      }
      printf("\n----------\n");
}

find_donor(int apps, int procs, int index[apps][procs], double den[apps][procs], double den_old[apps][procs], int *donor_app, int *donor_proc, int rec_app)
{
      printf("finding donor for %d starts\n", rec_app);
      int i,proc, app_indx;
      double candidate_den = DBL_MAX;
      for(proc=0;proc < procs;proc++)
	    for(i=0;i<apps;i++)
	    {
		  app_indx = index[i][proc];
		  //den_old[app_indx][proc] > 0 because we dont want to migrate sp to a new processor
		  if(app_indx != rec_app && den[app_indx][proc] > 0 && den_old[rec_app][proc] > 0 && den[app_indx][proc] < candidate_den)
		  {
			candidate_den = den[app_indx][proc];
			*donor_app = app_indx;
			*donor_proc = proc;
			printf("[%d][%d] app: %d proc: %d densiti_old: %f\n",  app_indx, proc, *donor_app, *donor_proc,  den_old[rec_app][proc]);
		  }
	    }
	    
      printf("ends\n");
}


