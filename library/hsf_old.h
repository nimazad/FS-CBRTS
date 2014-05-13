#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <../arch/x86/include/asm/div64.h>
#include <linux/spinlock.h> // spinklock_t
#include <linux/semaphore.h> // struct semaphore
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>

//#include <asm/atomic.h> // atomic
//#include <../../kernels/2.6.31-14-generic/include/resch/core.h>
/* ==============================For API calls ==============================*/
	#include <linux/cdev.h>
	#include <linux/fs.h>
	#include <asm/current.h>
	#include <asm/uaccess.h>
	#include "hsf_api.h"
/* ===================================<End>=================================*/

int scheduling_error;
int Module_status;
int scheduling_flag;
int INT_CNT;
float available_CPU = 1.0;
unsigned long start_time;
unsigned long total_overhead;
unsigned long control_overhead;
struct timeval start_time_tv;
struct semaphore handler_sem;
spinlock_t mr_lock;// = SPIN_LOCK_UNLOCKED;
#ifdef JIFFY_TIMER
struct timer_list timer1;
struct timer_list timer2;
#endif
struct hrtimer hr_timer1;
struct hrtimer hr_timer2;
#define MS_TO_NS(x) (x * 1000 * 1000)
#define NS_TO_MS(x) (x / 1000 / 1000)
#define MS_TO_JF(x) (x / 4)
inline void print_warning(char *function_name, char *string)
{
	printk(KERN_EMERG "Nima-HSF ==> in function <%s>: %s.\n", function_name, string);
}
inline void print_int(char *function_name, char *string, int i)
{
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s = %d.\n", function_name, string, i);
}
inline void print_long(char *function_name, char *string, long int i)
{
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s = %ld.\n", function_name, string, i);
}

inline void print_int_int(char *function_name, char *string, int i, int j)
{
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s = %d, %d.\n", function_name, string, i, j);
}
inline void print_long_long(char *function_name, char *string, long i, long j)
{
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s = %ld, %ld.\n", function_name, string, i, j);
}
inline void print_time(char *function_name, char *string, unsigned long i)
{
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s at %lu.\n", function_name, string, i);
}
inline void print_time_now(char *function_name, char *string)
{
	struct timeval time_now;
	do_gettimeofday(&time_now);
// 	time_now = t.tv_sec * 1000 + t.tv_usec / 1000;
	printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s at %lu jiffies, %lu millisecs.\n", function_name, string, jiffies - start_time, (time_now.tv_sec - start_time_tv.tv_sec) *1000 + (time_now.tv_usec - start_time_tv.tv_usec)/1000 );
	print_long("", "now", time_now.tv_usec);
}
inline unsigned long get_time_now(void)
{
	struct timeval time_now;
	do_gettimeofday(&time_now);
	return time_now.tv_sec *1000000 + time_now.tv_usec ;
}
typedef struct _Server Server;
typedef struct _Queue Queue;
typedef struct _Children
{
	struct list_head 	head;
	int id;
	int type;
	struct Server *server;
	struct Task *task;
}Children;
struct Log_Data
{
  int index;
  int cnt[LOG_LENGTH];
  int dl_miss[LOG_LENGTH];
  unsigned long time[LOG_LENGTH];
  unsigned long budget[LOG_LENGTH];
  unsigned long consumed_budget[LOG_LENGTH];
  unsigned long avg_consumed_budget[LOG_LENGTH];
  unsigned long extra_req_budget[LOG_LENGTH];
  unsigned long total_budget[LOG_LENGTH];	
};
struct ANN
{
  int U[NO_NODS];
  int W[NO_INPUT][NO_NODS];
};
struct Application
{
	struct list_head 	head;
	Children	 	servers;
	Children	 	tasks;
	int id;
	int criticality;
	unsigned long budget;
	unsigned long period;
	unsigned long bandwidth;
	struct Queue *ready_queue;
};
struct Server 
{	
	struct list_head 	head;
	Children	 	children;
	int type;
	int id;
	int priority;
	int attached_to_server;
	int cnt;
	int total_child_job;
	int status;
	int dl_miss;
	int control_period;
	int consumed_budget_history_index;
	int criticality;
	int prev_ANN_input_valid;
	int prev_ANN;
	unsigned long budget;
	unsigned long period;
	unsigned long relative_deadline;
	unsigned long abs_deadline;
	unsigned long current_budget;
	unsigned long consumed_budget;
	unsigned long consumed_budget_history[HISTORY_LENGTH];
	unsigned long extra_req_budget_history[HISTORY_LENGTH];
	unsigned long avg_consumed_budget;
	unsigned long extra_req_budget;
	unsigned long total_budget;
	unsigned long timestamp;
	unsigned long timestamp_mic;
	unsigned long budget_ceiling;
	unsigned long prev_ANN_input[NO_INPUT];
	unsigned long prev_estimation;
	struct Queue *ready_queue;
	#ifdef JIFFY_TIMER
	struct timer_list period_timer;
	struct timer_list budget_timer;
	#endif
	struct Server *parent;
	struct Application *parent_app;
	struct Log_Data *log;
	struct ANN my_network_surp;
	struct ANN my_network_def;
};
struct Server server_list;
struct Application app_list;
struct Server cbs_list;
struct Server *active_server;

