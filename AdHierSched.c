#include "library/hsf.h"
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("HSF Scheduler");
MODULE_AUTHOR("Nima M Khalilzad");
#define MODULE_NAME	"AdHierSched"
/* device number. */
static dev_t dev_id;
/* char device structure. */
static struct cdev c_dev;
static int __init hsf_init(void) 
{
	int ret, loop_i;
	/* get the device number of a char device. */
	ret = alloc_chrdev_region(&dev_id, 0, 1, MODULE_NAME);
	if (ret < 0) {
		print_warning("hsf_init", "failed to allocate device", YES);
		return ret;
	}
	/* initialize the char device. */
	cdev_init(&c_dev, &hsf_fops);
	/* register the char device. */
	ret = cdev_add(&c_dev, dev_id, 1);
	if (ret < 0) {
		print_warning("hsf_init", "failed to register device", YES);
		return ret;
	}
	/* set number of servers*/
	server_list.id = -1;
	INIT_LIST_HEAD(&server_list.head);
	task_list.id = -1;
	INIT_LIST_HEAD(&task_list.head);
	cbs_list.id = -1;
	INIT_LIST_HEAD(&cbs_list.head);
	app_list.id = -1;
	INIT_LIST_HEAD(&app_list.head);
	
	/* init ready queue*/
	init_queue(&Global_Ready_Queue, DEADLINE_QUEUE);
	Current_Ready_Queue = &Global_Ready_Queue;
	sema_init(&handler_sem, 1);
	scheduling_error = 0;
	Module_status = MOD_RUNNING;
	total_overhead = 0;
	control_overhead = 0;
	//mr_lock = SPIN_LOCK_UNLOCK;
	spin_lock_init(&mr_lock);
	for(loop_i=0;loop_i<number_of_proc;loop_i++)
	{
	      init_queue(&Proc_Local_Ready_Queue[loop_i], DEADLINE_QUEUE);
	}
	print_long("","", HZ, YES);
	print_warning("AdHierSched_init", "Hello!", YES);
	return 0;
}

static void __exit hsf_exit(void) 
{
	/* delete the char device. */
	cdev_del(&c_dev);
	/* return back the device number. */
	unregister_chrdev_region(dev_id, 1);

	print_warning("AdHierSched_exit", "Bye!", YES);
}
module_init(hsf_init);
module_exit(hsf_exit);
/*===============================Handlers===============================================*/

static void task_release_handler(unsigned long input_data)
{
    struct Handler_Data *data;
    unsigned long handler_timestamp = jiffies;
    spin_lock_irq(&mr_lock);
    data = (struct Handler_Data *) input_data;
    if(if_scheduling_error("task_release_handler") == YES)
    {
      print_warning("task_release_handler", "scheduling error has happened", YES);
      spin_unlock_irq(&mr_lock);
      return;
    }//if(data->task->cnt >50)
      //return;
    if(Module_status == MOD_STOP)
    {
      print_warning("task_release_handler", "Module has been stopped", YES);
      spin_unlock_irq(&mr_lock);
      return;
    }
    if(data->task == NULL)
    {
      print_warning("task_release_handler", "===**=== ERROR: task is null", YES);
      scheduling_error = -1;
      spin_unlock_irq(&mr_lock);
      return;
    }
    print_int("task_release_handler", "start---------------------------", data->task->id, YES);
    remove_timer(&data->task->period_timer);
    print_int_int("task_release_handler", "task id, cnt",data->task->id, data->task->cnt, NO);
    //print_time_now("task_release_handler","", NO);
    if(data->task->state == TASK_DETACHED)
    {
      print_warning("task_release_handler", "task is detached", YES);
      spin_unlock_irq(&mr_lock);
      return;
    }
    else
    {
      if(release_hsf_task(data->task) == RES_FAULT)
      {
	  print_warning("task_release_handler", "release_hsf_task failed", YES);
	  spin_unlock_irq(&mr_lock);
	  return;
      }
    }
    /*intra-app Scheduling event*/
    if(run_first_element(data->task->parent_app->ready_queue) == RES_FAULT)
    {
	print_warning("task_release_handler", "run_first_element failed", YES);
	spin_unlock_irq(&mr_lock);
	return;
    }
    print_int_int("task_release_handler", "finish---------------------------", data->task->id, jiffies - handler_timestamp, YES);
    spin_unlock_irq(&mr_lock);
}

