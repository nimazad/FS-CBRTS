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
int slacks[number_of_proc];
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
inline void print_warning(char *function_name, char *string, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s.\n", jiffies - start_time, function_name, string);
	}
	
}
inline void print_int(char *function_name, char *string, int i, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s = %d.\n", jiffies - start_time, function_name, string, i);
	}
}

inline void print_long(char *function_name, char *string, long int i, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s = %ld.\n", jiffies - start_time, function_name, string, i);
	}
}

inline void print_int_int(char *function_name, char *string, int i, int j, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s = %d, %d.\n", jiffies - start_time, function_name, string, i, j);
	}
}
inline void print_long_long(char *function_name, char *string, long i, long j, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s = %ld, %ld.\n", jiffies - start_time, function_name, string, i, j);
	}
}
inline void print_time(char *function_name, char *string, unsigned long i, int active)
{
	//struct timeval time_now;
	if(DEBUG)
	    active = YES;
	if(active == YES)
	{
	    //do_gettimeofday(&time_now);
	    printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: %s at %lu.\n", jiffies - start_time, function_name, string, i);
	}
}
/*inline void print_time_now(char *function_name, char *string, int active)
{
	struct timeval time_now;
	if(active == YES || DEBUG)
	{
	    do_gettimeofday(&time_now);
	    //time_now = t.tv_sec * 1000 + t.tv_usec / 1000;
	    printk(KERN_WARNING "Nima-HSF ==> in function <%s>: %s at %lu jiffies, %lu millisecs.\n", function_name, string, jiffies - start_time, (time_now.tv_sec - start_time_tv.tv_sec) *1000 + (time_now.tv_usec - start_time_tv.tv_usec)/1000 );
	    //print_long("", "now", time_now.tv_usec, YES);
	}
}*/
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
  int time[LOG_LENGTH];
  int dl_miss[LOG_LENGTH];
  int alpha[LOG_LENGTH];
  int beta[LOG_LENGTH];
  int mu[LOG_LENGTH];
  int period[LOG_LENGTH];
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
	int importance;
	int dl_miss;
	int priority;
	unsigned long budget;
	unsigned long period;
	int total_budget;
	int bandwidth;
	int useful_budget;
	int mu;
	int proc_list[number_of_proc];
	struct Queue *ready_queue;
	struct Log_Data 	*log;
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
	int status;
	int dl_miss;
	int proc_id;
	unsigned long budget;
	unsigned long period;
	unsigned long relative_deadline;
	unsigned long abs_deadline;
	int current_budget;
	unsigned long consumed_budget_history[HISTORY_LENGTH];
	unsigned long extra_req_budget_history[HISTORY_LENGTH];
	unsigned long timestamp;
	unsigned long timestamp_beta;
	unsigned long timestamp_mic;
	#ifdef JIFFY_TIMER
	struct timer_list period_timer;
	struct timer_list budget_timer;
	#endif
	struct Application 	*parent_app;
	struct Task 		*running_task;
};
struct Server server_list;
struct Application app_list;
struct Server cbs_list;
struct Server *active_server[number_of_proc];

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
	int queued_releases;
	/* timing properties: */
	unsigned long wcet; 		/* by microseconds */
	unsigned long period; 		/* by jiffies */
	unsigned long release_time;	/* by jiffies */
	unsigned long next_release_time;/* by jiffies */
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
	struct Server *parent_server;
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
inline int insert_timer(struct timer_list *timer, void (*my_handler)(unsigned long),  unsigned long data, unsigned long expire_time)
{
  int timer_status = timer_pending(timer);
  struct Handler_Data *data_handler;
  data_handler = (struct Handler_Data *) data;
  data_handler->timestamp = expire_time;
  //print_int("insert_timer","timer status", timer_status);
  if(&timer->entry == NULL)
  {
	print_warning("insert_timer","===**=== ERROR: timer is NULL", YES);
	scheduling_error = -1;
	return RES_FAULT;
  }
  if(expire_time < jiffies)
  {
	print_warning("insert_timer","===**=== ERROR: expire_time < jiffies", YES);
	print_int("insert_timer", "difference", jiffies - expire_time, NO);
	scheduling_error = -1;
	return RES_FAULT;
  }
  //print_long("insert_timer","setting timer to", expire_time);
  if(timer_status == 1 && timer->expires == jiffies)
  {
      // Pending
      print_warning("insert_timer","setting timer which expires now", NO);
      timer->data = data;
      mod_timer(timer, expire_time);
  }
  else
  {
      setup_timer_on_stack(timer, my_handler, data);
      mod_timer(timer, expire_time);
  }
  return RES_SUCCESS;
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
      print_warning("check task", "task is NULL", NO);
      #endif
      return RES_FAULT;
  }
  if(&task->head == NULL)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task head is NULL", NO);
      #endif
      return RES_FAULT;
  }
  if(&task->period < 0 || &task->priority < 0 || &task->abs_deadline < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task params are not correct", NO);
      #endif
      return RES_FAULT;
  }
  
  if(&task->cbs != NULL && &task->cbs->current_budget < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check task", "task current_budget < 0", NO);
      #endif
      return RES_FAULT;
  }
  return RES_SUCCESS;
}
static inline int check_server(struct Server *server)
{
  //print_warning("check server", "starts");

  if(server == NULL)
  {
      #ifdef DEBUG_SCHED
      print_warning("check server", "server is NULL", NO);
      #endif
      return RES_FAULT;
  }
  if(&server->head == NULL)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check server", "server head is NULL", NO);
      #endif
      return RES_FAULT;
  }
  if(&server->budget < 0 || &server->period < 0 || &server->current_budget < 0 || &server->priority < 0)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("check server", "server params are not correct", NO);
      #endif
      return RES_FAULT;
  }
  //print_warning("check server", "ends");
  return RES_SUCCESS;
}
static inline int check_element(struct Queue_element *element)
{
      if (element == NULL || &element->head == NULL)
      {
	  #ifdef DEBUG_SCHED
	  print_warning("check_element", "element head is NULL", NO);
	  #endif
	  return RES_FAULT;
      }
      if(element->element_type == SERVER_TYPE && element->server == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("check_element", "element server is NULL", NO);
	  #endif
	  return RES_FAULT;
      }
      if(element->element_type == TASK_TYPE && element->task == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("check_element", "element task is NULL", NO);
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
	  print_warning("check_queue", "queue head is NULL", NO);
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
	print_warning("print_queue", "============ Print Queue ============", YES);
	if (queue == NULL || queue->elements == NULL)
	{
	    print_warning("print_queue", "queue head is NULL", YES);
	    return RES_FAULT;
	}	
	if (&queue->elements->head == NULL)
	{
	    print_warning("print_queue", "queue head is NULL", YES);
	    return RES_FAULT;
	}	
	switch (queue->queue_type){
	case PRIORITY_QUEUE:
		print_warning("print_queue", "priority queue", YES);
		break;
	case DEADLINE_QUEUE:
		print_warning("print_queue", "deadline queue", YES);
		break;
	default:
		print_warning("print_queue", "illegal queue type", YES);
		break;
	}
	if(list_empty(&queue->elements->head))
	{
	    print_warning("print_queue", "queue is empty", YES);
	}
	list_for_each_entry(pos, &queue->elements->head, head)
	{
	  if(queue->queue_type == PRIORITY_QUEUE)
	  {
		if (pos->element_type == SERVER_TYPE)
		      if(pos->server != NULL)
		      {
			  print_int("print_queue", "SERVER_TYPE, priority", pos->server->priority, YES);
			  print_int("print_queue", "SERVER_TYPE, id", pos->server->id, YES);
		      }
		if (pos->element_type == TASK_TYPE)
		  if(pos->task != NULL)
		  {
		    print_int("print_queue", "TASK_TYPE, priority", pos->task->priority, YES);
		    print_int("print_queue", "TASK_TYPE, id", pos->task->id, YES);
		  }
	  }
	  if(queue->queue_type == DEADLINE_QUEUE)
	  {
		if (pos->element_type == SERVER_TYPE)
		  if(pos->server != NULL)
		  {
			print_int("print_queue", "SERVER_TYPE, deadline", pos->server->abs_deadline, YES);
			print_int("print_queue", "SERVER_TYPE, id", pos->server->id, YES);
		  }
		if (pos->element_type == TASK_TYPE)
		  if(pos->task != NULL)
		  {
			print_int("print_queue", "TASK_TYPE, deadline", pos->task->abs_deadline, YES);
			print_int("print_queue", "TASK_TYPE, id", pos->task->id, YES);
		  }
	  }
	}
	print_warning("print_queue", "========== Print Queue End ==========", YES);
  return RES_SUCCESS;
}
static inline void* search_queue_server(struct Queue *queue, struct Server *server)
{
	struct Queue_element *pos;
	if(check_queue(queue) == RES_FAULT || check_server(server) == RES_FAULT)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "check queue or server failed", NO);
	    #endif
	    return NULL;
	}
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "queue head is NULL", NO);
	    #endif
	    return NULL;
	    
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_server", "queue is empty", NO);
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
	print_warning("search_queue_task", "start", NO);
	#endif
	if(check_queue(queue) == RES_FAULT || check_task(task) == RES_FAULT)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "check queue or task failed", NO);
	    #endif
	    return NULL;
	}
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "queue head is NULL", NO);
	    #endif
	    return NULL;
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("search_queue_task", "queue is empty", NO);
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
	    print_warning("_insert_priority", "illigal priority", NO);
	    #endif
	    return RES_FAULT;
	}
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_priority", "queue is empty", NO);
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
			    print_warning("_insert_priority", "inserted to the ready queue", NO);
			    #endif
			    return RES_SUCCESS;
			}
		if (pos->element_type == TASK_TYPE)
			if( pos->task->priority > priority)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_priority", "inserted to the ready queue", NO);
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
	    print_warning("_insert_deadline", "illigal deadline", NO);
	    #endif
	    return RES_FAULT;
	}
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("_insert_deadline", "queue is empty", NO);
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
			    print_warning("_insert_deadline", "inserted to the ready queue", NO);
			    #endif
			    return RES_SUCCESS;
			}
		if (pos->element_type == TASK_TYPE)
			if( pos->task->abs_deadline > deadline)
			{
			    list_add_tail(&element->head, &pos->head);
			    #ifdef DEBUG_SCHED
			    print_warning("_insert_deadline", "inserted to the ready queue", NO);
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
		print_warning("insert_queue", "check queue or element failed", NO);
		#endif
		return RES_FAULT;
	}
	switch (queue->queue_type){
	case PRIORITY_QUEUE:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "priority queue", NO);
		#endif
		return _insert_priority(queue, element);
		break;
	case DEADLINE_QUEUE:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "deadline queue", NO);
		#endif
		return _insert_deadline(queue, element);
		break;
	default:
		#ifdef DEBUG_SCHED
		print_warning("insert_queue", "illegal queue type", NO);
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
	    print_warning("delete_queue", "element is NULL", YES);
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
	    print_warning("delete_whole_queue", "queue head is NULL", NO);
	    #endif
	    return RES_FAULT;
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_whole_queue", "queue is empty", NO);
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
static inline int delete_server_from_queue(struct Queue *queue, struct Server *server)
{
	struct Queue_element *pos, *tmp, *element;
	if (&queue->elements->head == NULL)
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_server_from_queue", "===**=== ERROR:queue head is NULL", YES);
	    #endif
	    scheduling_error = -1;
	    return RES_FAULT;
	}	
	if(list_empty(&queue->elements->head))
	{
	    #ifdef DEBUG_SCHED
	    print_warning("delete_server_from_queue", "queue is empty", YES);
	    #endif
	    return RES_SUCCESS;
	}
	tmp = NULL;
	print_queue(queue);
	element = list_entry(queue->elements->head.next, struct Queue_element, head);
	list_for_each_entry(pos, &queue->elements->head, head)
	{
	    if(pos->server == server)
		tmp = pos;
	}
	if(tmp != NULL)
	    delete_queue(tmp);
	//init_queue(queue, queue->queue_type);
	#ifdef DEBUG_SCHED
	print_queue(queue);
	#endif
	return RES_SUCCESS;
}
/* ==============================Queue end===================================*/
 /*========================================Basic functions====================================================*/
 static inline int if_scheduling_error(char *string)
{
    if(scheduling_error == -1)
    {
      print_warning(string, "scheduling error has happened!", YES);
      return YES;
    }
   return NO;
}
 static inline int count_list(struct list_head *head)
{
	int cnt = 0;
	struct list_head *pos;
	if(head == NULL)
		{
		  #ifdef DEBUG_SCHED
		    print_warning("count_list", "head is NULL", NO);
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
	//print_warning("find_server", "starts");
	struct Server *s;
	struct list_head *pos;
	list_for_each(pos, myhead)
	{
		s = list_entry(pos, struct Server, head);
		if (s->id == id)
			return s;
	}
	//print_warning("find_server", "ends");
	return NULL;
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

static void find_slacks(struct list_head *myhead)
{
	//print_warning("find_server", "starts");
	int i;
	struct Server *s;
	struct list_head *pos;
	for(i=0;i<number_of_proc;i++)
	{
	    slacks[i] = 100;
	}
	list_for_each(pos, myhead)
	{
		s = list_entry(pos, struct Server, head);
		if(SERVER_DETACHED != s->status)
		{
		      slacks[s->proc_id] -= division(s->budget*100, s->period);
		      print_int_int("find_slacks","bandwidth", division(s->budget*100, s->period), slacks[s->proc_id], YES);
		}
		else
		  print_int("find_slacks","server is detached",s->id, YES);
	}
	//print_warning("find_server", "ends");
}
static int schedulable(struct list_head *myhead)
{
	//print_warning("find_server", "starts");
	int i;
	struct Server *s;
	struct list_head *pos;
	int u[number_of_proc];
	for(i=0;i<number_of_proc;i++)
	{
	    u[i] = 0;
	}
	list_for_each(pos, myhead)
	{
		s = list_entry(pos, struct Server, head);
		if(SERVER_DETACHED != s->status)
		{
		      u[s->proc_id] += (s->budget*100)/s->period;
		      printk(KERN_WARNING "Nima-HSF [%lu]==> in function <%s>: = proc: %d, app: %d, u_total: %d, u: %ld budget %ld, period %ld.\n", jiffies - start_time, "schedulable", s->proc_id, s->parent_app->id, u[s->proc_id], (s->budget*100)/s->period, s->budget, s->period);
		      //print_int_int("schedulable","bandwidth", s->id, (s->budget*100)/s->period, YES);
		      //print_int_int("schedulable","u ", s->proc_id, u[s->proc_id], YES);
		}
		else
		  print_int("schedulable","server is detached",s->id, YES);
		if(u[s->proc_id] > 100)
		{
		      print_int("schedulable","===**===ERROR: utilization more than 100",s->proc_id, YES);
		      scheduling_error = -1;
		      return RES_FAULT;
		}
	}
	return RES_SUCCESS;
	//print_warning("find_server", "ends");
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
      print_warning("compare element", "===**=== ERROR: check_element failed, first", YES);
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(check_element(second) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("compare element", "===**=== ERROR: check_element failed, second", YES);
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(queue_type == PRIORITY_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare element", "prio queue", NO);
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
      print_warning("compare element", "deadline queue", NO);
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
  print_warning("compare element", "Finish", NO);
  #endif
//   print_long("compare_elements", "first", first_value);
//   print_long("compare_elements", "second", second_value);
  if (first_value < second_value)
    return YES;
  return NO;
}

static inline int compare_tasks(struct Task *first, struct Task *second, int queue_type)
{
  long first_value, second_value;
  first_value = second_value = -1;
  if(second == NULL)
  {
      print_warning("compare_tasks", "second is null", NO);
      return YES;
  }
  if(queue_type == PRIORITY_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare_tasks", "prio queue", NO);
      #endif
      first_value = first->priority;
      second_value = second->priority;
  }
  if(queue_type == DEADLINE_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare_tasks", "deadline queue", NO);
      #endif
      first_value = first->abs_deadline;
      second_value = second->abs_deadline;
  }
  #ifdef DEBUG_SCHED
  print_warning("compare_tasks", "Finish", NO);
  #endif
//   print_long("compare_elements", "first", first_value);
//   print_long("compare_elements", "second", second_value);
  if (first_value < second_value)
    return YES;
  return NO;
}

static inline int compare_servers(struct Server *first, struct Server *second, int queue_type)
{
  long first_value, second_value;
  first_value = second_value = -1;
  if(second == NULL)
  {
      print_warning("compare_servers", "second is null", NO);
      return YES;
  }
  if(queue_type == PRIORITY_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare_servers", "prio queue", NO);
      #endif
      first_value = first->priority;
      second_value = second->priority;
  }
  if(queue_type == DEADLINE_QUEUE)
  {
      #ifdef DEBUG_SCHED
      print_warning("compare_servers", "deadline queue", NO);
      #endif
      first_value = first->abs_deadline;
      second_value = second->abs_deadline;
  }
  #ifdef DEBUG_SCHED
  print_warning("compare_servers", "Finish", NO);
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
		  print_warning("permission", "illegal function_id", NO);
		  break;
  }
  return NO;
}

static inline unsigned long activation_function(struct Server *server, long input)
{
     if(check_server(server) == RES_FAULT)
     {
       scheduling_error = -1;
       print_warning("activation_function", "===**=== ERROR: server check failed", YES);
       return RES_FAULT;
     }
     
     if(input <= 0)
	return 0;
     if(input > server->period*100)
       return server->period*100;
    return input;
}


static inline void budget_controller_log(struct Server *server)
{
  /* if(check_server(server) == RES_FAULT)
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
  server->log->index++;*/
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
  return RES_SUCCESS;
}
static inline int stop_task(struct Task *task);
static inline int run_first_element(struct Queue *queue);
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

static inline int api_test_timer(struct API_Data data);
static inline int api_print_log(struct API_Data data);
//Multiprocessor
static inline int api_create_app(struct API_Data data);
static inline int api_attach_server_to_app(struct API_Data data);
static inline int api_attach_task_to_app(struct API_Data data);
static inline int api_intra_app_sched_event(struct API_Data data);
static inline int api_get_ctrl_data(struct API_Data data);
static inline int api_get_mnger_data(struct API_Data data);
static inline int api_set_app_param(struct API_Data data);
/*
static inline int stop_server(struct Server *server);
static inline int run_queue(struct Queue *queue);
static inline int try_to_run_server(struct Server *server);
static inline void* if_outrank(struct Server *first, struct Server *second);
static inline int is_ancestor(struct Server *first, struct Server *second);
static inline int stop_task(struct Task *task);
static inline int try_to_run_task(struct Task *task);
static inline int release_hsf_task(struct Task *task);
static inline int run_task(struct Task *task);
*/
static inline int release_server(struct Server *server);
static inline int release_hsf_task(struct Task *task);
static inline int stop_linux_task(struct task_struct *task);
static inline int insert_task_to_queue(struct Task *task);

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
	print_warning("hsf_write", "write", YES);
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
		print_warning("hsf_ioctl", "failed to copy data", YES);
		return -EFAULT;
	}
	//print_int("api call", cmd);
	  if(Module_status == MOD_STOP && cmd != API_PRINT_LOG && cmd != API_STOP)
	  {	  
	      if(cmd != API_TASK_FINISH_JOB)
	      {
		  print_warning("hsf_ioctl","Module has been stopped!!!", YES);
		  //api_stop();
	      }
	      return -EFAULT;
	  }
	  if(scheduling_error == -1 && cmd != API_PRINT_LOG && cmd != API_STOP)
	  {	  
	      print_warning("hsf_ioctl","scheduling error has happend!!!", YES);
	      //api_stop();
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
	case API_TEST_TIMER:
		  res = api_test_timer(data);
		  break;
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
	case API_INTRA_APP_SCHED_EVENT:
		  res = api_intra_app_sched_event(data);
		  break;
	case API_GET_CTRL_DATA:
		  res = api_get_ctrl_data(data);
		  break;
	case API_GET_MNGER_DATA:
		  res = api_get_mnger_data(data);
		  break;
	case API_SET_APP_PARAM:
		  res = api_set_app_param(data);
		  break;
	  default: /* illegal api identifier. */
		  res = RES_ILLEGAL;
		  print_warning("hsf_ioctl", "illegal API", YES);
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
		print_int("__change_prio", "===**=== ERROR: failed to change priority, task pid:", p->pid, YES);
		print_int("__change_prio", "===**=== ERROR: failed to change priority, my pid:", current->pid, YES);
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