struct Task {
	struct list_head 	head;
	int id;
	int pid;	/* original linux PID. */
	int priority; 	/* real-time priority. */
	int cpu_id;	/* CPU ID upon which the task is currently running. */
	int state;
	int cnt;
	int timer_satus;
	int dl_miss;
	int missing_dl_flag;
	/* timing properties: */
	unsigned long wcet; 		/* by microseconds */
	unsigned long period; 		/* by jiffies */
	unsigned long release_time;	/* by jiffies */
	unsigned long exec_time;	/* by jiffies */
	unsigned long relative_deadline;
	unsigned long abs_deadline;
	unsigned long stopped_time;	
	unsigned long timestamp;
	unsigned long timestamp_mic;
	unsigned long dl_miss_amount;
	struct task_struct *linux_task;
	#ifdef JIFFY_TIMER
	struct timer_list period_timer;
	#endif
	struct Server *parent;
	struct Application *parent_app;
	struct Server *cbs;
};
struct Task task_list;
struct Task *running_task;
struct Task *stopped_task;
struct Handler_Data
{
  struct hrtimer timer;;
  int id;
  int handler_type;
  struct Server *server;
  struct Task *task;
  unsigned long timestamp;
  struct timespec my_timespec;
};
#ifdef JIFFY_TIMER
inline void insert_timer(struct timer_list *timer, void (*my_handler)(unsigned long),  unsigned long data, unsigned long expire_time)
{
  int timer_status = timer_pending(timer);
  struct Handler_Data *data_handler;
  data_handler = (struct Handler_Data *) data;
  data_handler->timestamp = expire_time;
  //print_int("insert_timer","timer status", timer_status);
  if(&timer->entry == NULL)
  {
	print_warning("insert_timer","timer is NULL");
	scheduling_error = -1;
	return;
  }
  if(expire_time < jiffies)
  {
	print_warning("insert_timer","expire_time < jiffies");
	print_int("insert_timer", "difference", jiffies - expire_time);
	scheduling_error = -1;
	return;
  }
  //print_long("insert_timer","setting timer to", expire_time);
  if(timer_status == 1 && timer->expires == jiffies)
  {
      // Pending
      print_warning("insert_timer","setting timer which expires now");
      timer->data = data;
      mod_timer(timer, expire_time);
  }
  else
  {
      setup_timer_on_stack(timer, my_handler, data);
      mod_timer(timer, expire_time);
  }
}
inline void remove_timer(struct timer_list *timer)
{
//   if(timer->expires == jiffies)
//   {
//      print_warning("remove_timer", "Error: timer expires now");
//      scheduling_error = -1;
//      return;
//   }
    del_timer(timer);
}
#endif
inline void insert_hrtimer(struct hrtimer *timer, enum hrtimer_restart (*my_handler)(struct hrtimer*),  ktime_t *expire_ktime)
{
    timer->function = my_handler;
    //print_int("insert_hrtimer", "queued before", hrtimer_is_queued(timer));
    hrtimer_start(timer, *expire_ktime, HRTIMER_MODE_REL_PINNED);
    //hrtimer_start_expires(timer, HRTIMER_MODE_REL_PINNED);
    //print_int("insert_hrtimer", "queued after", hrtimer_is_queued(timer));
}
inline void remove_hrtimer(struct hrtimer *timer)
{
    hrtimer_cancel(timer);
}
/* ================================Queue=====================================*/
struct Queue_element{
	struct list_head 	head;
	struct Server 		*server;
	struct Task		*task;
	int element_type;		// element can be server or task
};

struct Queue{
	struct Queue_element *elements;
	int queue_type;			// queue can be priority or deadline based
};
struct Queue Global_Ready_Queue;
struct Queue Proc_Local_Ready_Queue[number_of_proc];
struct Queue *Current_Ready_Queue;
static inline int check_task(struct Task *task)
{
  if(task == NULL)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task is NULL");
      #endif
      return RES_FAULT;
  }
  if(&task->head == NULL)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task head is NULL");
      #endif
      return RES_FAULT;
  }
  if(&task->period < 0 || &task->priority < 0 || &task->abs_deadline < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task params are not correct");
      #endif
      return RES_FAULT;
  }
  
  if(&task->cbs != NULL && &task->cbs->current_budget < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task current_budget < 0");
      #endif
      return RES_FAULT;
  }
  return RES_SUCCESS;
}
static inline int check_server(struct Server *server)
{
  if(server == NULL)
  {
      #ifdef DEBUG_SCHED
      print_warning("check server", "server is NULL");
      #endif
      return RES_FAULT;
  }
  if(&server->head == NULL)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check server", "server head is NULL");
      #endif
      return RES_FAULT;
  }
  if(&server->budget < 0 || &server->period < 0 || &server->current_budget < 0 || &server->priority < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check server", "server params are not correct");
      #endif
      return RES_FAULT;
  }
  return RES_SUCCESS;
}
static inline int check_element(struct Queue_element *element)
{
      if (element == NULL || &element->head == NULL)
      {
	  #ifdef DEBUG_SCHED
	  print_warning("check_element", "element head is NULL");
	  #endif
	  return RES_FAULT;
      }
      if(element->element_type == SERVER_TYPE && element->server == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("check_element", "element server is NULL");
	  #endif
	  return RES_FAULT;
      }
      if(element->element_type == TASK_TYPE && element->task == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("check_element", "element task is NULL");
	  #endif
	  return RES_FAULT;
      }
      return RES_SUCCESS;
}

static inline int check_queue(struct Queue *queue)
{
      if (queue == NULL || queue->elements == NULL || queue->elements->head.next == NULL || queue->elements->head.prev == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("check_queue", "queue head is NULL");
	  #endif
	  return RES_FAULT;
      }
      return RES_SUCCESS;
}

