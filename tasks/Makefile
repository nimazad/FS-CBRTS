lib='library/libhsf.a'

all: 
	@gcc -o rt_task1 rt_task1.c $(lib)
	@gcc -o video_decoder video_decoder.c $(lib)
	@gcc -o idle_task idle_task.c $(lib)
	@gcc -o create_server create_server.c $(lib)
	@gcc -o attach_server attach_server.c $(lib)
	@gcc -o set_server_param set_server_param.c $(lib)
	@gcc -o create_task create_task.c $(lib)
	@gcc -o attach_task attach_task.c $(lib)	
	@gcc -o release_server release_server.c $(lib)
	@gcc -o kill_server kill_server.c $(lib)
	@gcc -o stop stop.c $(lib)
	@gcc -o run run.c $(lib)
	@gcc -o release_task release_task.c $(lib)
	@gcc -o set_task_param set_task_param.c $(lib)
	@gcc -o test_timer test_timer.c $(lib)
	@gcc -o print_log print_log.c $(lib)
	#@gcc  -o ColorSlider ColorSlider.c $(lib) `pkg-config --cflags --libs opencv` -I/usr/local/include
	@gcc  -o rt_edge_detector rt_edge_detector.c $(lib) `pkg-config --cflags --libs opencv` -I/usr/local/include  
	@gcc -o create_app create_app.c $(lib)
	@gcc -o attach_server_to_app attach_server_to_app.c $(lib)
	@gcc -o attach_task_to_app attach_task_to_app.c $(lib)
	@gcc -o intra_app_sched_event intra_app_sched_event.c $(lib)
	@gcc -o get_ctrl_data get_ctrl_data.c $(lib)
	@gcc -o set_app_param set_app_param.c $(lib)
	@gcc -o  lqr_ctrl lqr_ctrl.c $(lib)
	@gcc -o  manager manager.c $(lib)
	
clean:
	rm -f  rt_task1 idle_task create_server attach_server create_task attach_task release_server set_server_param kill_server stop run release_task set_task_param test_timer ColorSlider print_log *~ 
