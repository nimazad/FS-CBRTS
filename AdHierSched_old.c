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
	int ret;
	/* get the device number of a char device. */
	ret = alloc_chrdev_region(&dev_id, 0, 1, MODULE_NAME);
	if (ret < 0) {
		print_warning("hsf_init", "failed to allocate device");
		return ret;
	}
	/* initialize the char device. */
	cdev_init(&c_dev, &hsf_fops);
	/* register the char device. */
	ret = cdev_add(&c_dev, dev_id, 1);
	if (ret < 0) {
		print_warning("hsf_init", "failed to register device");
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
	spin_lock_init(&mr_lock);
	print_long("","", HZ);
	print_warning("AdHierSched_init", "Hello!");
	return 0;
}

static void __exit hsf_exit(void) 
{
	/* delete the char device. */
	cdev_del(&c_dev);
	/* return back the device number. */
	unregister_chrdev_region(dev_id, 1);

	print_warning("AdHierSched_exit", "Bye!");
}
module_init(hsf_init);
module_exit(hsf_exit);
/*===============================Handlers===============================================*/
int my_tmp_cnt = 0;
#ifdef TEST_TIMER
static enum hrtimer_restart hrhandler(struct hrtimer *timer)
{
    struct Handler_Data *data;
//     ktime_t ktime;
    data = container_of(timer, struct Handler_Data , timer);
    //print_int("hrhandler","start", data->id);
     struct timespec t, diff;//, add;
    //struct tm broken;
    //do_gettimeofday(&t);
    //time_to_tm(t.tv_sec, 0, &broken);
    //printk("%d:%d:%d:%ld\n", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec);
    //clock_gettime( CLOCK_REALTIME, &t);
    t = current_kernel_time();
    print_long_long("hrhandler", "set time", data->my_timespec.tv_sec, data->my_timespec.tv_nsec);
    print_long_long("hrhandler", "now     ", t.tv_sec, t.tv_nsec);
    diff = timespec_sub(data->my_timespec, t);
    print_long_long("hrhandler", "diff==================>>>>>>", diff.tv_sec,NS_TO_MS(diff.tv_nsec));
    if(scheduling_error == -1)
    {	  
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("hrhandler","scheduling error has happend!!!");
	#endif
	return HRTIMER_NORESTART;
    }
    if(Module_status == MOD_STOP)
    {	  
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("hrhandler","Module has been stopped!!!");
	#endif
	return HRTIMER_NORESTART;
    }
    switch(data->handler_type)
    {
      case SERVER_RELEASE_HANDLER:
	;
	break;
      case SERVER_BUDGET_HANDLER:
	;
	break;
      case TASK_RELEASE_HANDLER:
      ;
      break;
      case CBS_BUDGET_HANDLER:
      ;
      break;
      default:
	#ifdef DEBUG_SCHED
	print_warning("if_i_can_preempt_running", "compare_elements error running_task_element ACT_NULL");
	#endif
	break;
    }
    return HRTIMER_NORESTART;
}
static void handler(unsigned long input_data)
{
  struct Handler_Data *data;
//   long i = 0;
  //local_irq_disable();
  print_time("", "now", get_time_now());
  return;
  data = (struct Handler_Data *) input_data;
  print_int("handler", "-----", my_tmp_cnt);
  if(scheduling_error == -1)
  {	  
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler","scheduling error has happend!!!");
      #endif
      return;
  }
  if(Module_status == MOD_STOP)
  {	  
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler","Module has been stopped!!!");
      #endif
      return;
  }
  if(data->timestamp != jiffies)
  {
      scheduling_error = -1;
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler", "===**=== ERROR: handler is running at wrong jiffies");
      #endif
      return;
  }
  print_time_now("handler", "now");
//   if(data->id == 1 && my_tmp_cnt<5000)
//   {
// 	  my_tmp_cnt++;
// // 	  while(i < 30000000000)
// // 	      i++;
// 	  #ifdef JIFFY_TIMER
// 	  insert_timer(&timer1, handler, (unsigned long) data, jiffies + 10);
// 	  #endif
//   }
//   else
//     print_time("handler", "finished", jiffies);  
//   
//   if(data->task != NULL && data->task->linux_task != NULL)
//   {
//       if(my_tmp_cnt%2 == 1)
//       {
// 	    wake_up_process(data->task->linux_task);
//       }
//       else
//       {
// 	    data->task->linux_task->state = __TASK_STOPPED;//TASK_UNINTERRUPTIBLE;
// 	    set_tsk_need_resched(data->task->linux_task);
//       }
//   }
  
  //local_irq_enable();
}
#endif
static void handler_task_release(unsigned long input_data)
{
  struct Handler_Data *data;
  struct timespec tmp_time_start, tmp_time_end, overhead;
  getnstimeofday(&tmp_time_start);
  scheduling_flag = TASK_RELEASE_HANDLER;
//   local_irq_disable();
  spin_lock_irq(&mr_lock);
  INT_CNT = 0;
  data = (struct Handler_Data *) input_data;
  if(timer_error_on && data->timestamp != jiffies)
  {
      scheduling_error = -1;
      #ifdef DEBUG_SCHED_ERRORS
      print_int("handler_task_release", "===**=== ERROR: handler is running at wrong jiffies", jiffies-data->timestamp);
      #endif
      print_time("", "", data->timestamp - start_time);
      print_time_now("handler_task_release", "now");
      spin_unlock_irq(&mr_lock);
      return;
  }
  #ifdef DEBUG_HANDLER_TASK_RELEASE
  print_time("handler_task_release", "task release handler start", jiffies);
  #endif
  if(Module_status == MOD_STOP)
  {	  
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler_task_release","Module has been stopped!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  if(scheduling_error == -1)
  {	  
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler_task_release","scheduling error has happend!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  
  if(data->task == NULL)
  {
      scheduling_error = -1;
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler_task_release", "===**=== ERROR: task is null");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  if(data->task->state == TASK_STOP)
  {	  
      #ifdef DEBUG_HANDLER_TASK_RELEASE
      print_warning("handler_task_release","task is deactive!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  INT_CNT = data->task->id;
  #ifdef JIFFY_TIMER
  remove_timer(&data->task->period_timer);
  #endif
  //if(down_interruptible(&handler_sem) == 0)
  //spin_lock_irq(&mr_lock);
  /* critical section ... */
  {
//     data->task->release_time = jiffies + data->task->period;
//     print_int("handler_task_release", "increasing release time of task", data->task->id);
    data->task->cnt++;
    if(data->task->parent != NULL)
	data->task->parent->total_child_job++;
//     print_int("handler_task_release", "inserting task to queue", data->task->id);
    release_hsf_task(data->task);
    data->task->state = TASK_READY;
    data->task->abs_deadline = data->task->release_time + data->task->relative_deadline;
    data->task->release_time += data->task->period;
    try_to_run_task(data->task);
  }
  #ifdef DEBUG_HANDLER_TASK_RELEASE
  print_time("handler_task_release", "task release handler finish critical", jiffies);
  #endif
  getnstimeofday(&tmp_time_end);
  overhead = timespec_sub(tmp_time_end, tmp_time_start);
  total_overhead += overhead.tv_nsec + overhead.tv_sec*1000000000;
  spin_unlock_irq(&mr_lock);
//   local_irq_enable();
}
static void handler_server_release(unsigned long input_data)
{
  struct Handler_Data *data;
  struct timespec tmp_time_start, tmp_time_end, overhead;
  getnstimeofday(&tmp_time_start);
  scheduling_flag = SERVER_RELEASE_HANDLER;
  INT_CNT = 0;
//   local_irq_disable();
  spin_lock_irq(&mr_lock);
  data = (struct Handler_Data *) input_data;

//   if(jiffies - start_time > 100)
//   {scheduling_error = -1;}
  #ifdef DEBUG_CHECKS
  if(timer_error_on && data->timestamp != jiffies)
  {
      scheduling_error = -1;
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler_server_release", "===**=== ERROR: handler is running at wrong jiffies");
      print_time("", "", data->timestamp - start_time);
      print_time_now("handler_server_release", "now");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  #endif
  if(Module_status == MOD_STOP)
  {	  
      #ifdef DEBUG_HANDLER_SERVER_RELEASE
      print_warning("handler_server_release","Module has been stopped!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  if(scheduling_error == -1)
  {	  
      #ifdef DEBUG_HANDLER_SERVER_RELEASE
      print_warning("handler_server_release","scheduling error has happend!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  #ifdef DEBUG_HANDLER_SERVER_RELEASE
  print_time("handler_server_release", "server release handler", jiffies);
  #endif
  if(data->server == NULL)
  {
      scheduling_error = -1;
      #ifdef DEBUG_HANDLER_SERVER_RELEASE
      print_warning("handler_server_release", "===**=== ERROR: server is null");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  if(data->server->status == SERVER_DEACTIVE)
  {	  
      #ifdef DEBUG_HANDLER_SERVER_RELEASE
      print_warning("handler_server_release","server is deactive!!!");
      #endif
      spin_unlock_irq(&mr_lock);
      return;
  }
  if(limited_executions && data->server->cnt > 5)
  {
//       api_stop();
      print_warning("handler_server_release","server job cnt > 5");
      scheduling_error = -1;
      spin_unlock_irq(&mr_lock);
      return;
  }
  INT_CNT = data->server->id;
  #ifdef JIFFY_TIMER
  remove_timer(&data->server->period_timer);
  #endif
  //if(down_interruptible(&handler_sem) == 0)
  //spin_lock_irq(&mr_lock);
  
  #ifdef DEBUG_HANDLER_SERVER_RELEASE
  print_int_int("handler_server_release", "server release handler start critical, server id",data->server->id, INT_CNT);
  #endif
  /* critical section ... */
  if(data->server->cnt > 0)
      budget_controller(data->server);

//   print_warning("handler_server_release", "handler releasing server");
  
  release_server(data->server);
	
  #ifdef DEBUG_HANDLER_SERVER_RELEASE
  print_int_int("handler_server_release", "server release handler end critical, server id",data->server->id, INT_CNT);
  #endif
  getnstimeofday(&tmp_time_end);
  overhead = timespec_sub(tmp_time_end, tmp_time_start);
  total_overhead += overhead.tv_nsec + overhead.tv_sec*1000000000;
  spin_unlock_irq(&mr_lock);
//   local_irq_enable();
}
static void handler_server_budget(unsigned long input_data)
{
    struct Handler_Data *data;
    struct Server *tmp_server;
    struct Queue *tmp_queue;
    struct Queue_element *e;
    struct timespec tmp_time_start, tmp_time_end, overhead;
    getnstimeofday(&tmp_time_start);
    scheduling_flag = SERVER_BUDGET_HANDLER;
    INT_CNT = 0;
//     local_irq_disable();
//     if(down_interruptible(&handler_sem) == 0)
    spin_lock_irq(&mr_lock);
    data = (struct Handler_Data *) input_data;
    #ifdef DEBUG_CHECKS
    if(timer_error_on && data->timestamp != jiffies)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_server_budget", "===**=== ERROR: handler is running at wrong jiffies");
	print_time("", "", data->timestamp - start_time);
	print_time_now("handler_server_budget", "now");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    #endif
    #ifdef DEBUG_HANDLER_SERVER_BUDGET
    print_time("handler_server_budget", "server budget handler", jiffies);
    #endif
    if(Module_status == MOD_STOP)
    {	  
	#ifdef DEBUG_HANDLER_SERVER_BUDGET
	print_warning("handler_server_budget","Module has been stopped!!!");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(scheduling_error == -1)
    {	  
	#ifdef DEBUG_HANDLER_SERVER_BUDGET
	print_warning("handler_server_budget","scheduling error has happend!!!");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(data->server == NULL)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_server_budget", "===**=== ERROR: server is null");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(data->server->status == SERVER_DEACTIVE)
    {	  
	#ifdef DEBUG_HANDLER_SERVER_BUDGET
	print_warning("handler_server_budget","server is deactive!!!");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    INT_CNT = data->server->id;
    //destroy_timer_on_stack(&data->server->budget_timer);
    #ifdef JIFFY_TIMER
    remove_timer(&data->server->budget_timer);
    #endif
    #ifdef DEBUG_HANDLER_SERVER_BUDGET
    print_int_int("handler_server_budget", "server budget handler start critical, server id",data->server->id, INT_CNT);
    #endif
  /* critical section ... */
    {
//       if(active_server == data->server || ( active_server != NULL && is_ancestor(data->server, active_server) == YES))
      if(active_server != NULL && is_ancestor(data->server, active_server) == YES)
      {
	  tmp_server =  active_server;
	  stop_server(active_server);
	  if(tmp_server->parent == NULL)
	      tmp_queue = &Global_Ready_Queue;
	  else
	      tmp_queue = tmp_server->parent->ready_queue;
	  if(tmp_server != data->server)
	  {
	      if(search_queue_server(tmp_queue, tmp_server) != NULL)
	      {
		  #ifdef DEBUG_SCHED_ERRORS
		  print_warning("handler_server_budget", "===**=== ERROR: server already in the ready queue");
		  #endif
		  scheduling_error = -1;
		  spin_unlock_irq(&mr_lock);
		  return;
	      }
	      if(tmp_server->current_budget > 0)
	      {
    // 		print_warning("preempt", "inserting server");
		  e = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
		  e->element_type = SERVER_TYPE;
		  e->server = tmp_server;
		  if(insert_queue(tmp_queue, e) == RES_FAULT)
		  {
		    #ifdef DEBUG_SCHED_ERRORS
		    print_warning("handler_server_budget", "===**=== ERROR: insert_queue failed");
		    scheduling_error = -1;
		    #endif
		    spin_unlock_irq(&mr_lock);
		    return;
		  }
    // 		print_queue(preemptee->parent->ready_queue);
	      }
	      else
	      {;
// 		  print_warning("handler_server_budget", "preemptee budget is zero");
	      }
	  }
      }
// 	  print_int("handler_server_budget", "server budget handler deplited, stopping and running queue, server id",data->server->id);
      stop_server(data->server);
      //if you are stoping the server, try to run the next one
      if(data->server->parent == NULL)
	  Current_Ready_Queue =  &Global_Ready_Queue;
      else
	  Current_Ready_Queue = data->server->parent->ready_queue;
// 	  print_warning("handler_server_budget", "Current_Ready_Queue is changed and calling run_queue");
      run_queue(Current_Ready_Queue);    
//       }
    }
    
    #ifdef DEBUG_HANDLER_SERVER_BUDGET
    print_int_int("handler_server_budget", "server budget handler end critical, server id",data->server->id, INT_CNT);
    #endif
    getnstimeofday(&tmp_time_end);
    overhead = timespec_sub(tmp_time_end, tmp_time_start);
    total_overhead += overhead.tv_nsec + overhead.tv_sec*1000000000;
    spin_unlock_irq(&mr_lock);
//     local_irq_enable();
//     test_flag = 0;
}
static void handler_cbs_budget(unsigned long input_data)
{
    struct Handler_Data *data;
    struct Queue_element *task_element;
    int running_flag;
    scheduling_flag = CBS_BUDGET_HANDLER;
    INT_CNT = 0;
    spin_lock_irq(&mr_lock);
    data = (struct Handler_Data *) input_data;
    #ifdef DEBUG_CHECKS
    if(timer_error_on && data->timestamp != jiffies)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_cbs_budget", "===**=== ERROR: handler is running at wrong jiffies");
	print_time("", "", data->timestamp - start_time);
	print_time_now("handler_cbs_budget", "now");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    #endif
    #ifdef DEBUG_HANDLER_CBS_BUDGET
    print_time("handler_cbs_budget", "cbs budget handler start", jiffies);
    #endif
    if(Module_status == MOD_STOP)
    {	  
	#ifdef DEBUG_HANDLER_CBS_BUDGET
	print_warning("handler_cbs_budget","Module has been stopped!!!");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(scheduling_error == -1)
    {	  
	#ifdef DEBUG_HANDLER_CBS_BUDGET
	print_warning("handler_cbs_budget","scheduling error has happend!!!");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(data->server == NULL)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_cbs_budget", "===**=== ERROR: server is null");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(data->task == NULL)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_cbs_budget", "===**=== ERROR: task is null");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
     if(data->task->cbs == NULL)
    {
	scheduling_error = -1;
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_cbs_budget", "===**=== ERROR: task cbs is null");
	#endif
	spin_unlock_irq(&mr_lock);
	return;
    }
    INT_CNT = data->server->id;

    //if(down_interruptible(&handler_sem) == 0)
    task_element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
    task_element->element_type = TASK_TYPE;
    task_element->task = data->task;
   
    /* Since it might accure at the same jiffies as server_budget_handler we might not need to stop */
    if(data->task->state == TASK_RUNNING_HSF)
	running_flag = 1;
    else
	running_flag = 0;
    if(running_flag)
	stop_task(data->task);
    //Replanish budget
    
    data->task->cbs->current_budget = data->task->cbs->budget;
    data->task->cbs->abs_deadline = jiffies + data->task->cbs->relative_deadline;
    data->task->abs_deadline = data->task->cbs->abs_deadline;
    
    if(!running_flag)
    {
	spin_unlock_irq(&mr_lock);
	return;
    }
    if(data->task->parent == NULL)
	Current_Ready_Queue =  &Global_Ready_Queue;
    else
	Current_Ready_Queue = data->task->parent->ready_queue;
//     print_warning("handler_cbs_budget", "Current_Ready_Queue is changed");
    /*Should be inserted to the reqady queue again*/
    if(search_queue_task(Current_Ready_Queue, data->task) != NULL)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("handler_cbs_budget", "===**=== ERROR: task is already in the ready queue");
	print_queue(Current_Ready_Queue);
	#endif
	scheduling_error = -1;
	return ;
    }
//     print_int("handler_cbs_budget", "inserting task to queue", data->task->id);
    if(insert_queue(Current_Ready_Queue, task_element) == RES_FAULT)
    {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("handler_cbs_budget", "===**=== ERROR: insert_queue failed");
      #endif
      scheduling_error = -1;
      spin_unlock_irq(&mr_lock);
      return;
    }
//     insert_queue(Current_Ready_Queue, task_element);  
    data->task->state = TASK_READY;
//     print_time("handler_cbs_budget", "calling run_queue", jiffies);
    run_queue(Current_Ready_Queue);    
    #ifdef DEBUG_HANDLER_CBS_BUDGET
    print_time("handler_cbs_budget", "cbs budget handler finish", jiffies);
    #endif
//     local_irq_enable();
    spin_unlock_irq(&mr_lock);
}
/*===============================Scheduler funcrions===============================================*/
static inline void print_status(void)
{
  print_warning("print_status", "===============================");
  if(running_task != NULL)
  {
      print_int("print_status","TASK is running", running_task->id);
  }
  if(active_server != NULL)
  {
      print_int("print_status","SERVER is actie", active_server->id);
  }
  print_warning("print_status", "===============================");
}
/*
 * if head of the queue can preempt the running element
 */
static inline int if_i_can_preempt_running(struct Task *task, struct Server *server)
{
  struct Queue_element *queue_head, *running_task_element, *active_server_element, *anc_active_server_element;
  struct Server *ancestor_active_server = NULL;
  int case_number;
  int queue_type;
//   if(running_task == NULL && active_server == NULL)
//       return YES;
  #ifdef DEBUG_CHECKS
  if(check_queue(Current_Ready_Queue) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("if_i_can_preempt_running", "===**=== ERROR: checking current ready_queue failed");
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  #endif
  #ifdef DEBUG_SCHED
  print_warning("if_i_can_preempt_running", "start");
  #endif
  
  queue_head	= (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
  #ifdef DEBUG_SCHED
  print_warning("if_i_can_preempt_running", "here");
  #endif
  if(task == NULL)
  {
      if(server == NULL)
      {
	  #ifdef DEBUG_SCHED_ERRORS
	  print_warning("if_i_can_preempt_running", "===**=== ERROR: input task and server are a NULL pointers");
	  #endif
	  scheduling_error = -1;
	  return RES_FAULT;
      }
      #ifdef DEBUG_CHECKS
      if(check_server(server) == RES_FAULT)
      {
	    #ifdef DEBUG_SCHED_ERRORS
	    print_warning("if_i_can_preempt_running", "===**=== ERROR: check_server failed");
	    #endif
	    scheduling_error = -1;
	    return RES_FAULT;
      }
      #endif
      queue_head->element_type = SERVER_TYPE;
      queue_head->server = server;
      if(server->parent == NULL)
	queue_type = Global_Ready_Queue.queue_type;
      else
	queue_type = server->parent->ready_queue->queue_type;
  }
  else
  {
      queue_head->element_type = TASK_TYPE;
      queue_head->task = task;
      if(task->parent == NULL)
	queue_type = Global_Ready_Queue.queue_type;
      else
	queue_type = task->parent->ready_queue->queue_type;
  }
  running_task_element	= (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
  active_server_element	= (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
  anc_active_server_element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
  running_task_element->element_type	= TASK_TYPE;
  running_task_element->task		= running_task;
  active_server_element->element_type	= SERVER_TYPE;
  active_server_element->server		= active_server;
  anc_active_server_element->element_type	= SERVER_TYPE;
  anc_active_server_element->server		= ancestor_active_server;
  
  #ifdef DEBUG_SCHED
  print_warning("if_i_can_preempt_running", "start2");
  #endif
  case_number = UNKNOWN;
  #ifdef DEBUG_SCHED
  print_warning("if_i_can_preempt_running", "finding case ...");
  #endif
  if(queue_head->element_type == SERVER_TYPE)
  {
	if(active_server == NULL)
	{
	    if(queue_head->server->parent == NULL)
	    {
		if(running_task == NULL)
		    case_number = ACT_NULL_GLOBAL_RUN_NULL;
		else
		{
		    #ifdef DEBUG_SCHED
		    print_warning("if_i_can_preempt_running", "wants to compare");
		    #endif
		    #ifdef DEBUG_CHECKS
		    if(check_queue(Current_Ready_Queue) == RES_FAULT)
		    {
			#ifdef DEBUG_SCHED_ERRORS
			print_warning("if_i_can_preempt_running", "===**=== ERROR: checking current ready_queue failed");
			#endif
			scheduling_error = -1;
			return RES_FAULT;
		    }
		    #endif
		    switch(compare_elements(running_task_element, queue_head, queue_type))
		    {
		      case YES:
			case_number = ACT_NULL_CAN_NOT_PREEMPT_RUN;
			break;
		      case NO:
			case_number = ACT_NULL_CAN_PREEMPT_RUN;
			break;
		      default:
			#ifdef DEBUG_SCHED
			print_warning("if_i_can_preempt_running", "compare_elements error running_task_element ACT_NULL");
			#endif
			break;
		    }
			
		}
	    }
	    else
		case_number = ACT_NULL_NOT_GLOBAL;
	}
	else // Active server not NULL
	{
	    if(server->parent == active_server->parent)
	    {
		switch(compare_elements(active_server_element, queue_head, queue_type))
		  {
		    case YES:
		      case_number = ACT_SIBL_CAN_NOT_PREEMPT;
		      break;
		    case NO:
		      case_number = ACT_SIBL_CAN_PREEMPT;
		      break;
		    default:
		      #ifdef DEBUG_SCHED
		      print_warning("if_i_can_preempt_running", "compare_elements error active_server_element");
		      #endif
		      break;
		  }
	    }
	    else // I am not at the same level as the active server
	    {
		ancestor_active_server = if_outrank(server, active_server);
		anc_active_server_element->server = ancestor_active_server;
		 if(ancestor_active_server == NULL)
		 {
		    #ifdef DEBUG_SCHED
		    print_warning("if_i_can_preempt_running", "ancestor_active_server is NULL!!!");
		    #endif
		 }
		if(check_server(ancestor_active_server) != RES_FAULT)
		{
		    switch(compare_elements(anc_active_server_element, queue_head, queue_type))
		    {
		      case YES:
			case_number = OUTRANK_ACT_CAN_NOT_PREEMPT_ANC;
			break;
		      case NO:
			case_number = OUTRANK_ACT_CAN_PREEMPT_ANC;
			break;
		      default:
			#ifdef DEBUG_SCHED
			print_warning("if_i_can_preempt_running", "compare_elements error anc_active_server_element");
			#endif
			break;
		    }
		}
		else 
		{
		    case_number = NOT_OUTRANK_ACT;
		}
		if(server->parent == active_server && active_server != NULL)
		{
		    if(running_task == NULL)
			case_number = ACT_PAR_RUN_NULL;
		    else
		    {
			switch(compare_elements(running_task_element, queue_head, queue_type))
			{
			  case YES:
			    case_number = ACT_PAR_CAN_NOT_PREEMPT_RUN;
			    break;
			  case NO:
			    case_number = ACT_PAR_CAN_PREEMPT_RUN;
			    break;
			  default:
			    #ifdef DEBUG_SCHED
			    print_warning("if_i_can_preempt_running", "compare_elements error running_task_element");
			    #endif
			    break;
			}
		    }
		}
	    }
	}
  }
  
  else if(queue_head->element_type == TASK_TYPE)
  {
	#ifdef DEBUG_SCHED
	print_warning("if_i_can_preempt_running", "queue head is task type");
	#endif
	if(active_server != queue_head->task->parent)
	    case_number = PAR_SERV_NOT_ACT;
	else
	{
	    if(running_task == NULL)
		case_number = RUN_TASK_NULL;
	    else
	    {
		running_task_element->task = running_task;
		switch(compare_elements(running_task_element, queue_head, queue_type))
		  {
		    case YES:
		      case_number = RUN_SIBL_CAN_NOT_PREEMPT;
		      break;
		    case NO:
		      case_number = RUN_SIBL_CAN_PREEMPT;
		      break;
		    default:
		      #ifdef DEBUG_SCHED
		      print_warning("if_i_can_preempt_running", "compare_elements error running_task_element, task type");
		      #endif
		      break;
		  }
	    }
	}
  }
  
//   print_int("if_i_can_preempt_running","case_number", case_number);
  switch(case_number)
  {
    case RUN_TASK_NULL:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "RUN_TASK_NULL");
      #endif
      return YES;
      break;
    case RUN_SIBL_CAN_PREEMPT:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "RUN_SIBL_CAN_PREEMPT");
      #endif
      return YES;
      break;
    case ACT_NULL_GLOBAL_RUN_NULL:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "ACT_NULL_GLOBAL_RUN_NULL");
      #endif
      return YES;
      break;
    case ACT_NULL_CAN_PREEMPT_RUN:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "ACT_NULL_CAN_PREEMPT_RUN");
      #endif
      return YES;
      break;
    case ACT_SIBL_CAN_PREEMPT:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "ACT_SIBL_CAN_PREEMPT");
      #endif
      return YES;
      break;
    case OUTRANK_ACT_CAN_PREEMPT_ANC:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "OUTRANK_ACT_CAN_PREEMPT_ANC");
      #endif
      return YES;
      break;
    case ACT_PAR_RUN_NULL:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "ACT_PAR_RUN_NULL");
      #endif
      return YES;
      break;
    case ACT_PAR_CAN_PREEMPT_RUN:
      #ifdef DEBUG_SCHED
      print_warning("if_i_can_preempt_running", "ACT_PAR_CAN_PREEMPT_RUN");
      #endif
      return YES;
      break;
    default:
      #ifdef DEBUG_SCHED
      print_int("if_i_can_preempt_running", "case_number", case_number);
      #endif
      return NO;
      break;
  }

  #ifdef DEBUG_SCHED_ERRORS
  print_warning("if_i_can_preempt_running", "===**=== ERROR: NO case can be detected!!!");
  #endif
  scheduling_error = -1;
  return NO;
}
/*
 * Inserts preemptee to its ready_queue
 */
static inline int preempt(struct Server *preempter, struct Server *preemptee)
{
    struct Queue_element *e;
    #ifdef DEBUG_SCHED
    print_warning("preempt", "start");
    #endif
    #ifdef DEBUG_CHECKS
    if(check_server(preempter) == RES_FAULT || check_server(preemptee) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("preempt", "===**=== ERROR: check server faild");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
    /*
    if(is_ancestor(preempter, preemptee) == YES)
    {
	return RES_SUCCESS;
    }*/
//     print_int_int("preempt","preempter, preemptee",preempter->id, preemptee->id);
    while(preemptee != NULL && preempter != NULL && preempter->parent == preemptee->parent)
    {
//       print_warning("preempt", "while");
	e = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
	e->element_type = SERVER_TYPE;
	e->server = preemptee;
	if(preemptee->parent == NULL)
	{
// 	  print_warning("preempt", "parent null");
	      if(search_queue_server(&Global_Ready_Queue, preemptee) != NULL)
	      {
		  #ifdef DEBUG_SCHED_ERRORS
		  print_warning("preempt", "===**=== ERROR: prev already in the ready queue");
		  #endif
// 		  scheduling_error = -1;
// 		  return RES_FAULT;
	      }
	      else if(preemptee->current_budget > 0)
	      {
// 		  print_warning("preempt", "inserting server");
		  if(insert_queue(&Global_Ready_Queue, e) == RES_FAULT) 
		  {
		      #ifdef DEBUG_SCHED_ERRORS
		      print_warning("preempt", "===**=== ERROR: insert_queue failed");
		      #endif
		      scheduling_error = -1;
		      return RES_FAULT;
		  }
// 		  print_queue(&Global_Ready_Queue);
	      }
	      else
	      {
// 		    print_warning("preempt", "preemptee budget is zero");
	      }
	}
	else
	{
// 	  print_warning("preempt", "parent not null");
	    if(search_queue_server(preemptee->parent->ready_queue, preemptee) != NULL)
	    {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("preempt", "===**=== ERROR: server already in the ready queue");
		#endif
// 		scheduling_error = -1;
// 		return RES_FAULT;
	    }
	    else if(preemptee->current_budget > 0)
	    {
// 		print_warning("preempt", "inserting server");
		if(insert_queue(preemptee->parent->ready_queue, e) == RES_FAULT)
		{
		  #ifdef DEBUG_SCHED_ERRORS
		  print_warning("preempt", "===**=== ERROR: insert_queue failed");
		  scheduling_error = -1;
		  #endif
		  return RES_FAULT;
		}
// 		print_queue(preemptee->parent->ready_queue);
	    }
	    else
	    {
		  print_warning("preempt", "preemptee budget is zero");
	    }
	}
	
	preemptee = preemptee->parent;
    }
    return RES_SUCCESS;
}

static inline int run_server(struct Server *server)
{
    struct Handler_Data *data_budget;
    #ifdef DEBUG_CHECKS
    if(check_server(server) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("run_server", "===**=== ERROR: check server faild");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
    /*Run server*/
    active_server = server;
    Current_Ready_Queue = server->ready_queue;
//     print_warning("run_server", "Current_Ready_Queue is changed");
    server->timestamp_mic = get_time_now();
    server->timestamp = jiffies;
    /*Insert timer for budget*/
    data_budget = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
    data_budget->id = server->id;
    data_budget->handler_type = SERVER_BUDGET_HANDLER;
    data_budget->server = server;
   
    #ifdef JIFFY_TIMER
//     print_warning("run_server", "inserting timer");
    insert_timer(&server->budget_timer, handler_server_budget, (unsigned long) data_budget, jiffies + server->current_budget);
    #endif
    #ifdef DEBUG_SCHED_TMP
    print_int("run_server", "****************RUNNING SERVER", server->id);
    print_time_now("run_server", "running");
    print_int("","", server->current_budget);
    #endif
    // Now run my children
//     print_time("run_server", "calling run_queue", jiffies);
    
    run_queue(Current_Ready_Queue);
    return RES_SUCCESS;
}
static inline int run_task(struct Task *task)
{
    struct Handler_Data *data_budget;
    
    /*if(permission(RUN_TASK_ID) == NO)
      return RES_FAULT;*/
    
    #ifdef DEBUG_SCHED_MAIN
    print_warning("run_task", "<<<<<<<<<<---------->>>>>>>>>>");
    #endif
    #ifdef DEBUG_CHECKS
    if(check_task(task) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("run_task", "===**=== ERROR: task is a NULL pointer");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
    running_task = task;
    if(task->linux_task == NULL)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("run_task", "===**=== ERROR: linux_task is a NULL pointer");
	#endif
	print_int("run_task", "task id", task->id);
	scheduling_error = -1;
	return RES_FAULT;
    }
    if(task->parent != NULL && task->parent != active_server)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("run_task", "===**=== ERROR: task parent is not an active server");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
//     print_int("run_task", "running task", task->id);
//     print_time_now("run_task", "running");
    
    if(stopped_task == task && stopped_task->stopped_time == jiffies)
    {
	//The task was running but we stopped it just at this jiffy, so we change its state
	#ifdef DEBUG_SCHED_MAIN
	print_warning("run_task", "only changing the state");
	#endif
// 	task->linux_task->state = TASK_RUNNING;
// 	set_tsk_thread_flag(task->linux_task, 0);
// 	print_int("run_task", "waking up", task->id);
	wake_up_process(task->linux_task);
    }
    if(task->linux_task->state != TASK_RUNNING)
    {
// 	print_int("run_task", "waking up", task->id);
	wake_up_process(task->linux_task);
    }
    task->state = TASK_RUNNING_HSF;
    task->timestamp_mic = get_time_now();
    task->timestamp = jiffies;
    
    if(task->release_time < jiffies)
	task->missing_dl_flag = YES;
    else
	task->missing_dl_flag = NO;
    
    if(task->parent == NULL)
	Current_Ready_Queue =  &Global_Ready_Queue;
    else
	Current_Ready_Queue = task->parent->ready_queue;
//     print_warning("run_task", "Current_Ready_Queue is changed");
    if(task->cbs != NULL) 
    {
	data_budget = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
	data_budget->id = task->cbs->id;
	data_budget->server = task->cbs;
	data_budget->task = task;
	#ifdef DEBUG_SCHED_MAIN
	print_time("run_task", "setting timer to", jiffies + task->cbs->current_budget);
	print_time("run_task", "now", jiffies_64);
	#endif
	#ifdef JIFFY_TIMER
// 	print_warning("run_task", "inserting timer");
	insert_timer(&task->cbs->budget_timer, handler_cbs_budget, (unsigned long) data_budget, jiffies + task->cbs->current_budget);
	#endif
	task->cbs->timestamp_mic = get_time_now();
	task->cbs->timestamp = jiffies;
    }
    return RES_SUCCESS;
}
/*
 * if first is ancestor of second retun YES
 */
static inline int is_ancestor(struct Server *first, struct Server *second)
{
  #ifdef DEBUG_CHECKS
  if(check_server(first) == RES_FAULT) 
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("is_ancestor", "===**=== ERROR: check_server failed on first");
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  if(check_server(second) == RES_FAULT)
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("is_ancestor", "===**=== ERROR: check_server failed on second");
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  #endif
  while (second->parent != NULL)
  {
    if(second->parent == first)
      break;
    second= second->parent;
  }
  if(second->parent == first)
      return YES;
  else
      return NO;
}
/*
 * if first outranks second retun YES 
 * ancestor_server will be anscestor of the second server that is at the same level as the first server
 */
static inline void* if_outrank(struct Server *first, struct Server *second)
{
  if(check_server(first) == RES_FAULT || check_server(second) == RES_FAULT)
  {
    #ifdef DEBUG_SCHED
    print_warning("if_outrank", "server is null");
    #endif
    return NULL;
  }
  while (second->parent != NULL)
  {
      if(second->parent == first->parent)
	break;
      second= second->parent;
  }
  
  if(second->parent == first->parent)
      return second;
  else
      return NULL;
}

// static inline int is_descendant(struct Server *first, struct Server *second)
// {
//     return is_ancestor(second, first);
// }
static inline int run_queue(struct Queue *queue)
{
  struct Queue_element *queue_head;
  #ifdef DEBUG_CHECKS
  if(check_queue(queue) == RES_FAULT)
  {
      #ifdef DEBUG_SCHED_ERRORS
      print_warning("run_queue", "===**=== ERROR: check_queue failed");
      #endif
      scheduling_error = -1;
      return RES_FAULT;
  }
  if(check_element(queue->elements) == RES_FAULT)
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("run_queue", "===**=== ERROR: queue elements is a NULL pointer");
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  #endif
  if(list_empty(&queue->elements->head))
  {
//       print_warning("run_queue", "queue is empty");
//       print_queue(queue);
      return RES_SUCCESS;
  }
  
  queue_head = list_entry(queue->elements->head.next, struct Queue_element, head);
  #ifdef DEBUG_SCHED
  print_warning("run_queue", "reading queue head");
  #endif
  #ifdef DEBUG_CHECKS
  if(check_element(queue_head) == RES_FAULT)
  {
    #ifdef DEBUG_SCHED_ERRORS
    print_warning("run_queue", "===**=== ERROR: queue elements is a NULL pointer");
    #endif
    scheduling_error = -1;
    return RES_FAULT;
  }
  #endif
  #ifdef DEBUG_SCHED
  print_int("run_queue", "queue head element type", queue_head->element_type);
  #endif
  if(queue_head->element_type == SERVER_TYPE)
  {
      #ifdef DEBUG_SCHED
      print_warning("run_queue", "queue head is server, trying to run");
      #endif
      try_to_run_server(queue_head->server);
  }
  if(queue_head->element_type == TASK_TYPE)
  {
    #ifdef DEBUG_SCHED
    print_warning("run_queue", "queue head is task, trying to run");
    #endif
    try_to_run_task(queue_head->task);
  }
  return RES_SUCCESS;
}
/*
 * returns result (RES_FAULT or RES_SUCCESS)
 */ 
static inline int try_to_run_server(struct Server *server)
{
    struct Queue_element *queue_head;
    struct Server *tmp_server;
    struct Task *tmp_task;
    #ifdef DEBUG_CHECKS
    if(check_server(server) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("try_to_run_server", "===**=== ERROR: check server failed");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
//     print_int("try_to_run_server", "trying to run_server", server->id);
    if(if_i_can_preempt_running(NULL, server) == YES)
    {
	  if(server->parent == NULL)
	      queue_head = Global_Ready_Queue.elements;
	  else
	      queue_head = server->parent->ready_queue->elements;
	  
// 	  if(list_empty(&queue_head->head))
// 	  {
// 	      print_warning("print_queue", "queue is empty");
// 	      scheduling_error = -1;
// 	      return RES_FAULT;
// 	  }
	  #ifdef DEBUG_CHECKS    
	  if(check_element(queue_head) == RES_FAULT)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_server", "===**=== ERROR: check_element failed on queue_head");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }
	   if(queue_head->head.next == NULL)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_server", "===**=== ERROR: queue_head next is NULL");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }
	  #endif
	  queue_head = list_entry(queue_head->head.next, struct Queue_element, head);
	  #ifdef DEBUG_CHECKS
	  if(check_element(queue_head) == RES_FAULT)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_server", "===**=== ERROR: check_element failed on queue_head (2)");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }
	  #endif
	  if(queue_head->server == server)
	  {	
	      delete_queue(queue_head);
	      #ifdef DEBUG_SCHED
	      print_warning("try_to_run_server", "********DELETEING QUEUE HEAD");
	      #endif
	  }
	  else
	  {
	    //print_int("try_to_run_server", "===**=== ERROR: QUEUE HEAD is changed, tying to run:", server->id);
	    //print_int("try_to_run_server", "element type", queue_head->element_type);
	    //print_status();
// 	    if(queue_head->element_type == SERVER_TYPE)
// 		print_int("try_to_run_server", "queue_head id", queue_head->server->id);
// 	    print_queue(&Global_Ready_Queue);
// 	    scheduling_error = -1;
	    return RES_FAULT;
	  }
	  if(running_task != NULL)
	  {
		  tmp_task = running_task;
		  stop_task(running_task);
// 		  print_int("try_to_run_task", "inserting task to queue", tmp_task->id);
		  release_hsf_task(tmp_task);
		  tmp_task->state = TASK_READY;
	
	  }
	  if(active_server != NULL)
	  {
		if(active_server != server)
		{
// 		    print_warning("try_to_run_server", "preempting and running server");
		    if(is_ancestor(active_server, server) == NO)
		    {
			  tmp_server = active_server;
			  stop_server(active_server);
			  if(preempt(server, tmp_server) == RES_FAULT)
			  {
			      //print_warning("try_to_run_server", "preempting failed");
			      //print_int("try_to_run_server", "preempter", server->id);
			      //print_int("try_to_run_server", "preemptee", tmp_server->id);
			      return RES_FAULT;
			  }			  
		    }
		    run_server(server);
		}
	  }
	  else
	  {
// 		print_warning("try_to_run_server", "running server");
		run_server(server);
	  }
    }
//     else
//     {
//       print_warning("try_to_run_server", "Could not preempt running");
//       print_status();
//     }
 
return RES_SUCCESS;
}
static inline int try_to_run_task(struct Task *task)
{
    struct Queue_element *queue_head;
    struct Task *tmp_task;
    #ifdef DEBUG_SCHED_MAIN
    print_warning("try_to_run_task", "<<<<<<<<<<---------->>>>>>>>>>");
    #endif
    #ifdef DEBUG_CHECKS
    if(check_task(task) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("try_to_run_task", "===**=== ERROR: check task failed");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
    if(task->state != TASK_READY)
    {
	#ifdef DEBUG_SCHED
	print_warning("try_to_run_task", "task is not ready");
	#endif
	return RES_FAULT;
    }
    #ifdef DEBUG_SCHED
    print_warning("try_to_run_task", "start");
    #endif
    if(if_i_can_preempt_running(task, NULL) == YES)
    {
	  #ifdef DEBUG_SCHED
	  print_warning("try_to_run_task", "I can preempt running");
	  #endif
	  if(task->parent == NULL)
	      queue_head = Global_Ready_Queue.elements;
	  else
	      queue_head = task->parent->ready_queue->elements;
	  
// 	  if(list_empty(&queue_head->head))
// 	  {
// 	      print_warning("print_queue", "queue is empty");
// 	      scheduling_error = -1;
// 	      return RES_FAULT;
// 	  }
	  #ifdef DEBUG_CHECKS
	  if(check_element(queue_head) == RES_FAULT)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_task", "===**=== ERROR: check_element failed on queue_head");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }
	  
	   if(queue_head->head.next == NULL)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_task", "===**=== ERROR: queue_head next is NULL");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }
	  #endif
	  queue_head = list_entry(queue_head->head.next, struct Queue_element, head);
	  #ifdef DEBUG_CHECKS
	  if(check_element(queue_head) == RES_FAULT)
	  {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("try_to_run_task", "===**=== ERROR: check_element failed on queue_head (2)");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	  }	  
	  #endif
	  if(queue_head->task == task)
	  {
	      delete_queue(queue_head);
	      #ifdef DEBUG_SCHED
	      print_warning("try_to_run_task", "********DELETEING QUEUE HEAD");
	      #endif
	  }
	  else
	  {
	    //print_warning("try_to_run_task", "===**=== ERROR: QUEUE HEAD is changed!!!");
	    //print_status();
	    //print_queue(Current_Ready_Queue);
// 	    scheduling_error = -1;
	    return RES_FAULT;
	  }
	  
	  if(running_task != NULL)
	  {
		if(running_task != task)
		{
		  tmp_task = running_task;
		  stop_task(running_task);
// 		  print_int("try_to_run_task", "inserting task to queue", tmp_task->id);
		  release_hsf_task(tmp_task);
		  tmp_task->state = TASK_READY;
		}
// 		print_int("try_to_run_task", "running task", task->id); 
		run_task(task);
	  }
	  else//running_task == NULL
	  {
// 	      print_int("try_to_run_task", "running task", task->id); 
	      run_task(task);
	  }
    }
    else
    {
	  #ifdef DEBUG_SCHED
	  print_warning("try_to_run_task", "I can preempt running");
	  #endif
    }
 
return RES_SUCCESS;
}

static inline int replanish_server_budget(struct Server *server)
{
	#ifdef DEBUG_CHECKS
  	if(check_server(server) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("replanish_server_budget", "===**=== ERROR: server check failed");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	}
	#endif
	if(server->parent != NULL)
	{
	    if(server->current_budget < 0)
	    {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("replanish_server_budget", "===**=== ERROR: current_budget < 0");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	    }
// 	    print_time("replanish_server_budget","extra_req_budget of parent before",server->parent->extra_req_budget);
// 	    print_int("replanish_server_budget","extra_req_budget of parent budget",server->current_budget);
	    server->parent->extra_req_budget += server->current_budget;//+1;// / (server->period * server->parent->period);
// 	    print_time("replanish_server_budget","extra_req_budget of parent after",server->parent->extra_req_budget);
	}
	return RES_SUCCESS;
}


/*
 * returns result (RES_FAULT or RES_SUCCESS)
 */ 
static inline int release_server(struct Server *server)
{
  	struct Queue_element *element;
	struct Handler_Data *data_period;
	#ifdef DEBUG_CHECKS
	if(check_server(server) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_server", "===**=== ERROR: server check failed");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	}
	#endif
	#ifdef DEBUG_SCHED
	print_int_int("release_server", "RELEASING SERVER", server->id, server->cnt);
	#endif
	
	server->abs_deadline = jiffies + server->relative_deadline;
	element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
	element->element_type	= SERVER_TYPE;
	element->server		= server;
	if(active_server != server)
	{
	      if(server->parent == NULL)
	      {
		    if(search_queue_server(&Global_Ready_Queue, server) != NULL)
		    {
      // 		  #ifdef DEBUG_SCHED_ERRORS
// 			print_warning("release_server", "===**=== ERROR: server is already in the ready queue");
      // 		  print_int("release_server", "id", server->id);
      // 		  print_queue(&Global_Ready_Queue);
      // 		  print_time_now("release_server", "");
      // 		  #endif
      // 		  scheduling_error = -1;
      // 		  return RES_FAULT;
      // 		  replanish_server_budget(server);
		    }
		    else if(insert_queue(&Global_Ready_Queue, element) == RES_FAULT) 
		    {
		      #ifdef DEBUG_SCHED_ERRORS
		      print_warning("release_server", "===**=== ERROR: insert_queue failed");
		      print_int("release_server", "id", server->id);
		      #endif
		      scheduling_error = -1;
		      return RES_FAULT;
		    }
	      }
	      else
	      {
		  if(search_queue_server(server->parent->ready_queue, server) != NULL)
		  {
      // 		#ifdef DEBUG_SCHED_ERRORS
      // 		print_warning("release_server", "===**=== ERROR: server is already in the ready queue");
      // 		print_int("release_server", "id", server->id);
      // 		print_queue(server->parent->ready_queue);
      // 		print_time_now("release_server", "");
      // 		#endif
      // // 		delete_queue(tmp_element);
      // 		scheduling_error = -1;
      // 		return RES_FAULT;
		      replanish_server_budget(server);
		  }
		  else if(insert_queue(server->parent->ready_queue, element) == RES_FAULT)
		  {
		    #ifdef DEBUG_SCHED_ERRORS
		    print_warning("release_server", "===**=== ERROR: insert_queue failed");
		    #endif
		    scheduling_error = -1;
		    return RES_FAULT;
		  }
	      }
	}
	else
	{
	    replanish_server_budget(server);
// 	    server->consumed_budget += division_unsigned(get_time_now() - server->timestamp_mic, 1000);
	    if(running_task!=NULL)
	    {
		if(division_unsigned(get_time_now() - running_task->timestamp_mic, 1000) > running_task->parent->period)
	      {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_server", "consumed budget more than period");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	      }
	      running_task->parent->consumed_budget += division_unsigned(get_time_now() - running_task->timestamp_mic, 1000);
	      running_task->timestamp_mic = get_time_now();
	      running_task->timestamp = jiffies;
	      server->timestamp_mic = get_time_now();
	      server->timestamp =jiffies;
	    }
	}
	server->cnt ++;
	server->current_budget = server->budget;
	server->total_budget += server->budget;
	#ifdef DEBUG_SCHED
	print_long("release_server", "setting timer to", jiffies + server->period);
	#endif
	/*Insert timer for the next period*/
	data_period = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
	data_period->id = server->id;
	data_period->handler_type = SERVER_RELEASE_HANDLER;
	data_period->server = server;
	
	#ifdef JIFFY_TIMER
// 	print_warning("release_server", "inserting timer");
	insert_timer(&server->period_timer, handler_server_release, (unsigned long) data_period, jiffies + server->period);
	#endif
	#ifdef DEBUG_SCHED
	print_warning("release_server", "calling try to run");
	#endif
	if(active_server != server)
	    try_to_run_server(server);
	
	return RES_SUCCESS;
}
static inline int release_hsf_task(struct Task *task)
{
  	struct Queue_element *element;
	
// 	if(test_flag)
// 	  return RES_FAULT;
	#ifdef DEBUG_CHECKS
	if(check_task(task) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_hsf_task", "===**=== ERROR: task check failed");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	}
	#endif
	#ifdef DEBUG_SCHED
	print_int("release_hsf_task", "****************RELEASING TASK", task->id);
	print_time("release_hsf_task", "Release time", jiffies);
	#endif
	element = (struct Queue_element *)kmalloc(sizeof(struct Queue_element), GFP_ATOMIC);
	element->element_type	= TASK_TYPE;
	element->task		= task;
	if(task->cbs != NULL)
	{
	    #ifdef DEBUG_SCHED_TMP
	    print_int("release_hsf_task", "TASK is attached to a CBS", task->id);
	    print_time("release_hsf_task", "Rlease time", jiffies);
	    #endif
	    if(task->cbs->current_budget == 0)
	    {
		task->cbs->current_budget = task->cbs->budget;
		task->cbs->abs_deadline = jiffies + task->cbs->relative_deadline;
	    }
	    task->abs_deadline = task->cbs->abs_deadline;
	}
	if(task->parent == NULL)
	{
	      if(search_queue_task(&Global_Ready_Queue, task) != NULL)
	      {
		  #ifdef DEBUG_SCHED_ERRORS
		  print_warning("release_hsf_task", "===**=== ERROR: task is already in the ready queue");
		  print_queue(&Global_Ready_Queue);
		  #endif
		  scheduling_error = -1;
		  return RES_FAULT;
	      }
	      if(insert_queue(&Global_Ready_Queue, element) == RES_FAULT)
	      {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_hsf_task", "===**=== ERROR: insert_queue failed");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	      }
	}
	else
	{
	    if(search_queue_task(task->parent->ready_queue, task) != NULL)
	    {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("release_hsf_task", "===**=== ERROR: task is already in the ready queue");
		print_queue(task->parent->ready_queue);
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	    }
	    if(insert_queue(task->parent->ready_queue, element) == RES_FAULT)
	    {
	      #ifdef DEBUG_SCHED_ERRORS
	      print_warning("release_hsf_task", "===**=== ERROR: insert_queue failed");
	      #endif
	      scheduling_error = -1;
	      return RES_FAULT;
	    }
	}
		
	#ifdef DEBUG_SCHED_TMP
	print_warning("release_hsf_task", "task is insearted to the queue");
	print_time("release_hsf_task", "insert time", jiffies);
	#endif	
	
	return RES_SUCCESS;
}

static inline int stop_server(struct Server *server)
{
	struct Task *prev;
	if(check_server(server) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED
		print_warning("stop_server", "************** STOPING NULL SERVER");
		#endif
		return RES_SUCCESS;/*It is OK to be NULL*/
	}
// 	if(active_server == server || (is_descendant(active_server, server) == YES))
	{
	  active_server = NULL;
	  #ifdef DEBUG_SCHED
	  print_int("stop_server","consumed budget",server->consumed_budget);
	  #endif
// 	  print_int("stop_server", "STOPPING SERVER", server->id);
// 	  print_time_now("stop_server", "");
	  if(server->current_budget  > (division_unsigned(get_time_now() - server->timestamp_mic, 1000)))
	      server->current_budget = server->current_budget  - (division_unsigned(get_time_now() - server->timestamp_mic, 1000));
	  else
	      server->current_budget = 0;
	  if(server->parent != NULL)
	  {
	      if((division_unsigned(get_time_now() - server->timestamp_mic, 1000)) > server->parent->period)
	      {
		      #ifdef DEBUG_SCHED_ERRORS
		      print_warning("stop_server", "consumed_budget more than period");
		      #endif
// 		      scheduling_error = -1;
// 		      return RES_FAULT;/*It is OK to be NULL*/
	      }
	      server->parent->consumed_budget += (division_unsigned(get_time_now() - server->timestamp_mic, 1000));
// 	      print_int("stop_server","consumed budget of parent",server->parent->consumed_budget);
	  }
	  
	  #ifdef JIFFY_TIMER
	  remove_timer(&server->budget_timer);
	  #endif
	  //destroy_timer_on_stack(&server->budget_timer);
	  #ifdef DEBUG_SCHED
	  print_warning("stop_server", "****************STOPING ACTIVE SERVER"); 
	  #endif
	  if(running_task != NULL && running_task->parent == server)
	  {
	      prev = running_task;
	      stop_task(running_task);
// 	      prev->state = TASK_WAITING;
	      #ifdef DEBUG_SCHED_TMP
	      print_warning("stop_server", "****************calling release_hsf_task***********************************"); 
	      #endif
// 	      print_int("stop_server", "inserting task to queue", prev->id);
	      release_hsf_task(prev);//to inseart it into queue again	      
	      prev->state = TASK_READY;
	  }
	  return RES_SUCCESS;
	}
    return NO;
}

static inline int stop_linux_task(struct task_struct *task)
{
    if(task == NULL)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("stop_linux_task", "===**=== ERROR: task is a null pointer"); 
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    task->state = TASK_UNINTERRUPTIBLE;
    set_tsk_need_resched(task);
    //schedule();
    #ifdef DEBUG_SCHED
    print_warning("stop_linux_task", "task is stoped"); 
    #endif
    return RES_SUCCESS;
}
static inline int stop_task(struct Task *task)
{
		
	if(permission(STOP_TASK_ID) == NO)
	  return RES_FAULT;
	
	#ifdef DEBUG_SCHED_MAIN
	print_warning("stop_task", "<<<<<<<<<<---------->>>>>>>>>>");
	#endif
	#ifdef DEBUG_CHECKS
	if(check_task(task) == RES_FAULT)
	{
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("stop_task", "===**=== ERROR: TASK check failed");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	}
	if(task->linux_task == NULL)
	{
	    #ifdef DEBUG_SCHED_ERRORS
	    print_warning("stop_task", "===**=== ERROR: linux_task is a NULL pointer");
	    #endif
	    scheduling_error = -1;
	    return RES_FAULT;
	}
	#endif
	stop_linux_task(task->linux_task);
	task->state = TASK_WAITING;
	stopped_task = task;
	stopped_task->stopped_time = jiffies;
	running_task = NULL;
	if(task->parent != NULL)
	{
	    if(division_unsigned(get_time_now() - task->timestamp_mic, 1000) < 0)
	    {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("stop_task", "===**=== ERROR: jiffies - task->timestamp < 0");
		#endif
		scheduling_error = -1;
		return RES_FAULT;
	    }
	    if(task->missing_dl_flag == YES)
	    {
		task->parent->extra_req_budget += division_unsigned(get_time_now() - task->timestamp_mic, 1000);
	    }
	    else
	    {
	      if(division_unsigned(get_time_now() - task->timestamp_mic, 1000) > task->parent->period)
	      {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("stop_task", "consumed budget more than period");
		#endif
// 		scheduling_error = -1;
		return RES_FAULT;
	      }
	      task->parent->consumed_budget += division_unsigned(get_time_now() - task->timestamp_mic, 1000);
// 	      print_long("stop_task", "consumed_budget now", division_unsigned(get_time_now() - task->timestamp_mic, 1000));
// 	      print_time("stop_task", "consumed_budget total", task->parent->consumed_budget);
// 	      print_time_now("stop_task", "now");
	    }
	}
	
	if(task->cbs != NULL)
	{
	    if(task->cbs->current_budget  < division(get_time_now() - task->cbs->timestamp_mic, 1000))
	    {
		#ifdef DEBUG_SCHED_ERRORS
		print_warning("stop_task", "===**=== ERROR: consumed budget is more than available budget!");
		#endif
// 		scheduling_error = -1;
// 		return RES_FAULT;
		task->cbs->current_budget = 0;
		task->cbs->timestamp_mic = get_time_now();
		task->cbs->timestamp = jiffies;
	    }
	    else
	    {
		task->cbs->current_budget = task->cbs->current_budget  - division(get_time_now() - task->cbs->timestamp_mic, 1000);
		task->cbs->timestamp_mic = get_time_now();
		task->cbs->timestamp = jiffies;
		#ifdef JIFFY_TIMER
		remove_timer(&task->cbs->budget_timer);
		#endif
	    }
// 	    remove_timer(&task->cbs->budget_timer);
	}
	return RES_SUCCESS;
}
/*===============================API funcrions===============================================*/
static inline int api_create_server(struct API_Data data)
{
	struct Server *s;
	int i,j;
	s = (struct Server *)kmalloc(sizeof(struct Server), GFP_ATOMIC);
	s->cnt =0;
	s->budget = 0;
	s->period = 0;
	s->current_budget = 0; //complinent with CBS
	s->priority = 99;
	s->dl_miss = 0;
	s->consumed_budget = 0;
	s->total_budget = 0;
	s->extra_req_budget = 0;
	s->control_period = CONTROL_PERIOD;
	s->parent = NULL;
	s->type = data.server_type;
	s->status = SERVER_DEACTIVE;
	s->log = (struct Log_Data *)kmalloc(sizeof(struct Log_Data), GFP_ATOMIC);
	s->log->index = 0;
	s->consumed_budget_history_index = 0;
	s->prev_ANN_input_valid = 0;
	s->total_child_job = 0;
	INIT_LIST_HEAD(&s->children.head);
	if(data.server_type == PERIODIC_SERVER)
	{
	    s->id = get_last_server_id(&server_list.head) + 1;
	    list_add_tail(&s->head, &server_list.head);
	}
	if(data.server_type == CBS)
	{
	    s->id = get_last_server_id(&cbs_list.head) + 1;
	    list_add_tail(&s->head, &cbs_list.head);
	}
	s->ready_queue = (struct Queue *)kmalloc(sizeof(struct Queue), GFP_ATOMIC);
	init_queue(s->ready_queue, data.queue_type);
	#ifdef JIFFY_TIMER
	init_timer(&s->period_timer);
	init_timer(&s->budget_timer);
	#endif
	/*
	 * initializing ANN
	 */
	for(i=0;i<NO_NODS;i++)
	  s->my_network_surp.U[i] = 100/NO_NODS;
	
// 	for(i=0;i<NO_INPUT;i++)
	  for(j=0;j<NO_NODS;j++)
	      s->my_network_surp.W[0][j] = 20;//0/NO_INPUT;
	  
	  for(j=0;j<NO_NODS;j++)
	      s->my_network_surp.W[1][j] = 80;//0/NO_INPUT;

	for(i=0;i<NO_NODS;i++)
	  s->my_network_def.U[i] = 100/NO_NODS;
	
	for(i=0;i<NO_INPUT;i++)
	  for(j=0;j<NO_NODS;j++)
	      s->my_network_def.W[i][j] = 100/NO_INPUT;

	print_warning("api_create_server", "api call!");
	return s->id;
}
static inline int api_set_server_param(struct API_Data data)
{	
	struct Server *s;
	
	if(data.server_type == PERIODIC_SERVER)
	    s = find_server(&server_list.head, data.id);
	else 
	    s = find_server(&cbs_list.head, data.id);
	if(s == NULL)
	{
		print_warning("api_set_server_param", "could not find server");
		return RES_FAULT;
	}
	if(data.period > 0)
		s->period = data.period;
	if(data.budget >= 0)
		s->budget = data.budget;
	if(data.priority >= 0)
		s->priority= data.priority;
	else
		s->priority= -1; 
	if(data.deadline > 0)
		s->relative_deadline = data.deadline;
	else
		s->relative_deadline = s->period; //by defult deadline = period
	print_int("api_set_server_param", "id", s->id);
	print_int("api_set_server_param", "period", s->period);
	print_int("api_set_server_param", "budget", s->budget);
	print_int("api_set_server_param", "priority", s->priority);
	print_warning("api_set_server_param", "api call to set server");
	available_CPU -= s->budget / s->period; 
	return RES_SUCCESS;
}
static inline int api_attach_server_to_server(struct API_Data data)
{	/*attaching id2 to id1*/
	int parent, child;
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
	/* Adding server to children list*/
	c = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	c->id = child;
	c->type = SERVER_TYPE;
	c->server = s_child;
	list_add_tail(&c->head, &s_parent->children.head);
	s_child->parent = s_parent;
	print_int("api_attach_server_to_server", "list cnt =", count_list(&s_parent->children.head));	
	print_warning("api_attach_server_to_server", "api call!");
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
	t->exec_time		= 0;
	t->dl_miss		= 0;
	t->missing_dl_flag	= NO;
	t->parent		= NULL;
	t->linux_task		= NULL;
	t->cbs			= NULL;
	list_add_tail(&t->head, &task_list.head);
	#ifdef JIFFY_TIMER
	init_timer(&t->period_timer);
	#endif
	print_warning("api_create_task", "api call!");
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
		print_warning("api_attach_task_to_server", "server ids should be > 0");
		return RES_FAULT;
	}
	if(data.server_type == PERIODIC_SERVER)
	    s = find_server(&server_list.head, parent);
	else
	    s = find_server(&cbs_list.head, parent);
	if(s == NULL)
	{
		print_warning("api_attach_task_to_server", "could not find parent server");
		return RES_FAULT;
	}
	t = find_task(&task_list.head, child);
	if(t == NULL)
	{
		print_warning("api_attach_task_to_server", "could not find child task");
		return RES_FAULT;
	}
	tmp = find_children(&s->children.head, child, TASK_TYPE);
	if(tmp != NULL)
	{	
		print_int("api_attach_task_to_server", "task is already attached!", tmp->id);
		return RES_FAULT;	
	}
	/* Adding task to children list*/
	c = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	c->id = child;
	c->type = TASK_TYPE;
	c->task = t;
	list_add_tail(&c->head, &s->children.head);
	if(data.server_type == PERIODIC_SERVER)
	    t->parent = s;
	else
	    t->cbs = s;
	print_int("api_attach_task_to_server", "list cnt =", count_list(&s->children.head));	
	print_warning("api_attach_task_to_server", "api call!");
	return RES_SUCCESS;
}
static inline int api_release_server(struct API_Data data)
{
	struct Server *s;
	int res;
	s = find_server(&server_list.head, data.id);
	if(s != NULL)
	    s->status = SERVER_ACTIVE;
	res = release_server(s);
	print_warning("api_release_server", "api call to release server");
	return res;
}
static inline int api_release_task(struct API_Data data)
{
	struct Task *t;
	int res;
	t = find_task(&task_list.head, data.id);
	res = release_hsf_task(t);
	t->state = TASK_READY;
	t->release_time = jiffies + t->period;
	t->abs_deadline = jiffies + t->relative_deadline;
	try_to_run_task(t);
	print_warning("api_release_task", "api call to release task");
	return res;
}
static inline int api_kill_server(struct API_Data data)
{
  struct Server *server;
  struct Queue_element *element;
  int res;
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
  return res;
}

static inline int api_stop(void)
{
    struct Task *t_pos;
    int res;
    res = delete_whole_queue(&Global_Ready_Queue);
    stop_server(active_server);
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
    print_time_now("api_stop", "now");
    print_long("api_stop", "control overhead", control_overhead);
    print_long("api_stop", "total overhead", total_overhead);
    print_warning("api_stop", "api call!");
    return res;
}
static inline int api_run(void)
{
    int res = RES_SUCCESS;
    struct Server *pos;
    struct Task *t_pos;
//     struct timeval t;
    start_time = jiffies;
    do_gettimeofday(&start_time_tv);
//     start_time_milli = t.tv_sec * 1000 + t.tv_usec / 1000;
    list_for_each_entry(t_pos, &task_list.head, head)
    {
	if(t_pos != &task_list)
	{
	      res = release_hsf_task(t_pos);
	      t_pos->state = TASK_READY;
	      t_pos->release_time = jiffies + t_pos->period;
	      t_pos->abs_deadline = jiffies + t_pos->relative_deadline;
	      try_to_run_task(t_pos);
	}
    } 
    
    list_for_each_entry(pos, &server_list.head, head)
    {
	if(pos != &server_list && pos->status == SERVER_DEACTIVE)
	{
	    pos->status = SERVER_ACTIVE;
	    res = release_server(pos);
	}
    }  
    Module_status = MOD_RUNNING;
    #ifdef DEBUG_SCHED_API
    print_warning("api_run", "api call!");
    #endif
    return res;
}
static inline int api_set_task_param(struct API_Data data)
{	
	struct Task *t;
	
	t = find_task(&task_list.head, data.id);
	if(t == NULL)
	{
		print_warning("api_set_task_param", "could not find server");
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
	print_int("api_set_task_param", "id", t->id);
	print_int("api_set_task_param", "period", t->period);
	print_int("api_set_task_param", "exec_time", t->exec_time);
	print_int("api_set_task_param", "priority", t->priority);
	print_warning("api_set_task_param", "api call to set task");
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
	print_warning("api_attach_task_to_mod", "check task faild!");
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
    
    
    change_prio_task = kthread_create((void*)change_prio, (void*)t, "resch-kthread");
    kthread_bind(change_prio_task, CPU_NR);
    print_int("api_attach_task_to_mod", "sched_setscheduler result", sched_setscheduler(change_prio_task, SCHED_FIFO, &sp));
    wake_up_process(change_prio_task);
    //local_irq_enable();
    //change_prio(t, 0);
    
    print_int("api_attach_task_to_mod", "my pid is", t->linux_task->pid);
    t->linux_task->state = TASK_UNINTERRUPTIBLE;
    schedule();
    
    stop_linux_task(current);
    t->state = TASK_WAITING;
    
    print_warning("api_attach_task_to_mod", "api call!");
    return res;
}
static inline int api_detach_task(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Task *t;
    struct Queue_element *e;
    t = find_task(&task_list.head, data.id);
    if(check_task(t) == RES_FAULT)
    {
	print_warning("api_detach_task", "check task faild!");
	return RES_FAULT;
    }
     spin_lock_irq(&mr_lock);
    if(running_task == t)
	running_task = NULL;
    
    if(t->parent == NULL)
    {
	Current_Ready_Queue = &Global_Ready_Queue;
	print_warning("api_detach_task", "Current_Ready_Queue is changed");
	e = search_queue_task(&Global_Ready_Queue, t);
	if(e != NULL)
	    delete_queue(e);
    }
    else
    {
      Current_Ready_Queue = t->parent->ready_queue;
      print_warning("api_detach_task", "Current_Ready_Queue is changed");
      e = search_queue_task(t->parent->ready_queue, t);
	if(e != NULL)
	    delete_queue(e);
    }
    t->state = TASK_STOP;
    t->linux_task = NULL;
    #ifdef JIFFY_TIMER
    remove_timer(&t->period_timer);
    if(t->cbs != NULL)
	remove_timer(&t->cbs->budget_timer);
    #endif
    
    run_queue(Current_Ready_Queue);
    spin_unlock_irq(&mr_lock);
    schedule();
    print_int("api_detach_task", "api call from task", t->id);
    print_time_now("api_detach_task", "detaching task");
    return res;
}
static inline int api_detach_server(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Server *s;
    struct Queue_element *e;
    s = find_server(&server_list.head, data.id);
    if(check_server(s) == RES_FAULT)
    {
	print_warning("api_detach_server", "check server faild!");
	return RES_FAULT;
    }
    if(active_server == s)
	stop_server(active_server);
    if(s->parent == NULL)
    {
	Current_Ready_Queue = &Global_Ready_Queue;
	print_warning("api_detach_server", "Current_Ready_Queue is changed");
	e = search_queue_server(&Global_Ready_Queue, s);
	if(e != NULL)
	    delete_queue(e);
    }
    else
    {
      Current_Ready_Queue = s->parent->ready_queue;
      print_warning("api_detach_server", "Current_Ready_Queue is changed");
      e = search_queue_server(s->parent->ready_queue, s);
	if(e != NULL)
	    delete_queue(e);
    }
    s->status = SERVER_DEACTIVE;
    //destroy_timer_on_stack(&s->period_timer);
    //destroy_timer_on_stack(&s->budget_timer);
    #ifdef JIFFY_TIMER
    remove_timer(&s->period_timer);
    remove_timer(&s->budget_timer);
    #endif
    run_queue(Current_Ready_Queue);
    print_warning("api_detach_server", "api call!");
    return res;
}
static inline int api_task_finish_job(struct API_Data data)
{
    int res = RES_SUCCESS;
    struct Task *t;
    struct Handler_Data *data_period;
//     unsigned long start_time = jiffies;
    struct Queue_element *pos = NULL;
    struct timespec tmp_time_start, tmp_time_end, overhead;
    getnstimeofday(&tmp_time_start);
    
    if(test_mod)
      return -1;
    t = find_task(&task_list.head, data.id);
    #ifdef DEBUG_CHECKS
    if(check_task(t) == RES_FAULT)
    {
	#ifdef DEBUG_SCHED_ERRORS
	print_warning("api_task_finish_job", "===**=== ERROR: check task faild!");
	#endif
	scheduling_error = -1;
	return RES_FAULT;
    }
    #endif
//     print_int_int("api_task_finish_job", "job number, id", t->cnt, t->id);
//     print_time_now("api_task_finish_job", "job finish");
    #ifdef DEBUG_API_FINISH_JOB
    print_int_int("api_task_finish_job", "job number, id", t->cnt, t->id);
    print_int("api_task_finish_job", "deadline miss", t->release_time - jiffies);
    print_time("api_task_finish_job", "jiffies", jiffies);
    #endif
    spin_lock_irq(&mr_lock);
    
    
    if(t->parent == NULL)
	  Current_Ready_Queue = &Global_Ready_Queue;
    else
	  Current_Ready_Queue = t->parent->ready_queue;
//     print_warning("api_task_finish_job", "Current_Ready_Queue is changed");
    /*
     * task might be in the ready_queue if it is at the same time with server_budget_handler
     * So we should remove it
    */
    
    pos = search_queue_task(Current_Ready_Queue, t);
    if(pos != NULL)
    {
	delete_queue(pos);
    }
//     if(t->state == TASK_RUNNING_HSF)
    {
// 	  t->state = TASK_WAITING;
// 	  print_int_int("api_task_finish_job", "task job finish", t->id, t->cnt);
	  stop_task(t);
    }
//     else
//       print_int_int("api_task_finish_job", "task job finish: not stopping", t->id, t->cnt);
    /*Insert timer for the next period*/
    data_period = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
    data_period->id = t->id;
    data_period->handler_type = TASK_RELEASE_HANDLER;
    data_period->task = t;
    
//     printk(KERN_WARNING "tardiness_task%d: %lu %lu\n", t->id, t->release_time - jiffies, jiffies-start_time);
    if(t->release_time >= jiffies)
    {	
	      #ifdef JIFFY_TIMER
// 	      print_warning("api_task_finish_job", "inserting timer");
	      insert_timer(&t->period_timer, handler_task_release, (unsigned long) data_period, t->release_time);
	      #endif
    }
    else
    {
// 	      print_int("api_task_finish_job", "inserting task to queue", t->id);
	      
	      if(t->timestamp <= t->release_time)
	      {
// 		  print_warning("api_task_finish_job", "===task startet before the deadline and finished after the deadline===");
		  t->dl_miss_amount += t->release_time - jiffies;  
		  if(t->parent != NULL && t->missing_dl_flag == NO)//because if it is yes stop_task has observed it
		      t->parent->extra_req_budget += (t->release_time - jiffies);//+1;// /(t->period * t->parent->period);  
	      }
	      release_hsf_task(t);
	      t->state = TASK_READY;
	      t->abs_deadline = t->release_time + t->relative_deadline;
	      t->release_time += t->period;
	      t->dl_miss++;
	      t->cnt++;
	      if(t->parent != NULL)
	      {
		  t->parent->dl_miss++;
		  t->parent->total_child_job++;
	      }
    }
    if(t->cbs != NULL)
    {;
	#ifdef JIFFY_TIMER
	remove_timer(&t->cbs->budget_timer);
	#endif
    }
//     print_warning("api_task_finish_job", "calling run_queue");
    run_queue(Current_Ready_Queue);
    //print_int("api_task_finish_job", "CPU",smp_processor_id());
//     print_time("api_task_finish_job", "now",jiffies);
    #ifdef DEBUG_SCHED_API
    print_warning("api_task_finish_job", "api call!");
    #endif
    //**************************************************		
    
    getnstimeofday(&tmp_time_end);
    overhead = timespec_sub(tmp_time_end, tmp_time_start);
    total_overhead += overhead.tv_nsec + overhead.tv_sec*1000000000;
    spin_unlock_irq(&mr_lock);
    schedule();
    
    return res;
}

#ifdef TEST_TIMER
static inline int api_test_timer(struct API_Data data)
{
  struct Handler_Data *data_handler1;
  int res = RES_SUCCESS;
  unsigned long n;
  print_time("", "now", get_time_now());
  n=get_time_now();
  n-= 45;
  print_time("", "overhead", division(get_time_now()-n, 1000));
  
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
  
  my_tmp_cnt = 0;
  data_handler1 = (struct Handler_Data *)kmalloc(sizeof(struct Handler_Data), GFP_ATOMIC);
  data_handler1->id = 1;
  
//     list_for_each_entry(t_pos, &task_list.head, head)
//     {
// 	if(t_pos != &task_list)
// 	{
// 	      data_handler1->task = t_pos;
// 	}
//     } 
  #ifdef JIFFY_TIMER
  init_timer(&timer1);
  start_time = jiffies;
  do_gettimeofday(&start_time_tv);
  print_time_now("api", "now");
  insert_timer(&timer1, handler, (unsigned long) data_handler1, jiffies + 1);
  #endif
  //insert_timer(&timer2, handler, (unsigned long) data_handler2, jiffies + 20);
 
  /*
  destroy_timer_on_stack(&timer1);
  setup_timer_on_stack(&timer1, handler, (unsigned long)data_handler1);
  mod_timer(&timer1, jiffies + 10);
  
  destroy_timer_on_stack(&timer2);
  setup_timer_on_stack(&timer2, handler, (unsigned long)data_handler2);
  mod_timer(&timer2, jiffies + 10);
  */
  print_time("api_test_timer", "seting timer", jiffies);
  print_warning("api_test_timer", "API call to test timer");
  return res;
}
#endif
static inline int api_print_log(struct API_Data data)
{
    int i=0;
    long sum_extra_req_budget, sum_budget;
    struct Server *server;
    server = find_server(&server_list.head, data.id);
    if(check_server(server) == RES_FAULT)
    {
	print_warning("api_kill_server", "check server failed");
	return RES_FAULT;
    }
    if(server->log == NULL)
    {
	print_warning("api_kill_server", "server log is NULL");
	return RES_FAULT;
    }
    sum_extra_req_budget = 0;
    sum_budget = 0;
    for(i=0;i<server->log->index;i++)
    {
	  printk(KERN_WARNING "Nima-HSF LOG <%d, %d>: server:%d budget: %ld avg_consumed_budget:%ld consumed_budget: %ld extra_budget: %ld dl_miss:%d at %ld.\n", i, server->log->cnt[i], data.id, server->log->budget[i], server->log->avg_consumed_budget[i], server->log->consumed_budget[i], server->log->extra_req_budget[i], server->log->dl_miss[i], server->log->time[i]);
// 	  printk(KERN_WARNING "%lu\n", server->log->consumed_budget[i]);
	  sum_extra_req_budget += server->log->extra_req_budget[i];
	  sum_budget +=server->log->budget[i];
    }
    
    print_int("api_print_log", "index", server->log->index);
    print_int("api_print_log", "dl_miss", server->dl_miss);
    print_int("api_print_log", "total job", server->total_child_job);
    print_long("api_print_log", "sum_extra_req_budget", sum_extra_req_budget);
    print_long("api_print_log", "avg_utilization", division(sum_budget*10000, i*server->period));
    print_long("api_print_log", "dl_miss_ratio", division(server->dl_miss*10000, server->total_child_job));
    return RES_SUCCESS;
}
static inline int api_create_app(struct API_Data data)
{
	struct Application *app;
	app = (struct Application *)kmalloc(sizeof(struct Application), GFP_ATOMIC);
	app->budget = 0;
	app->period = 0;
	app->bandwidth = 0;
	INIT_LIST_HEAD(&app->servers.head);
	INIT_LIST_HEAD(&app->tasks.head);
	app->ready_queue = (struct Queue *)kmalloc(sizeof(struct Queue), GFP_ATOMIC);
	init_queue(app->ready_queue, data.queue_type);
	app->id = get_last_app_id(&app_list.head) + 1;
	list_add_tail(&app->head, &app_list.head);	
	print_warning("api_create_app", "api call!");
	print_int("api_create_app", "cnt app", count_list(&app_list.head));
	return app->id;
}

static inline int api_attach_server_to_app(struct API_Data data)
{	//attaching id2 to id1
	int app_id, server_id;
	struct Application *app_parent;
	struct Server *server;
	Children *child_server, *tmp_children;
	print_warning("api_attach_server_to_app", "api start");
	server_id = data.id;
	app_id  = data.id2;
	app_parent = find_app(&app_list.head, app_id);
	if(app_parent == NULL)
	{
		print_warning("api_attach_server_to_app", "could not find parent app");
		return RES_FAULT;
	}
	server = find_server(&server_list.head, server_id);
	if(server == NULL)
	{
		print_warning("api_attach_server_to_app", "could not find child server");
		return RES_FAULT;
	}
	tmp_children = find_children(&app_parent->servers.head, server_id, SERVER_TYPE);
	if(tmp_children != NULL)
	{	
		print_int("api_attach_server_to_app", "server is already attached!", tmp_children->id);
		return RES_FAULT;	
	}
	 /*Adding server to children list*/
	child_server = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	child_server->id = server_id;
	child_server->type = SERVER_TYPE;
	child_server->server = server;
	list_add_tail(&child_server->head, &app_parent->servers.head);
	server->parent_app = app_parent;
	print_int("api_attach_server_to_app", "list cnt =", count_list(&app_parent->servers.head));	
	print_warning("api_attach_server_to_app", "end of api call!");
	return RES_SUCCESS;
}

static inline int api_attach_task_to_app(struct API_Data data)
{	//attaching id2 to id1
	int app_id, task_id;
	struct Application *app_parent;
	struct Task *task;
	Children *child_task, *tmp_children;
	print_warning("api_attach_task_to_app", "api start");
	task_id = data.id;
	app_id  = data.id2;
	app_parent = find_app(&app_list.head, app_id);
	if(app_parent == NULL)
	{
		print_warning("api_attach_task_to_app", "could not find parent app");
		return RES_FAULT;
	}
	task = find_task(&task_list.head, task_id);
	if(task == NULL)
	{
		print_warning("api_attach_task_to_app", "could not find the child task");
		return RES_FAULT;
	}
	tmp_children = find_children(&app_parent->tasks.head, task_id, TASK_TYPE);
	if(tmp_children != NULL)
	{	
		print_int("api_attach_task_to_app", "task is already attached!", tmp_children->id);
		return RES_FAULT;	
	}
	 /*Adding server to children list*/
	child_task = (Children *)kmalloc(sizeof(Children), GFP_ATOMIC);
	child_task->id = task_id;
	child_task->type = TASK_TYPE;
	child_task->task = task;
	list_add_tail(&child_task->head, &app_parent->tasks.head);
	task->parent_app = app_parent;
	print_int("api_attach_task_to_app", "list cnt =", count_list(&app_parent->tasks.head));	
	print_warning("api_attach_task_to_app", "end of api call!");
	return RES_SUCCESS;
}
