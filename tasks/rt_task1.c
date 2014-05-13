/*
 * A sample program for RESCH.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>


#define USEC_UNIT	 1000000000
//#define LONG_MAX +2147483647
  #define LONG_MAX +568000
#define msecs_to_timeval(ms, tv)					\
	do {											\
		tv.tv_sec = ms / 1000; 						\
		tv.tv_usec = (ms - tv.tv_sec*1000) * 1000; 	\
	} while (0);


int main(int argc, char* argv[])
{
	long i, j;
	
	int task_id, no_jbs, exec_time;
	
	task_id		= atoi(argv[1]);
	no_jbs 		= atoi(argv[2]);
	exec_time	= atoi(argv[3]);
	int tmp;/* = rt_attach_task_to_server(server_id, task_id);
	printf("res: %d task %d attached to the server!\n", tmp, task_id);
	if(tmp != 0)
	  return 0;*/
	
	tmp = rt_attach_task_to_mod(task_id);
	printf("res: %d task %d exec_time:%d attached to the module!\n", tmp, task_id, exec_time);
	if(tmp != 0)
	  return 0;
	
	i = 0;
	j =0;
	printf("task %d start!\n", task_id);
	while(j<no_jbs)
	{
	  i=0;
	  while(i<LONG_MAX * exec_time)
	      i++;
	  printf("task <%d> job %ld on core %d  exec_time: %d!\n", task_id, j, sched_getcpu(), exec_time);
	  if(rt_task_finish_job(task_id) == -1)
	  {
	      printf("task %d aborted!!!\n", task_id);
	      return;
	  }
	  j++;
	}
	
	printf("task %d finished!\n", task_id);
	
	rt_detach_task(task_id);
// 	rt_detach_server(server_id);
	return 0;
}