static inline int init_queue(struct Queue *queue, int type)
{
	queue->elements = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_KERNEL);
	queue->queue_type = type;
	INIT_LIST_HEAD(&queue->elements->head);
	return RES_SUCCESS;
}
static inline int print_queue(struct Queue *queue)
{
	struct Queue_element *pos;
	print_warning("print_queue", "============ Print Queue ============");
	if (queue == NULL || queue->elements == NULL)
	{
	    print_warning("print_queue", "queue head is NULL");
	    return RES_FAULT;
	}	
	if (&queue->elements->head == NULL)
	{
	    print_warning("print_queue", "queue head is NULL");
	    return RES_FAULT;
	}	
	switch (queue->queue_type){
	case PRIORITY_QUEUE:
		print_warning("print_queue", "priority queue");
		break;
	case DEADLINE_QUEUE:
		print_warning("print_queue", "deadline queue");
		break;
	default:
		print_warning("print_queue", "illegal queue type");
		break;
	}
	if(list_empty(&queue->elements->head))
	{
	    print_warning("print_queue", "queue is empty");
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
	  if(queue->queue_type == PRIORITY_QUEUE)
	  {
		if (pos->element_type == SERVER_TYPE)
		      if(pos->server != NULL)
		      {
			  print_int("print_queue", "SERVER_TYPE, priority", pos->server->priority);
			  print_int("print_queue", "SERVER_TYPE, id", pos->server->id);
		      }
		if (pos->element_type == TASK_TYPE)
		  if(pos->task != NULL)
		  {
		    print_int("print_queue", "TASK_TYPE, priority", pos->task->priority);
		    print_int("print_queue", "TASK_TYPE, id", pos->task->id);
		  }
	  }
	  if(queue->queue_type == DEADLINE_QUEUE)
	  {
		if (pos->element_type == SERVER_TYPE)
		  if(pos->server != NULL)
		  {
			print_int("print_queue", "SERVER_TYPE, deadline", pos->server->abs_deadline);
			print_int("print_queue", "SERVER_TYPE, id", pos->server->id);
		  }
		if (pos->element_type == TASK_TYPE)
		  if(pos->task != NULL)
		  {
			print_int("print_queue", "TASK_TYPE, deadline", pos->task->abs_deadline);
			print_int("print_queue", "TASK_TYPE, id", pos->task->id);
		  }
	  }
	}
	print_warning("print_queue", "========== Print Queue End ==========");
  return RES_SUCCESS;
}
static inline void* search_queue_server(struct Queue *queue, struct Server *server)
{
	struct Queue_element *pos;
	if(check_queue(queue) == RES_FAULT || check_server(server) == RES_FAULT)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "check queue or server failed");
	    #endif
	    return NULL;
	}
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "queue head is NULL");
	    #endif
	    return NULL;
	    
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "queue is empty");
	    #endif
	    return NULL;
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
	    if(pos->server == server)
	      return pos;
	}
	return NULL;
}
static inline void* search_queue_task(struct Queue *queue, struct Task *task)
{
	struct Queue_element *pos;
	#ifdef DEBUG_SCHED
	print_warning("search_queue_task", "start");
	#endif
	if(check_queue(queue) == RES_FAULT || check_task(task) == RES_FAULT)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "check queue or task failed");
	    #endif
	    return NULL;
	}
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "queue head is NULL");
	    #endif
	    return NULL;
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "queue is empty");
	    #endif
	    return NULL;
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
	    if(pos->task == task)
	      return pos;
	}
	return NULL;
}
static inline int _insert_priority(struct Queue *queue, struct Queue_element *element)
{
	struct Queue_element *pos;
	int priority = -1;
	if(element->element_type == SERVER_TYPE)
	    priority = element->server->priority;
	if(element->element_type == TASK_TYPE)
	    priority = element->task->priority;
	if(priority < 0)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_priority", "illigal priority");
	    #endif
	    return RES_FAULT;
	}
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_priority", "queue is empty");
	    #endif
	    list_add_tail(&element->head, &queue->elements->head);
	    return RES_SUCCESS;
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
		if (pos->element_type == SERVER_TYPE)
			if( pos->server->priority > priority)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_priority", "inserted to the ready queue");
			    #endif
			    return RES_SUCCESS;
			}
		if (pos->element_type == TASK_TYPE)
			if( pos->task->priority > priority)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_priority", "inserted to the ready queue");
			    #endif
			    return RES_SUCCESS;
			}		      
	}
	// priority is higher than any other elemenst in the queue
	list_add_tail(&element->head, &queue->elements->head);
  return RES_SUCCESS;
}
static inline int _insert_deadline(struct Queue *queue, struct Queue_element *element)
{
	struct Queue_element *pos;
	long deadline = -1;
	if(element->element_type == SERVER_TYPE)
	    deadline = element->server->abs_deadline;
	if(element->element_type == TASK_TYPE)
	    deadline = element->task->abs_deadline;
	if(deadline < 0)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_deadline", "illigal deadline");
	    #endif
	    return RES_FAULT;
	}
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_deadline", "queue is empty");
	    #endif
	    list_add_tail(&element->head, &queue->elements->head);
	    return RES_SUCCESS;
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
		if (pos->element_type == SERVER_TYPE)
			if( pos->server->abs_deadline > deadline)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_deadline", "inserted to the ready queue");
			    #endif
			    return RES_SUCCESS;
			}
		if (pos->element_type == TASK_TYPE)
			if( pos->task->abs_deadline > deadline)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_deadline", "inserted to the ready queue");
			    #endif
			    return RES_SUCCESS;
			}		      
	}
	// deadline is higher than any other elemenst in the queue
	list_add_tail(&element->head, &queue->elements->head);
  return RES_SUCCESS;
}
static inline int insert_queue(struct Queue *queue, struct Queue_element *element)
{
	int res = RES_SUCCESS;
	if (check_queue(queue) == RES_FAULT || check_element(element) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "check queue or element failed");
		#endif
		return RES_FAULT;
	}
	switch (queue->queue_type){
	case PRIORITY_QUEUE:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "priority queue");
		#endif
		return _insert_priority(queue, element);
		break;
	case DEADLINE_QUEUE:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "deadline queue");
		#endif
		return _insert_deadline(queue, element);
		break;
	default:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "illegal queue type");
		#endif
		break;
		res = RES_FAULT;
	}
	return res;
}
static inline int delete_queue(struct Queue_element *element)
{
	int res = RES_SUCCESS;
	if (&element->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_queue", "element is NULL");
	    #endif
	    return RES_FAULT;
	}	
	list_del(&element->head);
	return res;
}
static inline int delete_whole_queue(struct Queue *queue)
{
	struct Queue_element *pos, *tmp, *element;
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_whole_queue", "queue head is NULL");
	    #endif
	    return RES_FAULT;
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_whole_queue", "queue is empty");
	    #endif
	    return RES_FAULT;
	}
	element = list_entry(queue->elements->head.next, struct Queue_element, head);
	list_for_each_entry_safe(pos, tmp, &element->head, head)
	{
	    if(pos != element)
		delete_queue(pos);
	}
	init_queue(queue, queue->queue_type);
	#ifdef DEBUG_SCHED
	print_queue(queue);
	#endif
	return RES_SUCCESS;
}
/* ==============================Queue end===================================*/
 /*========================================Basic functions====================================================*/
