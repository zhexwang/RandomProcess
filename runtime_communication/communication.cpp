#include "communication.h"
#include "utility.h"
#include <sched.h>

Communication::Communication()
{
	lock = new SpinLock();
}

BOOL Communication::stop_process()
{
	lock->lock();
	// 1.clear communication flag
	ASSERT(main_thread_info);
	main_thread_info->flag = 0;
	//TODO: child thread's communication flag need be set
	// 2.send message to stop the process
	//TODO: multi-threads need use pthread_kill
	if(main_thread_info->process_id==0)
		return false;
	
	kill(main_thread_info->process_id, SIGUSR1);
	// 3.wait process has already stop
	while(main_thread_info->flag!=1){
		if(main_thread_info->process_id==0){
			lock->unlock();
			return false;
		}
			
		sched_yield();
	}
	lock->unlock();
	return true;
}

BOOL Communication::continue_process()
{
	lock->lock();
	//TODO: multi-threads 
	ASSERT(main_thread_info->flag==1);
	main_thread_info->flag = 0;
	while(main_thread_info->flag!=1){
		if(main_thread_info->process_id==0){
			lock->unlock();
			return false;
		}
			
		sched_yield();
	}
	lock->unlock();
	return true;
}
