#ifndef __API_H__
#define __API_H__

/* device name. */
#define HSF_DEVNAME	"/dev/AdHierSched"

/* available priorities. */
#define HSF_MAX_APP_PRIO	96
#define HSF_MIN_APP_PRIO	4

#define CPU_NR	0
#define experiment_duration 610000
#define number_of_proc 2
#define max_number_of_apps 4
#define alpha_epsilon 5
/* period value for non-perioid tasks. */
#define HSF_PERIOD_INFINITY	0

#define JIFFY_TIMER
// #define HR_TIMER

/* API command numbers. */
#define API_CREATE_SERVER	 		10
#define API_SET_SERVER_PARAM	 		11
#define API_ATTACH_SERVER_TO_SERVER 		12
#define API_CREATE_TASK		 		13
#define API_ATTACH_TASK_TO_SERVER 		14
#define API_RELEASE_SERVER		 	15
#define API_KILL_SERVER		 		16
#define API_STOP				17
#define API_RUN					18
#define API_ATTACH_TASK_TO_MOD			19
#define API_SET_TASK_PARAM	 		20
#define API_RELEASE_TASK	 		21
#define API_DETACH_TASK				22
#define API_DETACH_SERVER			23
#define API_TASK_FINISH_JOB			24
#define API_TEST_TIMER				25
#define API_PRINT_LOG				26
#define API_CREATE_APP		 		27 // Multicore implementation
#define API_SET_APP_PARAM			28
#define API_ATTACH_TASK_TO_APP 		29
#define API_ATTACH_SERVER_TO_APP 		30
#define API_INTRA_APP_SCHED_EVENT		31
#define API_GET_CTRL_DATA			32
#define API_GET_MNGER_DATA			33

// Server queue_type
#define PRIORITY_QUEUE	0
#define DEADLINE_QUEUE	1


/* Timer error ON 1--> on */
#define timer_error_on 0
#define limited_executions 0
#define LOG_LENGTH 20000
#define CONTROL_PERIOD		1
#define HISTORY_LENGTH		10


#define NO_NODS			25
#define NO_INPUT		2 //Should be 2
#define Learning_rate		5
/* result values. they should be non-negative! */
#define RES_SUCCESS	0	/* sane result. */
#define RES_FAULT	-1	/* insane result. */
#define RES_ILLEGAL	2	/* illegal operations. */
#define RES_MISS	3 	/* deadline miss. */
#define YES		1
#define NO		0
#define INT_MOD		100
#define REG_MOD		0
#define DEBUG		0
//int test_mod = 0;

#define MOD_RUNNING	0
#define MOD_STOP	1


#define SERVER_DEACTIVE		0
#define SERVER_ACTIVE		1
#define SERVER_SLEEP		2
#define SERVER_READY		3
#define SERVER_DETACHED		4


#define FOUND		1
#define NOT_FOUND	0
struct Mnger_Data{
  int alpha[max_number_of_apps];
  int importance[max_number_of_apps];
  int periods[max_number_of_apps];
  int priorities[max_number_of_apps];
  int sp[max_number_of_apps][number_of_proc];
  int server_ids[max_number_of_apps][number_of_proc];
  int number_of_apps;
  int res;
};
struct Ctrl_Data{
  int beta;
  int mu;
  int sp[number_of_proc];
  int server_id[number_of_proc];
  int slacks[number_of_proc];
  int proc_list[number_of_proc];
  int alpha;
  int period;
  int priority;
  int dl_miss;  
  int importance;
  int res;
};
struct API_Data{
int id;
int id2;
int priority;
int queue_type;
int server_type;
int proc_id;
int importance;
long period;
long budget;
long deadline;
int alpha;
struct Ctrl_Data *ctrl;
struct Mnger_Data *mnger;
};
#endif


/*----------handler types-------------*/
#define SERVER_RELEASE_HANDLER	10
#define SERVER_BUDGET_HANDLER	11
#define TASK_RELEASE_HANDLER	12
#define CBS_BUDGET_HANDLER	13

#define API_CALL_ID		15
#define STOP_TASK_ID		16
#define RUN_TASK_ID		17


 /* cases for if_head_can_preempt_running function*/
#define UNKNOWN					0
#define ACT_NULL_GLOBAL_RUN_NULL		1
#define ACT_NULL_CAN_NOT_PREEMPT_RUN		2
#define ACT_NULL_CAN_PREEMPT_RUN		3
#define ACT_NULL_NOT_GLOBAL			4
#define ACT_SIBL_CAN_NOT_PREEMPT		5
#define ACT_SIBL_CAN_PREEMPT			6
#define OUTRANK_ACT_CAN_NOT_PREEMPT_ANC	7
#define OUTRANK_ACT_CAN_PREEMPT_ANC		8
#define NOT_OUTRANK_ACT				9
#define ACT_PAR_RUN_NULL			10
#define ACT_PAR_CAN_NOT_PREEMPT_RUN		11
#define ACT_PAR_CAN_PREEMPT_RUN		12
#define PAR_SERV_NOT_ACT			13
#define RUN_TASK_NULL				14
#define RUN_SIBL_CAN_NOT_PREEMPT		15
#define RUN_SIBL_CAN_PREEMPT			16

#define TASK_ATTACHED		1
#define TASK_READY		2
#define TASK_SLEEP		3
#define TASK_RUNNING_HSF	5
#define TASK_DETACHED		6

#define TIMER_ON	0
#define TIMER_OFF	1

#define SERVER_TYPE	10
#define TASK_TYPE	11
#define CBS	1
#define PERIODIC_SERVER	 0

/*------------------------------DEBUG-------------------------------*/
// #define MONITOR_WORKLOAD
#define DEBUG_CHECKS
#define DEBUG_SCHED_ERRORS //For scheduling error
#define LOG

#define DEBUG_API_FINISH_JOB
#define DEBUG_HANDLER_SERVER_BUDGET
#define DEBUG_HANDLER_TASK_RELEASE
#define DEBUG_HANDLER_SERVER_RELEASE
#define DEBUG_HANDLER_CBS_BUDGET
#define DEBUG_SCHED
#define DEBUG_SCHED_TMP
#define DEBUG_HANDLER
#define DEBUG_SCHED_API
#define DEBUG_SCHED_CBS
#define DEBUG_SCHED_MAIN