static inline int count_list(struct list_head *head)
{
	int cnt = 0;
	struct list_head *pos;
	if(head == NULL)
		{
		  #ifdef DEBUG_SCHED
		    print_warning("count_list", "head is NULL");
		  #endif
		return -1;
		  
		}
	list_for_each(pos, head)
	{
		cnt++;		
	}
	return cnt;
}
static inline int get_last_server_id(struct list_head *myhead)
{
	struct Server *s;
	s = list_entry(myhead->prev, struct Server, head);
	return s->id;
}
static inline int get_last_app_id(struct list_head *myhead)
{
	struct Application *app;
	app = list_entry(myhead->prev, struct Application, head);
	return app->id;
}

static inline int get_last_task_id(struct list_head *myhead)
{
	struct Task *t;
	t = list_entry(myhead->prev, struct Task, head);
	return t->id;
}
static inline void* find_server(struct list_head *myhead, int id)
{
	struct Server *s;
	struct list_head *pos;
	list_for_each(pos, myhead)
	{
		s = list_entry(pos, struct Server, head);
		if (s->id == id)
			return s;
	}

	return NULL;
}
static inline void* find_task(struct list_head *myhead, int id)
{
	struct Task *t;
	struct list_head *pos;
	list_for_each(pos, myhead)
	{
		t = list_entry(pos, struct Task, head);
		if (t->id == id)
			return t;
	}

	return NULL;
}
static inline void* find_app(struct list_head *myhead, int id)
{
	struct Application *app;
	struct list_head *pos;
	list_for_each(pos, myhead)
	{
		app = list_entry(pos, struct Application, head);
		if (app->id == id)
			return app;
	}

	return NULL;
}
static inline void* find_children(struct list_head *myhead, int id, int type)
{
	Children *c;
	struct list_head *pos;
	list_for_each(pos, myhead)
	{
		c = list_entry(pos, Children, head);
		if (type == c->type && c->id == id)
			return c;
	}

	return NULL;
}
/*
 * Returns 1 if first has higher priority or ealier deadline
 * */
