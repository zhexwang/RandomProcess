#include "communication.h"
#include "utility.h"
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <errno.h>

Communication::Communication()
{
	lock = new SpinLock();
}

int tkill(int tid, int sig)
{
	return syscall(SYS_tkill, tid, sig);
}

BOOL send_signal(COMMUNICATION_INFO *info)
{
	// 1.init flag
	info->flag = 0;
	// 2.send message to stop the main process
	if(info->process_id==0)
		return false;

	INT32 ret = tkill(info->process_id, SIGUSR1);
	if(ret==-1){
		if(errno==EINVAL || errno==ESRCH)
			ASSERT(info->process_id==0);
		else
			ASSERT(0);
	}
	
	// 3.wait main process has already stop
	while(info->flag!=1){
		if(info->process_id==0)
			return false;
		
		sched_yield();
	}
	//INFO("[%4d] stopped\n", info->process_id);
	return true;
}

BOOL Communication::stop_process()
{
	lock->lock();
	ASSERT(main_thread_info);
	//stop main thread 
	BOOL success = send_signal(main_thread_info);
	if(!success){
		lock->unlock();
		return false;
	}
	//stop child 
	for(vector<COMMUNICATION_INFO *>::iterator iter = child_thread_info.begin(); iter!=child_thread_info.end(); iter++){
		send_signal(*iter);
	}
	
	lock->unlock();
	return true;
}

BOOL set_flag(COMMUNICATION_INFO *info)
{
	if(info->process_id==0)
		return false;

	//INFO("continue %d\n", info->process_id);

	ASSERT(info->flag==1);
	info->flag = 0;
	while(info->flag!=1){
		if(info->process_id==0)
			return false;
			
		sched_yield();
	}
	return true;
}

BOOL Communication::continue_process()
{
	lock->lock();
	ASSERT(main_thread_info->process_id!=0);
	//main thread
	BOOL success = set_flag(main_thread_info);
	if(!success){
		lock->unlock();
		return false;
	}
	//child
	for(vector<COMMUNICATION_INFO *>::iterator iter = child_thread_info.begin(); iter!=child_thread_info.end(); iter++){
		set_flag(*iter);
	}
	lock->unlock();
	return true;
}
