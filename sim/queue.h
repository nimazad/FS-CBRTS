#include <list.h>
//#include "list.h"
#define NR_RT_TASKS 64
#define MAX_NR_OF_TASKS_IN_SERVER 10
#define NR_OF_SERVERS 10
#define PERIODIC_SERVER	1
#define SERVER_TYPE	10
#define TASK_TYPE	11
#define CBS	2

inline void print_warning(char *string)
{
	printf("Nima-HSF: %s.\n", string);
}
inline void print_int(char *string, int i)
{
	printf("Nima-HSF: %s = %d.\n", string, i);
}
typedef struct _Server Server;
typedef struct _Children
{
	struct list_head 	head;
	int id;
	int type;
	struct Server *server;
	struct Task *task;
}Children;
struct Server 
{	
	struct list_head 	head;
	Children	 	children;
	int type;
	int id;
	int priority;
	int attached_to_server;
	long budget;
	long period;
	long current_budget;
};
struct Server server_list;

struct Task {
	struct list_head 	head;
	int id;
	int pid;	/* original linux PID. */
	int priority; 	/* real-time priority. */
	int cpu_id;	/* CPU ID upon which the task is currently running. */
	int state;
	/* timing properties: */
	unsigned long wcet; 		/* by microseconds */
	unsigned long period; 		/* by jiffies */
	unsigned long deadline;		/* by jiffies */
	unsigned long release_time;	/* by jiffies */
	unsigned long exec_time;	/* by jiffies */
	struct task_struct *linux_yask;
};
struct Task task_list;
/* ================================Queue=====================================*/
struct Queue_element{
	struct list_head 	head;
	struct Server 		*server;
	struct Task		*task;
	int element_type;		// element can be server or task
};
#define PRIORITY_QUEUE	0
#define DEADLINE_QUEUE	1
typedef struct _Queue{
	struct Queue_element *elements;
	int queue_type;			// queue can be priority or deadline based
}Queue;
Queue Ready_Queue;
static inline int init_queue(Queue *queue, int type)
{
	queue->elements = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_KERNEL);
	queue->queue_type = type;
	INIT_LIST_HEAD(&queue->elements->head);
	return RES_SUCCESS;
}
static inline int print_queue(Queue *queue)
{
	struct list_head *pos, *queue_head;
	struct Queue_element *e;
	if (&queue->elements->head == NULL)
	{
	    print_warning("queue head is NULL");
	    return RES_FAULT;
	}	
	queue_head = &queue->elements->head;
	if(list_empty(queue_head))
	{
	    print_warning("queue is empty");
	}
	//print_int("type", queue->elements->element_type);
	/*
	list_for_each(pos, queue_head)
	{
		e = list_entry(pos, struct Queue_element, head);
		if (e->element_type == SERVER_TYPE)
		    ;//print_int("priority", e->server->priority);
		if (e->element_type == TASK_TYPE)
		    ;//print_int("priority", e->server->priority);
			
	}*/
  return RES_SUCCESS;
}
static inline int _insert_priority(Queue *queue, struct Queue_element *element)
{
	struct Queue_element *pos;
	struct list_head *queue_head;
	int priority = -1;
	if(element->element_type == SERVER_TYPE)
	    priority = element->server->priority;
	if(element->element_type == TASK_TYPE)
	    priority = element->task->priority;
	if(priority < 0)
	{
	    print_warning("illigal priority");
	    return RES_FAULT;
	}
	queue_head = &queue->elements->head;
	if(list_empty(queue_head))
	{
	    print_warning("queue is empty");
	    list_add_tail(&element->head, queue_head);
	}
	list_for_each_entry_continue(pos, queue_head, head)
	{
		//e = list_entry(pos, struct Queue_element, head);
		/*if (e->element_type == SERVER_TYPE)
			if( e->server->priority > priority)
			{
			    list_add_tail(&element->head, pos);
			    print_warning("inserted to the ready queue");
			    return RES_SUCCESS;
			}
		*/	
	}
	// priority is higher than any other elemenst in the queue
	list_add_tail(&element->head, queue_head);
  return RES_SUCCESS;
}
static inline int insert_queue(Queue *queue, struct Queue_element *element)
{