static inline int compare_elements(struct Queue_element *first, struct Queue_element *second, int queue_type)
{
  long first_value, second_value;
  first_value = second_value = -1;
  if(check_element(first) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("compare element", "===**=== ERROR: check_element failed, first");
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(check_element(second) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("compare element", "===**=== ERROR: check_element failed, second");
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(queue_type == PRIORITY_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare element", "prio queue");
      #endif
      if(first->element_type == SERVER_TYPE)
	first_value = first->server->priority;
      if(first->element_type == TASK_TYPE)
	first_value = first->task->priority;
      
      if(second->element_type == SERVER_TYPE)
	second_value = second->server->priority;
      if(second->element_type == TASK_TYPE)
	second_value = second->task->priority;
  }
  if(queue_type == DEADLINE_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare element", "deadline queue");
      #endif
      if(first->element_type == SERVER_TYPE)
	first_value = first->server->abs_deadline;
      if(first->element_type == TASK_TYPE)
	first_value = first->task->abs_deadline;
      
      if(second->element_type == SERVER_TYPE)
	second_value = second->server->abs_deadline;
      if(second->element_type == TASK_TYPE)
	second_value = second->task->abs_deadline;
  }
  #ifdef DEBUG_SCHED
  print_warning("compare element", "Finish");
  #endif
//   print_long("compare_elements", "first", first_value);
//   print_long("compare_elements", "second", second_value);
  if (first_value < second_value)
    return YES;
  return NO;
}

static inline int permission(int function_id)
{
  switch (function_id) 
  {
	  case STOP_TASK_ID:
	  {
		  switch(scheduling_flag)
		  {
		      case SERVER_RELEASE_HANDLER:
			    return YES;
			    break;
		      case SERVER_BUDGET_HANDLER:
			    return YES;
			    break;
		      case TASK_RELEASE_HANDLER:
			    return YES;
			    break;
		      case CBS_BUDGET_HANDLER:
			    return YES;
			    break;
		      case API_CALL_ID:
			    return YES;
			    break;
		  }
	  }
	  case RUN_TASK_ID:
	  {
		  switch(scheduling_flag)
		  {
		      case SERVER_RELEASE_HANDLER:
			    return YES;
			    break;
		      case SERVER_BUDGET_HANDLER:
			    return YES;
			    break;
		      case TASK_RELEASE_HANDLER:
			    return YES;
			    break;
		      case CBS_BUDGET_HANDLER:
			    return YES;
			    break;
		      case API_CALL_ID:
			    return YES;
			    break;
		  }
	  }
	  default: /* illegal function_id. */
		  print_warning("permission", "illegal function_id");
		  break;
  }
  return NO;
}

static inline long division(long first, long second)
{
    if(first != 0 && second != 0)
	return first / second + (0 ? first % second == 0 : 1);
    return 0;
}
static inline unsigned long division_unsigned(unsigned long first, unsigned long second)
{
    if(first != 0 && second != 0)
	return first / second + (0 ? first % second == 0 : 1);
    return 0;
}
static inline unsigned long activation_function(struct Server *server, long input)
{
     if(check_server(server) == RES_FAULT)
     {
       scheduling_error = -1;
       print_warning("activation_function", "server check failed");
       return RES_FAULT;
     }
     
     if(input <= 0)
	return 0;
     if(input > server->period*100)
       return server->period*100;
    return input;
}
static inline void teach_network(struct Server *server, unsigned long input_ann[NO_INPUT], unsigned long estimated_budget, unsigned long real_budget, int net)
{
    
    long out_err, hidden_err[NO_NODS], hidden[NO_NODS];
    int input, node;
    
    out_err = real_budget - estimated_budget;
    print_long_long("","error out, server", out_err, server->id);
      
 if(net==0)
 {
    for(node=0;node<NO_NODS;node++)
    {
      hidden[node] = 0;
      for(input=0;input<NO_INPUT;input++)
      {
  	  hidden[node] +=  activation_function(server, server->my_network_surp.W[input][node]*input_ann[input]);
      }
      hidden_err[node] = out_err * hidden[node];
//       print_long("","error", hidden_err[node]);
//       print_long("","old U", server->my_network_surp.U[node]);
      server->my_network_surp.U[node] += (out_err * Learning_rate/  100);
//       print_long("","new U", server->my_network_surp.U[node]);
      
    }
    for(node=0;node<NO_NODS;node++)
    {
      for(input=0;input<NO_INPUT;input++)
      {
  	  server->my_network_surp.W[input][node] += (hidden_err[node] * Learning_rate/ 10000);
      }
    }
 }
 else
 {//---------------------------
  for(node=0;node<NO_NODS;node++)
    {
      hidden[node] = 0;
      for(input=0;input<NO_INPUT;input++)
      {
  	  hidden[node] +=  activation_function(server, server->my_network_def.W[input][node]*input_ann[input]);
      }
      hidden_err[node] = out_err * hidden[node];
//       print_long("","error", hidden_err[node]);
//       print_long("","old U", server->my_network_def.U[node]);
      server->my_network_def.U[node] += (out_err * Learning_rate/ 100);
//       print_long("","new U", server->my_network_def.U[node]);
      
    }
    for(node=0;node<NO_NODS;node++)
    {
      for(input=0;input<NO_INPUT;input++)
      {
  	  server->my_network_def.W[input][node] += (hidden_err[node] * Learning_rate/ 10000);
      }
    }
 }
  
}

static inline unsigned long ask_network(struct Server *server, unsigned long input_ann[NO_INPUT], int net)
{
  int input, node;
  long hidden[NO_NODS], output;
  if(check_server(server) == RES_FAULT)
      return RES_FAULT;
    
  output = 0;
  
  if(net==0)
  {
  for(node=0;node<NO_NODS;node++)
  {
    hidden[node] = 0;
    for(input=0;input<NO_INPUT;input++)
    {
	hidden[node] +=  activation_function(server, server->my_network_surp.W[input][node]*input_ann[input]);
// 	print_time("", "W", server->my_network.first_W[input][node]);
// 	print_time("", "inp", input_L1[input]);
    }
	
//     	print_long("","hiden", hidden[node]);
  }
  
  for(node=0;node<NO_NODS;node++)
  {
      output += activation_function(server, (server->my_network_surp.U[node]*hidden[node]/ 100));
//       print_long("","0out", output);  
  }
  }
  else
  {
    //-----------------------------------------
      for(node=0;node<NO_NODS;node++)
  {
    hidden[node] = 0;
    for(input=0;input<NO_INPUT;input++)
    {
	hidden[node] +=  activation_function(server, server->my_network_def.W[input][node]*input_ann[input]);
// 	print_time("", "W", server->my_network.first_W[input][node]);
// 	print_time("", "inp", input_L1[input]);
    }
	
//     	print_long("","hiden", hidden[node]);
  }
  
  for(node=0;node<NO_NODS;node++)
  {
      output += activation_function(server, (server->my_network_def.U[node]*hidden[node]/ 100));
//       print_long("","1 out", output);  
  }
 
  }
  output = division(output, 100);
  
  return output;
}

static inline void budget_controller_log(struct Server *server)
{
   if(check_server(server) == RES_FAULT)
   {
      print_warning("budget_controller_log", "server check failed");
      return;
   }
    
  if(server->log == NULL)
   {
     print_warning("budget_controller_log", "server log is null");
      return;
   }
  if(server->log->index < LOG_LENGTH)
  {
    server->log->time[server->log->index] = jiffies - start_time;
    server->log->budget[server->log->index] = server->budget;
    server->log->consumed_budget[server->log->index] = server->consumed_budget;
    server->log->extra_req_budget[server->log->index] = server->extra_req_budget;
    server->log->avg_consumed_budget[server->log->index] = server->avg_consumed_budget;
    server->log->cnt[server->log->index] = server->cnt;
    server->log->dl_miss[server->log->index] = server->dl_miss;
//     print_time("budget_controller_log", "consumed_budget", server->log->consumed_budget[server->log->index]);
  }
  server->log->index++;
}

static inline void my_swap(void* array_add , int first, int second)
{
	unsigned long *array = array_add;
	unsigned long temp;
	temp = array[first];
	array[first] = array[second];
	array[second] = temp;
}
static inline int partition(void* array_add ,int left, int right) 
{
	int storeIndex = left;
	int piv_index;
	int pivot, i;
	unsigned long *array = array_add;
	//choose the pivot
	piv_index = (left + right)/2;
	pivot = array[piv_index];
	my_swap(array, piv_index, right);
	
	for(i=left;i<right;i++)
	{
		if(array[i] < pivot)
		{
			my_swap(array, i, storeIndex);
			storeIndex = storeIndex + 1;
		}
	}
	my_swap(array, storeIndex, right);
	//return pivots position
	return storeIndex;
}

static inline void quicksort(void* array_add , int l, int r)
{
	int pivot;
	if(r>l)
	{
		pivot = partition(array_add, l, r);
		quicksort(array_add, l, pivot-1);
		quicksort(array_add, pivot+1, r);
	}
}

static inline int budget_controller(struct Server *server)
{
    unsigned long newBudget, maxBudget, sum_consumed_budget, sum_extra_req_budget, avg_extra_req_budget, max_consumed_budget, std_consumed_budget;
    struct Server *tmp_server;
    int i, available_bandwidth, low_imp_ser_bandwidth, total_bandwidth, extra_bandwidth;
    struct timespec overhead, tmp_time;
    #ifdef DEBUG_CHECKS
    if(check_server(server) == RES_FAULT)
      return RES_FAULT;
    #endif
    #ifdef MONITOR_WORKLOAD
    budget_controller_log(server); 
    server->consumed_budget = 0;
    server->extra_req_budget = 0;
    return RES_SUCCESS;
    #endif
    
    if(server->id == 0) //This is hard real-time server
    {
      budget_controller_log(server); 
      server->consumed_budget = 0;
      server->extra_req_budget = 0;
      return RES_SUCCESS;
    }
   /* 
    if(server->parent == NULL)
      return RES_SUCCESS;*/
    if((server->cnt % server->control_period) != 0)
    {
      /*reseting parameters before returning*/
//       server->consumed_budget = 0;
//       server->extra_req_budget = 0;
      return RES_SUCCESS;
    }
//     tmp_time = current_kernel_time();
    getnstimeofday(&tmp_time);
    // Overload controller
    if(server->parent == NULL)
	total_bandwidth = 100;
    else
    {
	total_bandwidth = division(server->parent->budget*100, server->parent->period);
    }
    available_bandwidth = total_bandwidth;
    for(i=0;i<server->id;i++)
    {
	tmp_server = find_server(&server_list.head, i);
	#ifdef DEBUG_CHECKS
	if(check_server(tmp_server) == RES_FAULT)
	{
	    print_warning("budget_controller", "check server failed");
	    return RES_FAULT;
	}
	#endif
// 	print_int("budget_controller","bandwidth", (tmp_server->budget*100)/tmp_server->period);
	if(server->parent == tmp_server->parent)
	  available_bandwidth -= (tmp_server->budget*100)/tmp_server->period;
    }
    low_imp_ser_bandwidth = 0;
    for(i=server->id+1;i<=get_last_server_id(&server_list.head);i++)
    {
      tmp_server = find_server(&server_list.head, i);
	#ifdef DEBUG_CHECKS
	if(check_server(tmp_server) == RES_FAULT)
	{
	    print_warning("budget_controller", "check server failed");
	    return RES_FAULT;
	}
	#endif
// 	print_int("budget_controller","bandwidth", (tmp_server->budget*100)/tmp_server->period);
	if(server->parent == tmp_server->parent)
	  low_imp_ser_bandwidth += (tmp_server->budget*100)/tmp_server->period;
    }
//     print_int_int("budget_controller", "low_imp_ser_bandwidth", server->id, low_imp_ser_bandwidth);
//     print_long_long("","history consumed", server->id, server->consumed_budget);
//     print_long_long("","history extra", server->id, server->extra_req_budget);
    if(server->consumed_budget_history_index < HISTORY_LENGTH)
    {
	if(server->extra_req_budget == 0)
	    server->consumed_budget_history[server->consumed_budget_history_index] = division(server->consumed_budget, server->control_period);
	else
	  server->consumed_budget_history[server->consumed_budget_history_index] = server->budget + division(server->extra_req_budget, server->control_period);
	
	server->extra_req_budget_history[server->consumed_budget_history_index] = division(server->extra_req_budget, server->control_period);
	
	server->consumed_budget_history_index++;
    }
    else
    {
	for(i=0;i<server->consumed_budget_history_index-1;i++)
	{
	    server->consumed_budget_history[i] = server->consumed_budget_history[i+1];
	    server->extra_req_budget_history[i] = server->extra_req_budget_history[i+1];
	}
	if(server->extra_req_budget == 0)
	      server->consumed_budget_history[i] = division(server->consumed_budget, server->control_period);
	else
	      server->consumed_budget_history[i] = server->budget + division(server->extra_req_budget, server->control_period);
	
	server->extra_req_budget_history[i] = division(server->extra_req_budget, server->control_period);
    }
    
    sum_consumed_budget = 0;
    sum_extra_req_budget = 0;
    max_consumed_budget = server->consumed_budget_history[server->consumed_budget_history_index-1];
    for(i=0;i<server->consumed_budget_history_index;i++)
    {
// 	/*print_warning("","---------------history");
// 	print_long_long("","history", server->id, server->consumed_budget_history[i]);
// 	print_warning("","---------------history");*/
	sum_consumed_budget += server->consumed_budget_history[i];
	sum_extra_req_budget += server->extra_req_budget_history[i];
	if(server->consumed_budget_history_index/2 < i && max_consumed_budget < server->consumed_budget_history[i])
	    max_consumed_budget = server->consumed_budget_history[i];
    }
//     max_consumed_budget = division(max_consumed_budget ,server->control_period);
    server->avg_consumed_budget = division(sum_consumed_budget, i);
    avg_extra_req_budget = division(sum_extra_req_budget, i);
    std_consumed_budget = 0;
    for(i=0;i<server->consumed_budget_history_index;i++)
    {
	if(server->avg_consumed_budget > server->consumed_budget_history[i])
	      std_consumed_budget += server->avg_consumed_budget - server->consumed_budget_history[i];
	else
	      std_consumed_budget += server->consumed_budget_history[i] - server->avg_consumed_budget;
    }
    std_consumed_budget = division(std_consumed_budget, i);
    //-------------------- ANN
//     if(server->consumed_budget_history_index>=NO_INPUT)
//     {
// 	print_time("", "max_consumed_budget ", max_consumed_budget );
// 	print_time("", "avg_consumed_budget", server->avg_consumed_budget);
	
// 	if(sum_extra_req_budget > 0)
// 	    ANN_input[1] =  max_consumed_budget;// - server->avg_consumed_budget;
// 	else
// 	    ANN_input[1] =  0;
	
// 	ann_index = NO_INPUT ;
// 	for(i=server->consumed_budget_history_index-1;i>server->consumed_budget_history_index-NO_INPUT-1;i--)
// 	{	  
// 	     ann_index--;
// 	     ANN_input[ann_index] = server->consumed_budget_history[i] ;
// // 	     print_long("not","ann input", ANN_input[ann_index]);
// 	}
//  	quicksort((void *)&ANN_input, 0, NO_INPUT-1);
// 	ann_index = NO_INPUT ;
// 	for(i=server->consumed_budget_history_index-1;i>server->consumed_budget_history_index-NO_INPUT-1;i--)
// 	{	  
// 	     ann_index--;
// 	     ANN_input[ann_index] = server->consumed_budget_history[i] ;
// 	     print_long("sorted","ann input", ANN_input[ann_index]);
// 	}
// 	ask_network(server, ANN_input);
    
// 	if(server->prev_ANN_input_valid == 1)
// 	    teach_network(server, server->prev_ANN_input, server->prev_estimation, server->avg_consumed_budget, server->prev_ANN);
// 	print_long("","real consumed budget", server->avg_consumed_budget);
// 	print_long("","assigned budget", server->budget);
// 	print_int("","server id", server->id);
// 	ANN_input[0] = max_consumed_budget;
// 	ANN_input[1] = server->avg_consumed_budget;
// 	if(avg_extra_req_budget>0)
// 	  net = 1;
// 	else
// 	  net = 0;
	
// 	for(i=0;i<NO_INPUT;i++)
// 	{
// 	    server->prev_ANN_input[i] = ANN_input[i];
// 	    print_long("", "ANN input", ANN_input[i]);
// 	}
	
// 	server->prev_ANN_input_valid = 1;
// 	server->prev_ANN = net;
// 	newBudget = ask_network(server, ANN_input, net);
// 	print_long_long("", "ANN output, net", newBudget, net);
// 	server->prev_estimation = newBudget;
	
//     }
//     else
// 	newBudget = (server->avg_consumed_budget ) ;
    
    newBudget = server->avg_consumed_budget  + std_consumed_budget/2;
    
    
    newBudget += avg_extra_req_budget; //server->extra_req_budget_history[server->consumed_budget_history_index-1];
    
//     print_time("", "avg_extra_req_budgett", avg_extra_req_budget);
    
    maxBudget = available_bandwidth * server->period / 100;
//     print_long("", "maxBudget", maxBudget);
    
    if(newBudget <= 0)
      newBudget = 1;
    if(newBudget > maxBudget)
    {
//       scheduling_error = -1;
//       print_int_int("budget_controller", "NOT enough budget", server->id, server->cnt);
      newBudget = maxBudget;
    }
    server->budget = newBudget;
//     print_long("","new budget (avg)", newBudget);
    if(low_imp_ser_bandwidth > total_bandwidth-division(newBudget*100, server->period))
    {
// 	print_int("", "Overload!!!", server->id);
	extra_bandwidth = total_bandwidth-division(newBudget*100, server->period);
	for(i=get_last_server_id(&server_list.head);i>server->id;i--)
	{
	  tmp_server = find_server(&server_list.head, i);
	    #ifdef DEBUG_CHECKS
	    if(check_server(tmp_server) == RES_FAULT)
	    {
		print_warning("budget_controller", "check server failed");
		return RES_FAULT;
	    }
	    #endif
    // 	print_int("budget_controller","bandwidth", (tmp_server->budget*100)/tmp_server->period);
	    if(server->parent == tmp_server->parent)
	    {
	      print_int_int("budget_controller", "server1 throttling server2", server->id, tmp_server->id);
	      if(tmp_server->budget*100/tmp_server->period >= extra_bandwidth)
	      {
// 		print_time("budget_controller","tmp_budget", tmp_server->budget);
		tmp_server->budget = (tmp_server->budget*100/tmp_server->period - extra_bandwidth)*server->period/100;
// 		print_time("budget_controller","tmp_budget after", tmp_server->budget);
		break;
	      }
	      else
	      {
		extra_bandwidth -= tmp_server->budget*100/tmp_server->period;
		tmp_server->budget = 0;
	      }
	    }
	}
    }
    server->extra_req_budget = avg_extra_req_budget;
    
    getnstimeofday(&overhead);
    overhead = timespec_sub(overhead, tmp_time);
    control_overhead+= overhead.tv_sec*1000000 + overhead.tv_nsec;
//     overhead = current_kernel_time();
//     print_long_long("budget_controller", "Overhead==================>>>>>>", overhead.tv_sec, overhead.tv_nsec);
    #ifdef LOG
    budget_controller_log(server); 
    #endif
    /*reseting parameters before returning*/
    server->consumed_budget = 0;
    server->extra_req_budget = 0;
    /*
    print_int("", "myi", i);
    print_long("", "sum_consumed_budget", sum_consumed_budget);
    print_long("", "avg_consumed_budget", server->avg_consumed_budget);
    scheduling_error = -1;	
    return RES_FAULT;
    */
   
    return RES_SUCCESS;
}
/* ==============================For API calls ==============================*/
static inline int api_create_server(struct API_Data data);
static inline int api_set_server_param(struct API_Data data);
static inline int api_attach_server_to_server(struct API_Data data);
static inline int api_create_task(void);
static inline int api_attach_task_to_server(struct API_Data data);
static inline int api_release_server(struct API_Data data);
static inline int api_kill_server(struct API_Data data);
static inline int api_stop(void);
static inline int api_run(void);
static inline int api_attach_task_to_mod(struct API_Data data);
static inline int api_set_task_param(struct API_Data data);
static inline int api_release_task(struct API_Data data);
static inline int api_detach_task(struct API_Data data);
static inline int api_detach_server(struct API_Data data);
static inline int api_task_finish_job(struct API_Data data);
#ifdef TEST_TIMER
static inline int api_test_timer(struct API_Data data);
#endif
static inline int api_print_log(struct API_Data data);
//Multiprocessor
static inline int api_create_app(struct API_Data data);
static inline int api_attach_server_to_app(struct API_Data data);
static inline int api_attach_task_to_app(struct API_Data data);

static inline int stop_server(struct Server *server);
static inline int run_queue(struct Queue *queue);
static inline int try_to_run_server(struct Server *server);
static inline void* if_outrank(struct Server *first, struct Server *second);
static inline int is_ancestor(struct Server *first, struct Server *second);
static inline int release_server(struct Server *server);
static inline int stop_task(struct Task *task);
static inline int try_to_run_task(struct Task *task);
static inline int release_hsf_task(struct Task *task);
static inline int run_task(struct Task *task);

/* dummy function. */
static int hsf_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/* dummy function. */
static int hsf_release(struct inode *inode, struct file *filp)
{
	return 0;
}
/**
 * tests are done through this function.
 * @cmd holds the test command number.
 * @buf holds the values.
 * See api.h for details.
 */
static ssize_t hsf_write(struct file *file, const char *buf, 
						   size_t count, loff_t *offset)
{
	print_warning("hsf_write", "write");
	return RES_SUCCESS;
}
/**
 * Timing properties are set through this function.
 * @cmd holds the API number.
 * @arg holds the value.
 * See api.h for details.
 */
/*static int hsf_ioctl(struct inode *inode,
					   struct file *file,
					   unsigned int cmd, 
					   unsigned long arg)
adapted to new kernel unlocked_ioctl*/
static long hsf_ioctl(struct file *file,
					   unsigned int cmd, 
					   unsigned long arg)
{
	long res = -1;
	struct API_Data data;
	//local_irq_disable();
	/* copy data to kernel buffer. */
	if (copy_from_user(&data, (long *)arg, sizeof(struct API_Data))) {
		print_warning("hsf_ioctl", "failed to copy data");
		return -EFAULT;
	}
	//print_int("api call", cmd);
	  if(Module_status == MOD_STOP && cmd != API_PRINT_LOG)
	  {	  
	      if(cmd != API_TASK_FINISH_JOB)
	      {
		  print_warning("handler_server_release","Module has been stopped!!!");
		  api_stop();
	      }
	      return -EFAULT;
	  }
	  if(scheduling_error == -1 && cmd != API_PRINT_LOG)
	  {	  
	      print_warning("hsf_ioctl","scheduling error has happend!!!");
	      api_stop();
	      return -EFAULT;
	  }

	/*Choose function here*/
	//if (down_interruptible(&handler_sem) == 0)
// 	spin_lock_irq(&mr_lock);
	{
	  switch (cmd) {
	  case API_CREATE_SERVER:
		  res = api_create_server(data);
		  break;
	  case API_SET_SERVER_PARAM:
		  res = api_set_server_param(data);
		  break;
	  case API_ATTACH_SERVER_TO_SERVER:
		  res = api_attach_server_to_server(data);
		  break;
	  case API_CREATE_TASK:
		  res = api_create_task();
		  break;
	  case API_ATTACH_TASK_TO_SERVER:
		  res = api_attach_task_to_server(data);
		  break;
	  case API_RELEASE_SERVER:
		  res = api_release_server(data);
		  break;
	  case API_KILL_SERVER:
		  res = api_kill_server(data);
		  break;
	  case API_STOP:
		  res = api_stop();
		  break;		
	  case API_RUN:
		  res = api_run();
		  break;		
	  case API_ATTACH_TASK_TO_MOD:
		  res = api_attach_task_to_mod(data);
		  break;		
	  case API_SET_TASK_PARAM:
		  res = api_set_task_param(data);
		  break;		
	  case API_RELEASE_TASK:
		  res = api_release_task(data);
		  break;		
	  case API_DETACH_TASK:
		  res = api_detach_task(data);
		  break;		
	case API_DETACH_SERVER:
		  res = api_detach_server(data);
		  break;	
	case API_TASK_FINISH_JOB:
		  res = api_task_finish_job(data);
		  break;
	#ifdef TEST_TIMER
	case API_TEST_TIMER:
		  res = api_test_timer(data);
		  break;
	#endif
	case API_PRINT_LOG:
		  res = api_print_log(data);
		  break;
	case API_CREATE_APP:
		  res = api_create_app(data);
		  break;
	case API_ATTACH_SERVER_TO_APP:
		  res = api_attach_server_to_app(data);
		  break;
	case API_ATTACH_TASK_TO_APP:
		  res = api_attach_task_to_app(data);
		  break;
	  default: /* illegal api identifier. */
		  res = RES_ILLEGAL;
		  print_warning("hsf_ioctl", "illegal API");
		  break;
	  }
	}
	//up(&handler_sem);
	scheduling_flag = API_CALL_ID;
	//local_irq_enable();
// 	spin_unlock_irq(&mr_lock);
	return res;
}
static struct file_operations hsf_fops = {
	.owner = THIS_MODULE,
	.open = hsf_open, /* do nothing but must exist. */
	.release = hsf_release, /* do nothing but must exist. */
	.read = NULL,
	.write = hsf_write,
	//.ioctl = hsf_ioctl, // .unlocked_ioctl 
	.unlocked_ioctl = hsf_ioctl,// adapted to the new kernel
};
/* ===================================<End>=================================*/

/**
 * change the scheduling policy and p->rt_priority in the internal of
 * the Linux kernel, using sched_setscheduler(). 
 */
static inline int __change_prio(struct task_struct *p, int prio)
{
	struct sched_param sp;
	int result;
	sp.sched_priority = prio;
	//print_int("change_prio", "max prio", sched_get_priority_max(SCHED_FIFO));
	set_current_state(TASK_INTERRUPTIBLE);
	result = sched_setscheduler(p, SCHED_FIFO, &sp);
	if (result < 0) 
	{
		print_int("__change_prio", "failed to change priority, task pid:", p->pid);
		print_int("__change_prio", "failed to change priority, my pid:", current->pid);
		scheduling_error = -1;
		kthread_stop(p);
		return false;
	}
	wake_up_process(p);
	kthread_stop(p);
	return true;
}

/**
 * change the priority of the given task, and save it to @rt->prio. 
 */
static inline int change_prio(void *data)
{
	struct Task *task;
	task = (struct Task *) data;
	if (!__change_prio(task->linux_task, task->priority)) {
		return false;
	}
    
	return true;
}
