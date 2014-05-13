#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "../../library/hsf_api.h"


/**
 * internal function for APIs, using ioctl() system call.
 */

static inline int __api(unsigned long cmd, struct API_Data data)
{
	int fd, ret;
	fd = open(HSF_DEVNAME, O_RDWR);
	if (fd < 0) {
		printf("Error: failed to access the module!\n");
		return RES_FAULT;
	}
	ret = ioctl(fd, cmd, &data);
	close(fd);
	//printf("api call :%lu ret = %d df: %d\n", cmd, ret, fd);
	return ret;
}
/*****************************************************
 *                  API functions                    *
 *****************************************************/
int rt_create_server()
{
	struct API_Data data;
	return __api(API_CREATE_SERVER, data) ;
}
int rt_set_server_param(int server_id, long period, long deadline, long budget, int priority, int proc_id)
{
	struct API_Data data;
	data.id  	= server_id;
	data.period 	= period;
	data.budget 	= budget;
	data.deadline	= deadline;
	data.priority 	= priority;
	data.proc_id 	= proc_id;
	return __api(API_SET_SERVER_PARAM, data);
}
int rt_attach_server_to_server(int server_id, int server_id2)
{
	struct API_Data data;
	data.id  = server_id;
	data.id2 = server_id2;
	return __api(API_ATTACH_SERVER_TO_SERVER, data);
}
int rt_create_task()
{
	struct API_Data data;
	return __api(API_CREATE_TASK, data) ;
}
int rt_attach_task_to_server(int server_id, int task_id, int server_type)
{
	struct API_Data data;
	data.id  = server_id;
	data.id2 = task_id;
	data.server_type = server_type;
	return __api(API_ATTACH_TASK_TO_SERVER, data);
}
int rt_release_server(int server_id)
{
	struct API_Data data;
	data.id  = server_id;
	return __api(API_RELEASE_SERVER, data);
}
int rt_kill_server(int server_id)
{
	struct API_Data data;
	data.id  = server_id;
	return __api(API_KILL_SERVER, data);
}
int rt_stop()
{
	struct API_Data data;
	return __api(API_STOP, data);
}
int rt_run()
{
	struct API_Data data;
	return __api(API_RUN, data);
}
int rt_set_task_param(int task_id, long period, long deadline, long exec_time, int priority)
{
    struct API_Data data;
    data.id  		= task_id;
    data.period 	= period;
    data.budget 	= exec_time;
    data.deadline	= deadline;
    data.priority 	= priority;
    return __api(API_SET_TASK_PARAM, data);
}
int rt_attach_task_to_mod(int task_id)
{
    struct API_Data data;	
    data.id  		= task_id;
    return __api(API_ATTACH_TASK_TO_MOD, data);
}
int rt_release_task(int task_id)
{
	struct API_Data data;
	data.id  = task_id;
	return __api(API_RELEASE_TASK, data);
}
int rt_detach_task(int task_id)
{
      struct API_Data data;
      data.id  = task_id;
      return __api(API_DETACH_TASK, data);
}
int rt_detach_server(int server_id)
{
      struct API_Data data;
      data.id  = server_id;
      return __api(API_DETACH_SERVER, data);
}
int rt_task_finish_job(int task_id)
{
      struct API_Data data;
      data.id  = task_id;
      return __api(API_TASK_FINISH_JOB, data);
}
int rt_test_timer()
{
      struct API_Data data;
      return __api(API_TEST_TIMER, data);
}
int rt_print_log(int server_id)
{
	struct API_Data data;
	data.id= server_id;
	return __api(API_PRINT_LOG, data) ;
}
int rt_create_app(int queue_type)
{
	struct API_Data data;
	data.queue_type  = queue_type;
	return __api(API_CREATE_APP, data);
}
int rt_set_app_param(int app_id, long period, int bandwidth, int importance, int priority)
{
	struct API_Data data;
	data.id  	= app_id;
	data.period 	= period;
	data.alpha	= bandwidth;
	data.importance= importance;
	data.priority = priority;
	return __api(API_SET_APP_PARAM, data);
}
int rt_attach_server_to_app(int server_id, int app_id)
{
	struct API_Data data;
	data.id  = server_id;
	data.id2 = app_id;
	return __api(API_ATTACH_SERVER_TO_APP, data);
}
int rt_attach_task_to_app(int task_id, int app_id)
{
	struct API_Data data;
	data.id  = task_id;
	data.id2 = app_id;
	return __api(API_ATTACH_TASK_TO_APP, data);
}
int rt_intra_app_sched_event(int app_id)
{
	struct API_Data data;
	data.id = app_id;
	return __api(API_INTRA_APP_SCHED_EVENT, data);
}
struct Ctrl_Data * rt_get_ctrl_data(int app_id)
{
	struct API_Data data;
	data.id = app_id;
	data.ctrl = (struct Ctrl_Data *)malloc(sizeof(struct Ctrl_Data));
	data.ctrl->res = __api(API_GET_CTRL_DATA, data);
	return data.ctrl;
}
struct Mnger_Data * rt_get_mnger_data()
{
	struct API_Data data;
	data.mnger = (struct Mnger_Data *)malloc(sizeof(struct Mnger_Data));
	data.mnger->res = __api(API_GET_MNGER_DATA, data);
	return data.mnger;
}