static void server_release_handler(unsigned long input_data)
{
    struct Handler_Data *data;
    unsigned long handler_timestamp = jiffies;
    data = (struct Handler_Data *) input_data;
    spin_lock_irq(&mr_lock);
    if(if_scheduling_error("server_release_handler") == YES)
    {
      Module_status = MOD_STOP;
      print_warning("server_release_handler", "scheduling error has happened", YES);
      spin_unlock_irq(&mr_lock);
      return;
    }
    if( jiffies-start_time > experiment_duration || Module_status == MOD_STOP)
    {
      Module_status = MOD_STOP;
      print_warning("server_release_handler", "******************===========================End of experiment=======================***************************", YES);
      spin_unlock_irq(&mr_lock);
      return;
    }
    if(data->server == NULL)
    {
	print_warning("server_release_handler", "===**=== ERROR: server is null", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return ;
    } 
    print_int("server_release_handler", "start---------------------------", data->server->id, YES);
    remove_timer(&data->server->period_timer);
    remove_timer(&data->server->budget_timer);//TODO new
    print_int_int("server_release_handler", "server id, cnt",data->server->id, data->server->cnt, NO);
    if(release_server(data->server) == RES_FAULT)
    {
	print_warning("server_release_handler", "===**=== ERROR: release_server failed", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return ;
    }
    print_int_int("server_release_handler", "finish---------------------------", data->server->id, jiffies-handler_timestamp, YES);
    spin_unlock_irq(&mr_lock);
}
static void server_budget_handler(unsigned long input_data)
{
    struct Handler_Data *data;
    unsigned long handler_timestamp = jiffies;
    data = (struct Handler_Data *) input_data;
    spin_lock_irq(&mr_lock);
    if(if_scheduling_error("server_budget_handler") == YES)
    {
	  print_warning("server_budget_handler", "scheduling_error has happened", YES);
	  spin_unlock_irq(&mr_lock);
	  return;
    }
    if(data->server == NULL)
    {
	print_warning("server_release_handler", "===**=== ERROR: server is null", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return ;
    } 
    print_int("server_budget_handler", "start---------------------------", data->server->id, YES);
    remove_timer(&data->server->budget_timer);
    print_int_int("server_budget_handler", "server id, cnt",data->server->id, data->server->cnt, NO);
    //print_time_now("server_budget_handler","");
    if(data->server != active_server[data->server->proc_id])
    {
	  print_int("server_budget_handler", "budget handler is running while server is not active", data->server->id, YES);
	  //scheduling_error = -1;
	  spin_unlock_irq(&mr_lock);
	  return;
    }
    if(data->server->running_task != NULL )
    {
	print_time("server_budget_handler", "stopping running_task", jiffies-start_time, NO);
	if(stop_task(data->server->running_task) == RES_FAULT)
	{
	    print_warning("server_budget_handler", "===**=== ERROR: stop task failed", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return;
	}
	//intra-app scheduling event
	if(run_first_element(data->server->running_task->parent_app->ready_queue) ==RES_FAULT)
	{
	    print_warning("server_budget_handler", "===**=== ERROR: run_first_element failed", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return;
	}
	print_int("server_budget_handler", "setting running_task to null", -1, NO);
	data->server->running_task = NULL;
    }
    else
      print_warning("server_budget_handler", "running_task is null", NO);
    data->server->current_budget = 0;
    print_int("server_budget_handler", "putting server on SLEEP", active_server[data->server->proc_id]->id, NO);
    active_server[data->server->proc_id]->status = SERVER_SLEEP;
    active_server[data->server->proc_id] = NULL;
    // inter-app Scheduling event
    data->server->parent_app->total_budget += jiffies - data->server->timestamp_beta;
    print_int_int("server_budget_handler", "server triggering inter-app event: server id - total_budget", data->server->parent_app->id, data->server->parent_app->total_budget, NO);
    run_first_element(&Proc_Local_Ready_Queue[data->server->proc_id]);
    print_int_int("server_budget_handler", "finish---------------------------", data->server->id, jiffies-handler_timestamp, YES);
    spin_unlock_irq(&mr_lock);
}


/*===============================Scheduler funcrions===============================================*/
static inline void print_status(void)
{
  print_warning("print_status", "===============================", NO);
  
  print_warning("print_status", "===============================", NO);
}

/*
 * returns result (RES_FAULT or RES_SUCCESS)
 */ 
static inline int stop_task(struct Task *task)
{
	print_warning("stop_task", "starts", NO);	
	/*Insert it back into the queue*/
	if(task == NULL)
	{
	      print_warning("stop_task", "===**=== ERROR: task is null", YES);
	      scheduling_error = -1;
	      return RES_FAULT;
	}
	if(task->state == TASK_DETACHED)
	{
		print_warning("stop_task", "===**=== ERROR: task is detached", YES);
		scheduling_error = -1;
		return RES_FAULT;
	}
	if(task->linux_task == NULL)
	{
	      print_warning("stop_task", "===**=== ERROR: linux task is null", YES);
	      scheduling_error = -1;
	      return RES_FAULT;
	}
	if(insert_task_to_queue(task) == RES_FAULT) 
	{
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("stop_task", "===**=== ERROR: insert_queue failed", YES);
	  #endif
	  scheduling_error = -1;
	  return RES_FAULT;
	}
	print_int_int("stop_task", "task is inserted to the queue of app", task->id, task->parent_app->id, YES);
	print_warning("stop_task", "stopping linux task", NO);	
	if(stop_linux_task(task->linux_task) == RES_FAULT)
	    return RES_FAULT;
	//task->parent_server->consumed_budget += jiffies - task->timestamp;
	task->parent_app->useful_budget += jiffies - task->timestamp;
	print_int_int("stop_task", "task_exec", task->cnt, jiffies - task->timestamp, NO);
	print_int_int("stop_task", "useful_budget", task->parent_app->id, task->parent_app->useful_budget, YES);	
	if(task->missing_dl_flag == YES)
	{
	    task->parent_app->mu += jiffies - task->timestamp;
	    print_int_int("stop_task", "mu", task->parent_app->id, task->parent_app->mu, YES);
	}
	task->parent_server = NULL;
	task->state = TASK_READY;
	print_int("stop_task", "TASK_READY", task->id, YES);
	print_int_int("stop_task", "task stopes here****************************************", task->id, task->cnt, NO);	
	//print_time_now("stop_task", "stopping at", NO);
	print_warning("stop_task", "ends", NO);	
	return RES_SUCCESS;
}
static inline int server_context_switch(struct Server *server)
{
  struct Handler_Data *data_budget;
  struct Queue_element *element;
  struct Server *old_server;
  int allocated_budget;
  data_budget = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
  data_budget->id = server->id;
  data_budget->handler_type = SERVER_BUDGET_HANDLER;
  data_budget->server = server;
  if(active_server[server->proc_id] == server)
  {
      print_warning("server_context_switch", "only update timers", YES);
      remove_timer(&server->budget_timer);
      print_time("server_context_switch", "inserting server_budget_handler timer to", jiffies + server->current_budget - start_time, YES);
      return insert_timer(&server->budget_timer, server_budget_handler, (unsigned long) data_budget, jiffies + server->current_budget); 
  }
  if(server->status != SERVER_READY)
  {
	print_warning("server_context_switch", "===**=== ERROR: status is not active", YES);
	print_int_int("server_context_switch", "id - status ", server->id, server->status, YES);
	scheduling_error = -1;
	return RES_FAULT;
  }
  old_server = active_server[server->proc_id];
  if(old_server != NULL)
  {
      print_warning("server_context_switch", "stop active server and remove its budget deplition timer", YES);
      print_int_int("server_context_switch", "active server - new server", old_server->id, server->id, YES);
      remove_timer(&old_server->budget_timer);
      old_server->status = SERVER_READY;
      //old_server->current_budget = old_server->current_budget  - (division_unsigned(get_time_now() - old_server->timestamp_mic, 1000));
      allocated_budget = jiffies - old_server->timestamp;
      old_server->parent_app->total_budget += jiffies - old_server->timestamp_beta;
      print_int_int("server_context_switch", "total_budget", old_server->parent_app->id, old_server->parent_app->total_budget, YES);
      old_server->current_budget = old_server->current_budget  - allocated_budget;
      print_int_int("server_context_switch", "active_server->current_budget - allocated_budget", old_server->current_budget, allocated_budget, YES);
      if(old_server->current_budget < 0) 
      {
	old_server->current_budget = 0; //To fix the drift
	print_warning("server_context_switch", "===**=== ERROR: budget < 0", YES);
	print_int("server_context_switch", "id", old_server->id, YES);
	//scheduling_error = -1;
	//return RES_FAULT;
      }
      /*Stop its running task*/
      if(old_server->running_task != NULL)
      {
	    print_warning("server_context_switch", "stopping active_server running_task", YES);
	    if(stop_task(old_server->running_task) == RES_FAULT)
	      return RES_FAULT;
      }
      print_warning("server_context_switch", "setting active_server running task to null", NO);
      old_server->running_task = NULL;
      /*Insert it back into the queue if it has current_budget > 0*/
      if(old_server->current_budget > 0)
      {
	    element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
	    element->element_type	= SERVER_TYPE;
	    element->server		= old_server;//server; Fixed bug !!!
	    if(insert_queue(&Proc_Local_Ready_Queue[old_server->proc_id], element) == RES_FAULT) 
	    {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("server_context_switch", "===**=== ERROR: insert_queue failed", YES);
	      print_int("server_context_switch", "id", old_server->id, YES);
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	    }
	    else
	    {
	      print_int_int("server_context_switch", "server is inserted to queue id -current_budget", old_server->id, old_server->current_budget, YES);
	    //print_queue(&Proc_Local_Ready_Queue[old_server->proc_id]);
	    }
      }
  }
  if(server->running_task != NULL)
  {
      print_warning("server_context_switch", "stopping my running_task", YES);
      if(stop_task(server->running_task) == RES_FAULT)
	return RES_FAULT;
      print_warning("server_context_switch", "setting running task to null", NO);
      server->running_task = NULL;
  }
  print_warning("server_context_switch", "activate the input server", NO);
  active_server[server->proc_id] = server;
  server->status = SERVER_ACTIVE;
  print_warning("server_context_switch", "insert the budget deplition timer", NO);
  server->timestamp_mic = get_time_now();  
  server->timestamp = jiffies;
  server->timestamp_beta = jiffies;
  print_int("server_context_switch", "current_budget", server->current_budget, YES);
  print_int_int("server_context_switch", "***server is_running now on proc", server->id, server->proc_id, YES);
  print_time("server_context_switch", "inserting server_budget_handler timer to", jiffies + server->current_budget - start_time, YES);
  if(insert_timer(&server->budget_timer, server_budget_handler, (unsigned long) data_budget, jiffies + server->current_budget) == RES_FAULT)
    return RES_FAULT; 
  return RES_SUCCESS;
}

static inline int task_context_switch(struct Task *task, struct Server *server)
{
  cpumask_t mask;
  if(server->running_task == task)
  {
      print_warning("task_context_switch", "task is already running", NO);
      return RES_SUCCESS;
  }
  if(server->running_task != NULL)
  {
      print_int("task_context_switch", "stop running_task", task->id, NO);
      if(stop_task(server->running_task) == RES_FAULT)
	return RES_FAULT;      
      print_int("task_context_switch", "setting running_task to null", -1, NO);
      server->running_task = NULL;
  }
  print_warning("task_context_switch", "run the input task", NO);
  print_int("task_context_switch", "setting running_task", task->id, NO);
  server->running_task = task;
  task->parent_server = server;
  task->state = TASK_RUNNING_HSF;
  print_int("task_context_switch", "TASK_RUNNING_HSF", task->id, NO);
  task->timestamp = jiffies;
  if(task->abs_deadline < jiffies && task->missing_dl_flag == NO)
  {
      print_int_int("task_context_switch", "dl_miss", task->id, task->parent_app->dl_miss, YES);
      print_time("","deadline is at", task->abs_deadline - start_time, YES);
      print_time("","now is", jiffies - start_time, NO);
      task->missing_dl_flag = YES;
      task->parent_app->dl_miss++;
  }
  else
    print_warning("task_context_switch", "in_time", NO);
  if(task->linux_task == NULL)
  {
      print_warning("task_context_switch", "===***===linux task is null!!", YES);
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(task->cpu_id != server->proc_id)
  {
      // migration
      print_int("task_context_switch", "migrating to cpu", server->proc_id, NO);
      task->cpu_id = server->proc_id;
      cpus_clear(mask);
      cpu_set(task->cpu_id, mask);
      //set_cpus_allowed_ptr(task->linux_task, &mask);
      cpumask_copy(&task->linux_task->cpus_allowed, &mask);
      print_warning("task_context_switch", "migratinon finished", NO);
  }
  if(task->linux_task->state != TASK_RUNNING)
  {
// 	print_int("run_task", "waking up", task->id);
      print_int("task_context_switch", "wake_up_process", task->id, YES);
      wake_up_process(task->linux_task); 
      print_int_int("task_context_switch", "task starts here****************************************", task->id, task->cnt, NO);	
      //print_time_now("task_context_switch", "starting at", NO);
  }
  else
    print_warning("task_context_switch", "task_state is already running!!", NO);
  //wake_up_process(task->linux_task);
  return RES_SUCCESS;
}

static inline void* find_low_prio_task(struct Application *app, struct Task *newTask)
{
	Children *child_ser_pos;
	struct list_head *pos;
	struct Task *candidate_task;
	struct Server *candidate_server = NULL;
	candidate_task = newTask;
	list_for_each(pos, &app->servers.head)
	{
		child_ser_pos = list_entry(pos, Children, head);
		print_int_int("find_low_prio_task", "server_id and cnt", child_ser_pos->server->id, child_ser_pos->server->cnt, NO);
		if(child_ser_pos->server->status == SERVER_ACTIVE && child_ser_pos->server->running_task == NULL)
		{
		      print_int("find_low_prio_task", "server is idle", child_ser_pos->server->id, NO);
		      return child_ser_pos->server;
		}
		else if(child_ser_pos->server->status == SERVER_ACTIVE && compare_tasks(candidate_task, child_ser_pos->server->running_task, app->ready_queue->queue_type))
		{
		    print_int("find_low_prio_task", "candidate_task is", child_ser_pos->server->running_task->id, NO);
		    print_int("find_low_prio_task", "candidate_server is", child_ser_pos->server->id, NO);
		    candidate_task = child_ser_pos->server->running_task;
		    candidate_server = child_ser_pos->server;
		}
		
	}
	if(candidate_task != newTask)
	    return candidate_server;
	return NULL;
	
}
static inline int run_first_element(struct Queue *queue)
{
  struct Queue_element *queue_head;
  struct Server *server;
  struct Task *task;
  #ifdef DEBUG_CHECKS
  if(check_queue(queue) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("run_first_element", "===**=== ERROR: check_queue failed", YES);
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(check_element(queue->elements) == RES_FAULT)
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("run_first_element", "===**=== ERROR: queue elements is a NULL pointer", YES);
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  #endif
  if(list_empty(&queue->elements->head))
  {
       print_warning("run_first_element", "queue is empty", NO);
//       print_queue(queue);
      return RES_SUCCESS;
  }
  /*Queue head is not empty*/
  queue_head = list_entry(queue->elements->head.next, struct Queue_element, head);
  if(queue_head->element_type == TASK_TYPE)
  {
       print_warning("run_first_element", "queue head is a task", NO);
       task = queue_head->task;
	if(task->state == TASK_RUNNING_HSF || task->state == TASK_DETACHED)
	{
	      print_int_int("run_first_element", "===**=== ERROR: task state is running or detached, id - state", task->id, task->state, YES);
	      scheduling_error = -1;
	      return RES_FAULT;
	}
       server = find_low_prio_task(task->parent_app, task);
       if(server != NULL)
       {
	  if(delete_queue(queue_head) == RES_FAULT)
	      return RES_FAULT;
	  print_int_int("run_first_element", "task is deleted from the queue of app", task->id, task->parent_app->id, YES);
	  if(task_context_switch(task, server) ==RES_FAULT)
	    return RES_FAULT;
	  print_int_int("run_first_element", "***task is_running in server", task->id, server->id, YES);
	  print_int_int("run_first_element", "***task is_running on proc", task->id, task->cpu_id, NO);
       }
       else
	 print_int("run_first_element", "could not preempt any tasks", task->id, NO);
       return RES_SUCCESS;
  }
  if(queue_head->element_type == SERVER_TYPE)
  {
	print_warning("run_first_element", "queue head is a server", NO);
	print_int("run_first_element", "server_id", queue_head->server->id, NO);
	//check to see if current running can be preempted
	server = queue_head->server;
	if(compare_servers(server, active_server[server->proc_id], DEADLINE_QUEUE))
	{
	    if(delete_queue(queue_head) == RES_FAULT)
	      return RES_FAULT;
	    print_int_int("run_first_element", "server is removed from the queue proc", server->id, server->proc_id, NO);
	    //print_queue(queue);
	    if(server_context_switch(server) == RES_FAULT)
	      return RES_FAULT;
	    //Lock here
	    // intra-app scheduling event
	    if(run_first_element(server->parent_app->ready_queue) == RES_FAULT)
	      return RES_FAULT;  
	}
	return RES_SUCCESS;
  }
  return RES_SUCCESS;
}
static inline int release_server(struct Server *server)
{
    struct Queue_element *element;
    struct Handler_Data *data_period;
    print_warning("release_server", "starts", NO);
    //print_time_now("release_server", "now", NO);
    #ifdef DEBUG_CHECKS
    if(check_server(server) == RES_FAULT)
    {
	    #ifdef DEBUG_SCHED_ERRORS
	    print_warning("release_server", "===**=== ERROR: server check failed", YES);
	    #endif
	    scheduling_error = -1;
	    return RES_FAULT;
    }
    #endif
    server->cnt++;
    server->current_budget = server->budget;
    server->abs_deadline = server->relative_deadline + jiffies;
    print_int_int("release_server", "RELEASING SERVER status and replanishing current_budget", server->id, server->status, NO);
    server->status = SERVER_READY;
    //TODO remove recently moved here check later
    remove_timer(&server->budget_timer);
    element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
    element->element_type	= SERVER_TYPE;
    element->server		= server;
    
    if(active_server[server->proc_id] != server)
    {
      if(search_queue_server(&Proc_Local_Ready_Queue[server->proc_id], server) != NULL)
	  print_int_int("release_server", "server is already in the queue proc", server->id, server->proc_id, YES);
      else if(insert_queue(&Proc_Local_Ready_Queue[server->proc_id], element) == RES_FAULT) 
      {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("release_server", "===**=== ERROR: insert_queue failed", YES);
	print_int("release_server", "id", server->id, YES);
	#endif
	scheduling_error = -1;
	return RES_FAULT;
      }
      else{
	print_int_int("release_server", "server is inserted to queue, id -current_budget", server->id, server->current_budget, YES);
	//print_queue(&Proc_Local_Ready_Queue[server->proc_id]);
      }
      // inter-app Scheduling event
      print_int("release_server", "server triggering inter-app event: server id", server->id, NO);
      if(run_first_element(&Proc_Local_Ready_Queue[server->proc_id]) ==RES_FAULT)
	return RES_FAULT;
      
    }
    else
      print_int_int("release_server", "server is the active server", server->id, server->proc_id, NO);
    
    print_warning("release_server", "update timers", NO);
    /*update timesrs*/
    data_period = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
    data_period->id = server->id;
    data_period->handler_type = SERVER_RELEASE_HANDLER;
    data_period->server = server;
    print_time("release_server", "inserting timer to", jiffies + server->period - start_time, YES);
    if(insert_timer(&server->period_timer, server_release_handler, (unsigned long) data_period, jiffies + server->period) == RES_FAULT)
      return RES_FAULT;
    print_warning("release_server", "ends", NO);
    return RES_SUCCESS;
}

static inline int insert_task_period_timer(struct Task *task)
{
  struct Handler_Data *data_period;
  data_period = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
  data_period->id = task->id;
  data_period->handler_type = TASK_RELEASE_HANDLER;
  data_period->task = task;
  remove_timer(&task->period_timer);
  print_time("insert_task_period_timer", "inserting period timer to", task->next_release_time - start_time, YES);
  return insert_timer(&task->period_timer, task_release_handler, (unsigned long) data_period, task->next_release_time);
}
static inline int insert_task_to_queue(struct Task *task)
{
  struct Queue_element *element;
  element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
  element->element_type	= TASK_TYPE;
  element->task 		= task;
  if(search_queue_task(task->parent_app->ready_queue, task) != NULL)
    print_int_int("insert_task_to_queue", "task is already in the queue of app", task->id, task->parent_app->id, NO);
  else if(insert_queue(task->parent_app->ready_queue, element) == RES_FAULT) 
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("insert_task_to_queue", "===**=== ERROR: insert_queue failed", YES);
    print_int("insert_task_to_queue", "id", task->id, YES);
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  print_int_int("insert_task_to_queue", "task is inserted to app ready queue", task->id, task->parent_app->id, NO);
  return RES_SUCCESS;
}

static inline int release_hsf_task(struct Task *task)
{
	
	
	print_warning("release_hsf_task", "starts", NO);
	#ifdef DEBUG_CHECKS
	if(check_task(task) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_hsf_task", "===**=== ERROR: task check failed", YES);
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	}
	#endif
	task->cnt++;
	task->release_time = jiffies;
	task->next_release_time = jiffies + task->period;
	//if previous job is not finished yet
	if(task->state != TASK_SLEEP)
	{
	      task->queued_releases++;
	      print_int("release_hsf_task", "task is not sleeping, increasing queued_releases", task->id, NO);
	      return RES_SUCCESS;
	}
	task->state = TASK_READY;
	print_int("release_hsf_task", "inserintg task to queue TASK_READY", task->id, YES);
	task->abs_deadline = jiffies + task->relative_deadline;
	if(insert_task_to_queue(task) == RES_FAULT)
	    return RES_FAULT;
	if(insert_task_period_timer(task) == RES_FAULT)
	  return RES_FAULT;
	print_warning("release_hsf_task", "ends", NO);
	return RES_SUCCESS;
}


static inline int stop_linux_task(struct task_struct *task)
{
    if(task == NULL)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("stop_linux_task", "===**=== ERROR: task is a null pointer", YES); 
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    task->state = TASK_UNINTERRUPTIBLE;
    set_tsk_need_resched(task);
    //schedule();
    #ifdef DEBUG_SCHED
    print_warning("stop_linux_task", "task is stoped", NO); 
    #endif
    return RES_SUCCESS;
}

/*===============================API funcrions===============================================*/
static inline int api_create_server(struct API_Data data)
{
	struct Server *s;
	spin_lock_irq(&mr_lock);
	print_warning("api_create_server", "starts!", YES);
	s = (struct Server *)kmalloc(sizeof(struct Server), GFP_ATOMIC);
	s->cnt =0;
	s->budget = 0;
	s->period = 0;
	s->current_budget = 0; //complinent with CBS
	s->priority = 99;
	s->dl_miss = 0;
	s->type = data.server_type;
	s->status = SERVER_SLEEP;
	s->running_task = NULL;
	s->parent_app = NULL;
	s->abs_deadline = 0;
	s->relative_deadline = 0;
	
	
	s->id = get_last_server_id(&server_list.head) + 1;
	list_add_tail(&s->head, &server_list.head);
	
	#ifdef JIFFY_TIMER
	init_timer(&s->period_timer);
	init_timer(&s->budget_timer);
	#endif
	print_int("api_create_server", "server id", s->id, YES);
	print_int("api_create_server", "cnt server", count_list(&server_list.head), YES);
	print_warning("api_create_server", "api call ends!", YES);
	spin_unlock_irq(&mr_lock);
	return s->id;
}
static inline int api_set_server_param(struct API_Data data)
{	
	struct Server *s;
	spin_lock_irq(&mr_lock);
	s = find_server(&server_list.head, data.id);
	
	if(s == NULL)
	{
		print_warning("api_set_server_param", "could not find server", YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	}
	if(data.period > 0)
		s->period = data.period;
	if(data.budget >= 0)
		s->budget = data.budget;
	else
	{
		print_warning("api_set_server_param", "budget < 0", YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	}
	if(data.priority >= 0)
		s->priority= data.priority;
	else
		s->priority= -1; 
	if(data.deadline > 0)
		s->relative_deadline = data.deadline;
	else
		s->relative_deadline = s->period; //by defult deadline = period
	if(data.proc_id >=0 && data.proc_id < number_of_proc)	
		s->proc_id = data.proc_id;
	else
	{
	    print_warning("api_set_server_param", "proc_id is not in range", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
	}
	  
	print_int("api_set_server_param", "id", s->id, YES);
	print_int("api_set_server_param", "period", s->period, YES);
	print_int("api_set_server_param", "budget", s->budget, YES);
	print_int("api_set_server_param", "priority", s->priority, YES);
	print_int("api_set_server_param", "proc_id", s->proc_id, YES);
	print_warning("api_set_server_param", "api call ends", YES);
	spin_unlock_irq(&mr_lock);
	return RES_SUCCESS;
}
static inline int api_attach_server_to_server(struct API_Data data)
{	/*attaching id2 to id1*/
	/*int parent, child;
	struct Server *s_parent, *s_child;
	Children *c, *tmp;
	parent = data.id;
	child  = data.id2;
	if(parent<0 || child<0 || parent == child)
	{
		print_warning("api_attach_server_to_server", "server ids should be >= 0 and different");
		return RES_FAULT;
	}
	s_parent = find_server(&server_list.head, parent);
	if(s_parent == NULL)
	{
		print_warning("api_attach_server_to_server", "could not find parent server");
		return RES_FAULT;
	}
	s_child = find_server(&server_list.head, child);
	if(s_child == NULL)
	{
		print_warning("api_attach_server_to_server", "could not find child server");
		return RES_FAULT;
	}
	tmp = find_children(&s_parent->children.head, child, SERVER_TYPE);
	if(tmp != NULL)
	{	
		print_int("api_attach_server_to_server", "server is already attached!", tmp->id);
		return RES_FAULT;	
	}
	// Adding server to children list
	c = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	c->id = child;
	c->type = SERVER_TYPE;
	c->server = s_child;
	list_add_tail(&c->head, &s_parent->children.head);
	s_child->parent = s_parent;
	print_int("api_attach_server_to_server", "list cnt =", count_list(&s_parent->children.head));	
	print_warning("api_attach_server_to_server", "api call!");*/
	return RES_SUCCESS;
}
static inline int api_create_task(void)
{
	struct Task *t;
	t = (struct Task *)kmalloc(sizeof(struct Task), GFP_ATOMIC);
	t->id = get_last_task_id(&task_list.head) + 1;
	t->wcet			= 0;
	t->period		= 0;
	t->relative_deadline	= 0;
	t->release_time		= 0;
	t->next_release_time	= 0;
	t->exec_time		= 0;
	t->dl_miss		= 0;
	t->cnt 			= 0;
	t->missing_dl_flag	= NO;
	t->parent_server	= NULL;
	t->parent_app		= NULL;
	t->linux_task		= NULL;
	t->cbs			= NULL;
	t->queued_releases 	= 0;
	list_add_tail(&t->head, &task_list.head);
	#ifdef JIFFY_TIMER
	init_timer(&t->period_timer);
	#endif
	print_warning("api_create_task", "api call!", YES);
	return t->id;
}
static inline int api_attach_task_to_server(struct API_Data data)
{	
	int parent, child;
	struct Server *s;
	struct Task *t;
	Children *c, *tmp;
	parent = data.id;
	child  = data.id2;
	if(parent<0 || child<0)
	{
		print_warning("api_attach_task_to_server", "server ids should be > 0", YES);
		return RES_FAULT;
	}
	if(data.server_type == PERIODIC_SERVER)
	    s = find_server(&server_list.head, parent);
	else
	    s = find_server(&cbs_list.head, parent);
	if(s == NULL)
	{
		print_warning("api_attach_task_to_server", "could not find parent server", YES);
		return RES_FAULT;
	}
	t = find_task(&task_list.head, child);
	if(t == NULL)
	{
		print_warning("api_attach_task_to_server", "could not find child task", YES);
		return RES_FAULT;
	}
	tmp = find_children(&s->children.head, child, TASK_TYPE);
	if(tmp != NULL)
	{	
		print_int("api_attach_task_to_server", "task is already attached!", tmp->id, YES);
		return RES_FAULT;	
	}
	/* Adding task to children list*/
	c = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	c->id = child;
	c->type = TASK_TYPE;
	c->task = t;
	list_add_tail(&c->head, &s->children.head);
	if(data.server_type == PERIODIC_SERVER)
	    t->parent_server = s;
	else
	    t->cbs = s;
	print_int("api_attach_task_to_server", "list cnt =", count_list(&s->children.head), YES);	
	print_warning("api_attach_task_to_server", "api call!", YES);
	return RES_SUCCESS;
}
static inline int api_release_server(struct API_Data data)
{
	struct Server *s;
	int res;
	print_warning("api_release_server", "api start", YES);
	//start_time = jiffies;
	s = find_server(&server_list.head, data.id);
	if(s != NULL)
	    s->status = SERVER_ACTIVE;
	else
	{
	    print_warning("api_release_server", "could not find the server", YES);
	    scheduling_error = -1;
	    return RES_FAULT;
	}
	if(s->parent_app == NULL)
	{
	    print_warning("api_release_server", "parent_app is null", YES);
	    scheduling_error = -1;
	    return RES_FAULT;
	}
	
	res = release_server(s);
	print_warning("api_release_server", "api call ends", YES);
	return res;
}
static inline int api_release_task(struct API_Data data)
{
	struct Task *task;
	int res;
	task = find_task(&task_list.head, data.id);
	if(task->parent_app == NULL)
	{
	    print_warning("api_release_task", "paren app is null! it can not be released", YES);
	    scheduling_error = -1;
	    return RES_FAULT;
	}
	task->state = TASK_SLEEP;
	print_int("api_release_task", "TASK_SLEEP", task->id, YES);
	res = release_hsf_task(task);
	
	print_warning("api_release_task", "ends", YES);
	return res;
}
static inline int api_kill_server(struct API_Data data)
{
  int res = RES_SUCCESS;
  /*struct Server *server;
  struct Queue_element *element;
  
  server = find_server(&server_list.head, data.id);
  if(server == NULL)
  {
      print_warning("api_kill_server", "could not find server");
      return RES_FAULT;
  }
  if(server->parent == NULL)
      element = search_queue_server(&Global_Ready_Queue, server);
  else
      element = search_queue_server(server->parent->ready_queue, server);
  if(element == NULL)
  {
      print_warning("api_kill_server", "could not find server");
      return RES_FAULT;
  }
  res = delete_queue(element);
  if(server->parent == NULL)
      print_queue(&Global_Ready_Queue);
  else
      print_queue(server->parent->ready_queue);
  print_warning("api_kill_server", "API call to kill server");
  */
  return res;
}

static inline int api_stop(void)
{
    struct Task *t_pos;
    int res;
    struct Application *app;
    struct list_head *pos;
    res = delete_whole_queue(&Global_Ready_Queue);
    Module_status = MOD_STOP;
    
    list_for_each_entry(t_pos, &task_list.head, head)
    {
	if(t_pos != &task_list)
	  if(t_pos->linux_task != NULL)
	  {
	    wake_up_process(t_pos->linux_task);
	    t_pos->linux_task = NULL;
	  }
    } 
    //print_time_now("api_stop", "now", YES);
    print_long("api_stop", "control overhead", control_overhead, YES);
    print_long("api_stop", "total overhead", total_overhead, YES);
    
    list_for_each(pos, &app_list.head)
    {
	  app = list_entry(pos, struct Application, head);
	  //print_int_int("api_stop", "app id", app->id, app->total_budget, NO);
    }
	
    print_warning("api_stop", "api call!", YES);
    return res;
}
static inline int api_run(void)
{
    int res = RES_SUCCESS;
    struct Server *pos;
    struct Task *t_pos;
//     struct timeval t;
    spin_lock_irq(&mr_lock);
    print_warning("api_run", "start---------------------------", YES);
    start_time = jiffies;
    do_gettimeofday(&start_time_tv);
//     start_time_milli = t.tv_sec * 1000 + t.tv_usec / 1000;
    list_for_each_entry(t_pos, &task_list.head, head)
    {
	if(t_pos != &task_list)
	{
	      t_pos->state = TASK_SLEEP;
	      res = release_hsf_task(t_pos);
	      if(res == RES_FAULT)
	      {
		  print_warning("api_run", "===**=== ERROR", YES);
		  scheduling_error = -1;
		  spin_unlock_irq(&mr_lock);
		  return RES_FAULT;
	      }
	      //try_to_run_task(t_pos);
	}
    } 
    
    list_for_each_entry(pos, &server_list.head, head)
    {
	if(pos != &server_list && pos->status == SERVER_SLEEP)
	{
	    pos->status = SERVER_ACTIVE;
	    res = release_server(pos);
	    if(res == RES_FAULT)
	    {
		  print_warning("api_run", "===**=== ERROR 2", YES);
		  scheduling_error = -1;
		  spin_unlock_irq(&mr_lock);
		  return RES_FAULT;
	    }
	}
    }  
    Module_status = MOD_RUNNING;
    #ifdef DEBUG_SCHED_API
    print_warning("api_run", "end---------------------------", YES);
    #endif
    spin_unlock_irq(&mr_lock);
    return res;
}
static inline int api_set_task_param(struct API_Data data)
{	
	struct Task *t;
	
	t = find_task(&task_list.head, data.id);
	if(t == NULL)
	{
		print_warning("api_set_task_param", "could not find server", YES);
		scheduling_error = -1;
		return RES_FAULT;
	}
	if(data.period > 0)
		t->period = data.period;
	if(data.budget >= 0)
		t->exec_time= data.budget;
	if(data.priority >= 0)
		t->priority= data.priority;
	else
		t->priority= -1; 
	if(data.deadline > 0)
		t->relative_deadline = data.deadline;
	else
		t->relative_deadline = t->period;
	print_int("api_set_task_param", "id", t->id, YES);
	print_int("api_set_task_param", "period", t->period, YES);
	print_int("api_set_task_param", "exec_time", t->exec_time, YES);
	print_int("api_set_task_param", "priority", t->priority, YES);
	print_warning("api_set_task_param", "api call to set task", YES);
	return RES_SUCCESS;
}
static inline int api_attach_task_to_mod(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Task *t;
    struct task_struct *change_prio_task;
    cpumask_t mask;
    struct sched_param sp;
    sp.sched_priority = 99;
    
    t = find_task(&task_list.head, data.id);
    if(check_task(t) == RES_FAULT)
    {
	print_warning("api_attach_task_to_mod", "check task faild!", YES);
	scheduling_error = -1;
	return RES_FAULT;
    }
    t->linux_task = current;
    t->abs_deadline = 0;
    
    /*running_task = t;
    if(t->parent == NULL)
	Current_Ready_Queue = &Global_Ready_Queue;
    else
	Current_Ready_Queue = t->parent->ready_queue;*/
    /* save the CPU mask. */
    //cpumask_copy(0, t->cpus_allowed);
    /* temporarily disable migration. */
    cpus_clear(mask);
    //cpu_set(smp_processor_id(), mask);
    //cpumask_t mask;
    cpu_set(CPU_NR, mask);
    cpumask_copy(&t->linux_task->cpus_allowed, &mask);
    t->cpu_id = CPU_NR;
    
    change_prio_task = kthread_create((void*)change_prio, (void*)t, "resch-kthread");
    kthread_bind(change_prio_task, CPU_NR);
    print_int("api_attach_task_to_mod", "sched_setscheduler result", sched_setscheduler(change_prio_task, SCHED_FIFO, &sp), YES);
    wake_up_process(change_prio_task);
    //local_irq_enable();
    //change_prio(t, 0);
    
    print_int("api_attach_task_to_mod", "my pid is", t->linux_task->pid, YES);
    t->linux_task->state = TASK_UNINTERRUPTIBLE;
    schedule();
    
    if(stop_linux_task(current) == RES_FAULT)
    {
      print_warning("api_attach_task_to_mod", "===**=== ERROR in stop_linux_task", YES);
      scheduling_error = -1;
      return RES_FAULT;
    }
    t->state = TASK_ATTACHED;
    print_warning("api_attach_task_to_mod", "api call!", YES);
    return res;
}
static inline int api_detach_task(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Task *t;
    spin_lock_irq(&mr_lock);
    print_warning("api_detach_task", "starts!", YES);
    t = find_task(&task_list.head, data.id);
    if(check_task(t) == RES_FAULT)
    {
	print_warning("api_detach_task", "check task faild!", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return RES_FAULT;
    }
    print_int("api_detach_task", "setting running task of server to null!", t->parent_server->id, NO);
    t->parent_server->running_task = NULL;
    if(t->linux_task != NULL)
    {
	if(stop_linux_task(t->linux_task) == RES_FAULT)
	{
	    print_warning("api_detach_task", "===**=== ERROR in stop_linux_task", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
	}
    }
    else
      print_int("api_detach_task", "linux task is null", t->id, YES);
    t->parent_server = NULL;
	
    t->state = TASK_DETACHED;
    //t->linux_task = NULL;
    remove_timer(&t->period_timer);
    //intra-app scheduling event
    if(run_first_element(t->parent_app->ready_queue) == RES_FAULT)
    {
	print_warning("api_detach_task", "===**=== ERROR in stop_linux_task", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return RES_FAULT;
    }
    
    print_int("api_detach_task", "api call from task", t->id, YES);
    //print_time_now("api_detach_task", "detaching task", YES);
    spin_unlock_irq(&mr_lock);
    schedule();
    print_warning("api_detach_task", "===**=== ERROR task ended", YES);
    scheduling_error = -1; //TODO for this experiment
    print_warning("api_detach_task", "ends!", YES);
    return res;
}
static inline int api_detach_server(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Server *server;
    spin_lock_irq(&mr_lock);
    server = find_server(&server_list.head, data.id);
    if(server == NULL)
    {
	    print_warning("api_detach_server", "could not find server", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
    }
    if(server->parent_app == NULL)
    {
	    print_warning("api_detach_server", "parent app is null", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
    }
    server->parent_app->proc_list[server->proc_id] = 0;
    server->status = SERVER_DETACHED;
    print_warning("api_detach_server", "server is running, removing timers", YES);
    remove_timer(&server->budget_timer);
    remove_timer(&server->period_timer);
    if(active_server[server->proc_id] == server)
    {
	if(server->running_task != NULL )
	{
	    print_time("api_detach_server", "stopping running_task", jiffies-start_time, NO);
	    if(stop_task(server->running_task) == RES_FAULT)
	    {
		print_int("api_detach_server", "run_first_element running_task failed", server->id, YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	    }
	    //intra-app scheduling event
	    if(run_first_element(server->running_task->parent_app->ready_queue) == RES_FAULT)
	    {
		print_int("api_detach_server", "run_first_element failed for tasks", server->id, YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	    }
	    print_int("api_detach_server", "setting running_task to null", -1, YES);
	    server->running_task = NULL;
	}
	else
	  print_warning("api_detach_server", "running_task is null", YES);
	server->current_budget = 0;
	server->parent_app->total_budget += jiffies - server->timestamp_beta;
	print_int_int("api_detach_server", "total_budget", server->parent_app->id, server->parent_app->total_budget, YES);
	active_server[server->proc_id]->status = SERVER_DETACHED;
	active_server[server->proc_id] = NULL;
	// inter-app Scheduling event
	if(run_first_element(&Proc_Local_Ready_Queue[server->proc_id]) == RES_FAULT)
	{
	    print_int("api_detach_server", "run_first_element failed for servers", server->id, YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
	}
    }
    delete_server_from_queue(&Proc_Local_Ready_Queue[server->proc_id], server);
    print_warning("api_detach_server", "api call ends!", YES);
    spin_unlock_irq(&mr_lock);
    return res;
}
static inline int api_task_finish_job(struct API_Data data)
{
    struct Task *task;
    spin_lock_irq(&mr_lock);
    if(if_scheduling_error("task_release_handler") == YES)
    {
	    print_warning("api_task_finish_job", "scheduling error has happened!", YES);
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
    }
    task = find_task(&task_list.head, data.id);
    if(task == NULL)
    {
	    print_warning("api_task_finish_job", "===**=== ERROR: could not find task", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
    }
    print_int("api_task_finish_job", "starts", task->id, YES);
    if(task->state != TASK_RUNNING_HSF && task->state != TASK_READY)
    {
	    print_int("api_task_finish_job", "===**=== ERROR: task is not running state", task->state, YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
    }
    task->parent_app->useful_budget += jiffies - task->timestamp;
    if(task->missing_dl_flag == YES)
    {
	task->parent_app->mu += jiffies - task->timestamp;
	print_int_int("api_task_finish_job", "mu", task->parent_app->id, task->parent_app->mu, YES);
    }
    else
    {
	if(jiffies > task->abs_deadline)
	{
	      task->parent_app->mu += jiffies - task->abs_deadline; //Only the part that is executed after deadline
	      task->parent_app->dl_miss++;
	      print_int_int("api_task_finish_job", "dl_miss++, mu", task->parent_app->id, task->parent_app->mu, YES);
	      print_time("api_task_finish_job", "abs_deadline", task->abs_deadline-start_time, YES);
	}
    }
    print_int_int("api_task_finish_job", "useful_budget of app", task->parent_app->id, task->parent_app->useful_budget, NO);
    if(stop_linux_task(task->linux_task) == RES_FAULT)
    {
	print_int("api_task_finish_job", "stop linux task failed", task->id, YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return RES_FAULT;
    }
    if(task->parent_server != NULL)
	  task->parent_server->running_task = NULL;
    task->state = TASK_SLEEP;
    print_int("api_task_finish_job", "TASK_SLEEP", task->id, YES);
    task->missing_dl_flag = NO;
    print_int("api_task_finish_job", "setting running_task to null", -1, NO);
    print_int_int("api_task_finish_job", "task stopes here****************************************", task->id, task->cnt, NO);	
    //print_time_now("api_task_finish_job", "stopping at", NO);
    
    if(task->queued_releases > 0)
    {
	  print_int("api_task_finish_job", "queued_releases for", task->id, NO);
	  print_int("api_task_finish_job", "number of queued_releases", task->queued_releases, YES);
	  task->queued_releases--;
	  task->state = TASK_READY;
	  print_int("api_task_finish_job", "inserting task to queue TASK_READY", task->id, YES);
	  task->abs_deadline = task->release_time + task->relative_deadline;
	  if(insert_task_to_queue(task) == RES_FAULT)
	  {
	      print_int("api_task_finish_job", "inserting task to queu failed", task->id, YES);
	      scheduling_error = -1;
	      spin_unlock_irq(&mr_lock);
	      return RES_FAULT;
	  }
	  if(task->next_release_time > jiffies)
	  {
	      if(insert_task_period_timer(task) == RES_FAULT)
	      {
		  spin_unlock_irq(&mr_lock);
		  return RES_FAULT;
	      }
	      print_warning("api_task_finish_job", "inserting to the queue", NO);
	  }
	  else
	  {
	      //releasing next job here
	      print_warning("api_task_finish_job", "releasing next job here", NO);
	      task->cnt++;
	      task->queued_releases++;
	      task->release_time 	= task->next_release_time;
	      task->next_release_time += task->period;
	  }
    }
    //intra-app scheduling event 
    if(run_first_element(task->parent_app->ready_queue) == RES_FAULT)
    {
      print_warning("api_task_finish_job", "===**=== ERROR in run_first_element", YES);
	scheduling_error = -1;
	spin_unlock_irq(&mr_lock);
	return RES_FAULT;
    }
    print_int("api_task_finish_job", "ends", task->id, YES);
    spin_unlock_irq(&mr_lock);
    return RES_SUCCESS;
}

//#ifdef TEST_TIMER
static inline int api_test_timer(struct API_Data data)
{
  
  return 0;
  /*struct Handler_Data *data_handler1;
  int res = RES_SUCCESS;
  unsigned long n;
  print_time("", "now", get_time_now(), NO);
  n=get_time_now();
  n-= 45;
  print_time("", "overhead", division(get_time_now()-n, 1000), YES);
  */
//   int i;
//   ktime_t ktime;
 /* struct Task *t_pos;
  struct Server *s;
  unsigned long budget[NO_INPUT], net_out;
  for(i=0;i<NO_INPUT;i++)
    budget[i] = 1+i*i;
  
  s = find_server(&server_list.head, 1);
  net_out = ask_network(s, budget);
  teach_network(s, budget, net_out, 50);
  ask_network(s, budget);
  return RES_SUCCESS;
 */ /*do_gettimeofday(&t);
  time_to_tm(t.tv_sec, 0, &broken);
  printk("%d:%d:%d:%ld\n", broken.tm_hour, broken.tm_min, 
                         broken.tm_sec, t.tv_usec);
                         
  data_handler1 = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
  data_handler1->id = 12;
  
  list_for_each_entry(t_pos, &task_list.head, head)
    {
	if(t_pos != &task_list)
	{
	      data_handler1->task = t_pos;
	}
    } 

  
  hrtimer_init(&data_handler1->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  
  t = current_kernel_time();
  add.tv_nsec = 25;
  add.tv_sec = 0;
  data_handler1->my_timespec = timespec_add(t, add);
  ktime = ktime_set(0, (25));
  insert_hrtimer(&data_handler1->timer, hrhandler, &ktime);
*/
  //insert_hrtimer(&hr_timer1, hrhandler, &ktime);
  
//   my_tmp_cnt = 0;
//   data_handler1 = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
//   data_handler1->id = 1;
//   
//     list_for_each_entry(t_pos, &task_list.head, head)
//     {
// 	if(t_pos != &task_list)
// 	{
// 	      data_handler1->task = t_pos;
// 	}
//     } 
//   #ifdef JIFFY_TIMER
//   init_timer(&timer1);
//   //start_time = jiffies;
//   do_gettimeofday(&start_time_tv);
//   //print_time_now("api", "now", YES);
//   insert_timer(&timer1, handler, (unsigned long) data_handler1, jiffies + 1);
//   #endif
  //insert_timer(&timer2, handler, (unsigned long) data_handler2, jiffies + 20);
 
  /*
  destroy_timer_on_stack(&timer1);
  setup_timer_on_stack(&timer1, handler, (unsigned long)data_handler1);
  mod_timer(&timer1, jiffies + 10);
  
  destroy_timer_on_stack(&timer2);
  setup_timer_on_stack(&timer2, handler, (unsigned long)data_handler2);
  mod_timer(&timer2, jiffies + 10);
  *//*
  print_time("api_test_timer", "seting timer", jiffies, YES);
  print_warning("api_test_timer", "API call to test timer", YES);*/
  return 0;
}
//#endif
static inline int api_print_log(struct API_Data data)
{
    int i=0;
    struct Application *app;
    app = find_app(&app_list.head, data.id);
    if(app == NULL)
    {
	print_warning("api_print_log", "app is null", YES);
	return RES_FAULT;
    }
    if(app->log == NULL)
    {
	print_warning("api_print_log", "app log is NULL", YES);
	return RES_FAULT;
    }
    printk(KERN_WARNING "========== Log of App[%d] ==========\n\n", app->id);
    for(i=0;i<app->log->index;i++)
    {
	  printk(KERN_WARNING "Nima-HSF LOG %d: at[ %d ] mu: %d , beta: %d , dl_miss: %d , alpha: %d , T: %d\n", i, app->log->time[i], app->log->mu[i], app->log->beta[i], app->log->dl_miss[i], app->log->alpha[i], app->log->period[i]);
    }
    printk(KERN_WARNING "====================================\n\n");
    return RES_SUCCESS;
}
static inline int api_create_app(struct API_Data data)
{
	struct Application *app;
	int i;
	app = (struct Application *)kmalloc(sizeof(struct Application), GFP_ATOMIC);
	app->budget = 0;
	app->period = 0;
	app->bandwidth = 0;
	app->dl_miss = 0;
	app->importance = 0;
	app->total_budget = 0;
	app->useful_budget = 0;
	app->mu = 0;
	INIT_LIST_HEAD(&app->servers.head);
	INIT_LIST_HEAD(&app->tasks.head);
	app->ready_queue = (struct Queue *)kmalloc(sizeof(struct Queue), GFP_ATOMIC);
	init_queue(app->ready_queue, data.queue_type);
	app->id = get_last_app_id(&app_list.head) + 1;
	list_add_tail(&app->head, &app_list.head);	
	for(i=0;i<number_of_proc;i++)
	  app->proc_list[i] = 0;
	app->log = (struct Log_Data *)kmalloc(sizeof(struct Log_Data), GFP_ATOMIC);
	app->log->index = 0;
	print_warning("api_create_app", "api call!", YES);
	print_int("api_create_app", "cnt app", count_list(&app_list.head), YES);
	return app->id;
}

static inline int api_attach_server_to_app(struct API_Data data)
{	//attaching id2 to id1
	int app_id, server_id;
	struct Application *app_parent;
	struct Server *server;
	Children *child_server, *tmp_children;
	spin_lock_irq(&mr_lock);
	print_warning("api_attach_server_to_app", "api start", YES);
	server_id = data.id;
	app_id  = data.id2;
	app_parent = find_app(&app_list.head, app_id);
	if(app_parent == NULL)
	{
		print_warning("api_attach_server_to_app", "could not find parent app", YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	}
	server = find_server(&server_list.head, server_id);
	if(server == NULL)
	{
		print_warning("api_attach_server_to_app", "could not find child server", YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	}
	tmp_children = find_children(&app_parent->servers.head, server_id, SERVER_TYPE);
	if(tmp_children != NULL)
	{	
		print_int("api_attach_server_to_app", "server is already attached!", tmp_children->id, YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;	
	}
	 /*Adding server to children list*/
	child_server = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	child_server->id = server_id;
	child_server->type = SERVER_TYPE;
	child_server->server = server;
	list_add_tail(&child_server->head, &app_parent->servers.head);
	server->parent_app = app_parent;
	app_parent->proc_list[server->proc_id] = 1;
	print_int("api_attach_server_to_app", "list cnt =", count_list(&app_parent->servers.head), YES);	
	print_warning("api_attach_server_to_app", "end of api call!", YES);
	spin_unlock_irq(&mr_lock);
	return RES_SUCCESS;
}

static inline int api_attach_task_to_app(struct API_Data data)
{	//attaching id2 to id1
	int app_id, task_id;
	struct Application *app_parent;
	struct Task *task;
	Children *child_task, *tmp_children;
	print_warning("api_attach_task_to_app", "api start", YES);
	task_id = data.id;
	app_id  = data.id2;
	app_parent = find_app(&app_list.head, app_id);
	if(app_parent == NULL)
	{
		print_warning("api_attach_task_to_app", "could not find parent app", YES);
		return RES_FAULT;
	}
	task = find_task(&task_list.head, task_id);
	if(task == NULL)
	{
		print_warning("api_attach_task_to_app", "could not find the child task", YES);
		return RES_FAULT;
	}
	tmp_children = find_children(&app_parent->tasks.head, task_id, TASK_TYPE);
	if(tmp_children != NULL)
	{	
		print_int("api_attach_task_to_app", "task is already attached!", tmp_children->id, YES);
		return RES_FAULT;	
	}
	 /*Adding server to children list*/
	child_task = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	child_task->id = task_id;
	child_task->type = TASK_TYPE;
	child_task->task = task;
	list_add_tail(&child_task->head, &app_parent->tasks.head);
	task->parent_app = app_parent;
	print_int("api_attach_task_to_app", "list cnt =", count_list(&app_parent->tasks.head), YES);	
	print_warning("api_attach_task_to_app", "end of api call!", YES);
	return RES_SUCCESS;
}
static inline int api_intra_app_sched_event(struct API_Data data)
{	//attaching id2 to id1
	int app_id;
	struct Application *app;
	print_warning("api_intra_app_sched_event", "api start", YES);
	app_id  = data.id;
	app = find_app(&app_list.head, app_id);
	if(app == NULL)
	{
		print_warning("api_intra_app_sched_event", "could not find app", YES);
		return RES_FAULT;
	}
	print_int("api_intra_app_sched_event", "app id", app->id, YES);
	//Scheduling event
	if(run_first_element(app->ready_queue) == RES_FAULT)
	  return RES_FAULT;
	print_warning("api_intra_app_sched_event", "end of api call!", YES);
	return RES_SUCCESS;
}

static inline int api_get_ctrl_data(struct API_Data data)
{
	struct Ctrl_Data ctrl_data;
	struct Application *app;
	Children *child_server;
	struct list_head *pos;
	int i, total_mu, total_beta;
	spin_lock_irq(&mr_lock);
	print_warning("api_get_ctrl_data", "start", YES);
	app = find_app(&app_list.head, data.id);
	if(app == NULL)
	{
		print_warning("api_get_ctrl_data", "could not find app", YES);
		scheduling_error = -1;
		spin_unlock_irq(&mr_lock);
		return RES_FAULT;
	}
	find_slacks(&server_list.head);
	//initializing
	print_warning("api_get_ctrl_data", "init starts", YES);
	for(i=0;i<number_of_proc;i++)
	{
	    ctrl_data.server_id[i] = -1;
	    ctrl_data.sp[i] = 0;
	    ctrl_data.slacks[i] = slacks[i];
	    ctrl_data.proc_list[i] = app->proc_list[i];
	}
	ctrl_data.beta = -1;
	ctrl_data.mu = -1;
	print_warning("api_get_ctrl_data", "init finished", YES);
	total_beta = 0;
	total_mu = 0;
	list_for_each(pos, &app->servers.head)
	{
		child_server = list_entry(pos, Children, head);
		if (SERVER_TYPE == child_server->type && SERVER_DETACHED != child_server->server->status)// && child_server->server->status == SERVER_ACTIVE)
		{
		    print_int("api_get_ctrl_data", "server id", child_server->server->id, YES);
		    if(child_server->server->status == SERVER_ACTIVE)
		    {
			app->total_budget += jiffies - child_server->server->timestamp_beta;
			child_server->server->timestamp_beta = jiffies;
			print_int_int("api_get_ctrl_data", "SERVER_ACTIVE: total_budget", app->id, app->total_budget, YES);
			if(child_server->server->running_task != NULL)
			{
			      if(child_server->server->running_task->abs_deadline >= jiffies) //if has not missed its deadline
				    app->useful_budget+= jiffies - child_server->server->running_task->timestamp;
			      else if(child_server->server->running_task->missing_dl_flag == YES)
					app->mu += jiffies - child_server->server->running_task->timestamp;
				  else
					app->mu += jiffies - child_server->server->running_task->abs_deadline;
			      
			      child_server->server->running_task->timestamp = jiffies;
			}
		    }
		    ctrl_data.server_id[child_server->server->proc_id] = child_server->server->id;
		    ctrl_data.sp[child_server->server->proc_id] = child_server->server->budget;
		}
	}
	ctrl_data.beta = app->total_budget - app->useful_budget;
	if(ctrl_data.beta < 0)
	      ctrl_data.beta = 0;
	total_beta = ctrl_data.beta;
	app->useful_budget = 0;
	app->total_budget = 0;
	print_int_int("api_get_ctrl_data", "total_budget", app->id, app->total_budget, YES);
	print_int_int("api_get_ctrl_data", "useful_budget", app->id, app->total_budget, YES);
	ctrl_data.mu = app->mu;
	total_mu = app->mu;
	app->mu = 0;
	ctrl_data.dl_miss = app->dl_miss;
	ctrl_data.importance = app->importance;
	ctrl_data.priority = app->priority;
	ctrl_data.alpha = app->bandwidth;
	ctrl_data.period = app->period;
	
	if (copy_to_user(data.ctrl, &ctrl_data, sizeof(struct Ctrl_Data))) {
		print_warning("api_get_ctrl_data", "failed to copy data to user", YES);
	}
	/*
	 * Logging 
	 */
	if(app->log == NULL)
	{
	    print_warning("api_get_ctrl_data", "app log is null", YES);
	    scheduling_error = -1;
	    spin_unlock_irq(&mr_lock);
	    return RES_FAULT;
	}
	if(app->log->index < LOG_LENGTH)
	{
	    app->log->time[app->log->index] = jiffies - start_time;
	    app->log->dl_miss[app->log->index] = app->dl_miss;
	    app->log->alpha[app->log->index] = app->bandwidth;
	    app->log->mu[app->log->index] = total_mu;
	    app->log->beta[app->log->index] = total_beta;
	    app->log->period[app->log->index] = app->period;
	}
	app->log->index++;
	/*
	 * Resetting
	 */
	app->dl_miss = 0;
      print_warning("api_get_ctrl_data", "end of api call!", YES);
      spin_unlock_irq(&mr_lock);
      return RES_SUCCESS;
}

static inline int api_get_mnger_data(struct API_Data data)
{
  struct Mnger_Data mnger_data;
  struct Application *app;
  struct list_head *pos, *ser_pos;
  Children *child_server;
  int i,j;
  spin_lock_irq(&mr_lock);
  for(i=0;i<max_number_of_apps;i++)
	for(j=0;j<number_of_proc;j++)
	{
	      mnger_data.sp[i][j] = 0;
	      mnger_data.server_ids[i][j] = -1;
	}
	list_for_each(pos, &app_list.head)
	{
		app = list_entry(pos, struct Application, head);
		print_int("api_get_mnger_data", "server id", app->id, YES);
		mnger_data.alpha[app->id] = app->bandwidth;
		mnger_data.importance[app->id] = app->importance;
		mnger_data.periods[app->id] = app->period;
		mnger_data.priorities[app->id] = app->priority;
		print_int("api_get_mnger_data", "importance", app->importance, YES);
		list_for_each(ser_pos, &app->servers.head)
		{
			child_server = list_entry(ser_pos, Children, head);
			if (SERVER_TYPE == child_server->type && SERVER_DETACHED != child_server->server->status)// && child_server->server->status == SERVER_ACTIVE)
			{
			    print_int("api_get_mnger_data", "server id", child_server->server->id, YES);
			    mnger_data.sp[app->id][child_server->server->proc_id] = child_server->server->budget;
			    mnger_data.server_ids[app->id][child_server->server->proc_id] = child_server->server->id;
			}
		}
	}
   mnger_data.number_of_apps = get_last_app_id(&app_list.head) +1;
	if (copy_to_user(data.mnger, &mnger_data, sizeof(struct Mnger_Data))) {
		print_warning("api_get_mnger_data", "failed to copy data to user", YES);
	}
      print_warning("api_get_mnger_data", "end of api call!", YES);
  spin_unlock_irq(&mr_lock);
  return RES_SUCCESS;
}
static inline int api_set_app_param(struct API_Data data)
{
  struct Application *app;
  spin_lock_irq(&mr_lock);
  app = find_app(&app_list.head, data.id);
  if(app == NULL)
  {
	  print_warning("api_set_app_param", "could not find app", YES);
	  scheduling_error = -1;
	  spin_unlock_irq(&mr_lock);
	  return RES_FAULT;
  }
  app->bandwidth = data.alpha;
  app->period = data.period;
  app->importance = data.importance;
  app->priority = data.priority;
  print_int("api_set_app_param", "bandwidth", app->bandwidth, YES);
  print_long("api_set_app_param", "period", data.period, YES);
  print_int("api_set_app_param", "period", app->period, YES);
  print_int_int("api_set_app_param", "importance", app->importance, data.importance, YES);
  print_warning("api_set_app_param", "end of api call!", YES);
  if(schedulable(&server_list.head) == RES_FAULT)
  {
	  print_warning("api_set_app_param", "unschedulable", YES);
	  scheduling_error = -1;
	  spin_unlock_irq(&mr_lock);
	  return RES_FAULT;
  }
  spin_unlock_irq(&mr_lock);
  return RES_SUCCESS;
}