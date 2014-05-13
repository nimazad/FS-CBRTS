/*
 * A sample program for RESCH.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>


#define USEC_UNIT	 1000000000
//#define LONG_MAX +2147483647
  #define LONG_MAX +10000000
#define msecs_to_timeval(ms, tv)					\
	do {											\
		tv.tv_sec = ms / 1000; 						\
		tv.tv_usec = (ms - tv.tv_sec*1000) * 1000; 	\
	} while (0);


int main(int argc, char* argv[])
{
	long j;
	
	int task_id, server_id;
	
	/*
	struct sched_param scheduler_params;

	
	scheduler_params.sched_priority = 99;
	printf("changing scheduling policy %d\n", sched_setscheduler( getpid(),
	                    SCHED_FIFO,
	                    &scheduler_params) );
*/
	
	task_id		= atoi(argv[1]);
	server_id 	= atoi(argv[2]);
	int tmp;/* = rt_attach_task_to_server(server_id, task_id);
	printf("res: %d task %d attached to the server!\n", tmp, task_id);
	if(tmp != 0)
	  return 0;*/
	
	tmp = rt_attach_task_to_mod(task_id);
	printf("res: %d task %d attached to the module!\n", tmp, task_id);
	if(tmp != 0)
	  return 0;
	
	j =0;
	printf("idle task %d start!\n", task_id);
	while(1)
	{

//	  printf("task job %ld finish!\n", j);
//	  rt_task_finish_job(task_id);
	  j++;
	}
	printf("task %d finished!\n", task_id);
	
	rt_detach_task(task_id);
	rt_detach_server(server_id);
	return 0;
}

