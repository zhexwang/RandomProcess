#include "readlog.h"
#include "utility.h"
#include "code_segment_management.h"
#include "function.h"
#include "readelf.h"
#include "map_function.h"
#include "map_inst.h"
#include "codecache.h"
#include "stack.h"
#include "communication.h"
#include <iostream>
#include <unistd.h>
using namespace std;

CodeCacheManagement *cc_management = NULL;
MapInst *map_inst_info = NULL;
ShareStack *main_share_stack = NULL;
ShareStack *child_share_stack[THREAD_MAX_NUM];
Communication *communication = NULL;
CODE_SEG_MAP_ORIGIN_FUNCTION CSfunctionMapOriginList;

extern void split_function_from_target_branch();
extern BOOL need_omit_random_function(string function_name);
extern BOOL need_omit_stack_function(string function_name);


void readelf_to_find_all_functions()
{
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it<code_segment_vec.end(); it++){
		if(!(*it)->is_code_cache && !(*it)->is_stack){
			//map origin
			MAP_ORIGIN_FUNCTION *map_origin_function = new MAP_ORIGIN_FUNCTION();
			CSfunctionMapOriginList.insert(CODE_SEG_MAP_ORIGIN_FUNCTION_PAIR(*it, map_origin_function));
			//read elf
			ReadElf *read_elf = new ReadElf(*it);
			CodeCache *code_cache = (*it)->code_cache;
			read_elf->scan_and_record_function(map_origin_function, code_cache);
			delete read_elf;
		}
	}
	return ;
}

extern void read_syscall_inst_stack_type(CodeSegment *cs);
extern void read_unused_rbp_func_return_addr();

void analysis_all_functions_stack()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			string path = it->first->file_path;
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				func->analysis_stack_v2();
			}
		}else{//we only handle syscall instruction
			read_syscall_inst_stack_type(it->first);
		}
	}
	//record some function do not use rbp
	read_unused_rbp_func_return_addr();
	return ;
}

void random_all_functions()
{
	INT32 idx = 1;
	INT32 num = CSfunctionMapOriginList.size();
	progress_begin();
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		string path = it->first->file_path;
		if(path.find("/lib/ld-2.17.so")==string::npos){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				if(!need_omit_random_function(func->get_function_name()))
					func->random_function(map_inst_info->get_curr_mapping_oc(), map_inst_info->get_curr_mapping_co());
			}
		}
		print_progress(idx, num);
		idx++;
	}
	return ;
}

void erase_and_intercept_all_functions()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		CodeCache *code_cache = it->first->code_cache;
		code_cache->erase_old_cc();
		string path = it->first->file_path;
		if(path.find("lib/ld-2.17.so")==string::npos){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				if(!need_omit_random_function(func->get_function_name())){
					func->erase_function();	
					func->intercept_to_random_function();
				}
			}
		}
	}
	return ;
}

void flush()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
			Function *func = iter->second;
			func->flush_function_cc();
		}
	}
	cc_management->flush();
	return ;
}

Function *find_function_by_origin_cc(ORIGIN_ADDR addr)
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
			Function *func = iter->second;
			if(func->is_in_function_cc(addr))
				return func;
		}
	}
	return NULL;
}

Function *find_function_by_addr(ORIGIN_ADDR addr)
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
			Function *func = iter->second;
			if(func->is_in_function(addr))
				return func;
		}
	}
	return NULL;
}

BOOL relocate_retaddr_and_pc()
{
	ASSERT(main_share_stack);
	BOOL can_be_random = main_share_stack->check_relocate(map_inst_info);	
	if(!can_be_random)
		return false;
	for(INT32 idx=0; idx<THREAD_MAX_NUM; idx++){
		can_be_random = child_share_stack[idx]->check_relocate(map_inst_info);
		if(!can_be_random)
			return false;
		
	}
	
	main_share_stack->relocate_return_address(map_inst_info);
	main_share_stack->relocate_current_pc(map_inst_info);	
	for(INT32 idx=0; idx<THREAD_MAX_NUM; idx++){
		child_share_stack[idx]->relocate_return_address(map_inst_info);
		child_share_stack[idx]->relocate_current_pc(map_inst_info);
	}
	return true;
}

void wait_seconds_to_continue_random(INT32 seconds)
{
	while(seconds>0){
		YELLOW("	 <<%3d seconds left to rerandom>>", seconds);
		sleep(1);
		seconds--;
		for(INT32 idx = 0; idx<37; idx++)
			YELLOW("\b");
	}
	YELLOW("	 <<Begin to rerandom the process>>\n");

}
INT32 random_times = 1;

int main(int argc, const char *argv[])
{
	// 1.judge illegal
	if(argc!=3){
		ERR("Usage: ./%s shareCodeLogFile indirectProfileLogFile\n", argv[0]);
		abort();
	}
	// 2.read share log file and create code_segment_vec share stack
	ReadLog log(argv[1], argv[2]);
	// 3.init log and map info
	communication = new Communication();
	log.init_share_log();
	BLUE("[ 1] Finish sharing Code/CodeCache/Stack segment\n");
	log.init_profile_log();//read indirect inst and target
	BLUE("[ 2] Finish reading indirect profile information\n");
	map_inst_info = new MapInst();
	//dump_code_segment();
	// 4.read elf to find function
	readelf_to_find_all_functions();
	// 4.1 handle so function internal
	split_function_from_target_branch();
	BLUE("[ 3] Finish scanning all superblock and disassembling\n");
	// 5.find all stack map
	analysis_all_functions_stack();
	BLUE("[ 4] Finish analysis function stack\n");
	// loop for random
	BOOL continue_to_run = true;
	while(1){
		// 5.flush
		BLUE("[ 5] Iterate to random all function: %d times\n", random_times);
		INFO("[ 5] <1> Flush code cache\n");
		flush();
		map_inst_info->flush();
		// 6.random
		INFO("[ 5] <2> Begin to random: ");
		random_all_functions();
		INFO(" Finish random\n");
restop:		
		// 7.send signal to stop the process
		INFO("[ 5] <3> Send signal to stop working process\n");
		continue_to_run = communication->stop_process();
		if(!continue_to_run)
			break;
		
		INFO("[ 5] <4> State migration: \n");
		// 10.relocate
		PRINT("         1. Begin to relocate return address of stack and current pc\n");
		BOOL success = relocate_retaddr_and_pc();
		if(!success){
			ERR("            Fail to relocate return address of stack. Sleep serveral seconds and restop the process\n");
			INFO("[ 5] <5> Send signal to continue working process\n");
			communication->continue_process();
			sleep(1);
			goto restop;
		}
		// 9.erase and intercept
		erase_and_intercept_all_functions();
		PRINT("         2. Finish redirecting the superblock entry and erasing old code\n");
		
		// 11.continue to run
		INFO("[ 5] <5> Wake up and continue the working process\n");
		continue_to_run = communication->continue_process();
		if(!continue_to_run)
			break;
		
		random_times++;
		// 12.random interval
		wait_seconds_to_continue_random(2);
	}
	BLUE("[ 6] Live Rerandomization is finished. Thanks for using, bye!\n");
	return 0;
}
