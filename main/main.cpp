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
using namespace std;

CodeCacheManagement *cc_management = NULL;
MapInst *map_inst_info = NULL;
ShareStack *main_share_stack = NULL;
Communication *communication = NULL;
CODE_SEG_MAP_ORIGIN_FUNCTION CSfunctionMapOriginList;
CODE_SEG_MAP_FUNCTION CSfunctionMapList;

void readelf_to_find_all_functions()
{
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it<code_segment_vec.end(); it++){
		if(!(*it)->is_code_cache && !(*it)->is_stack){
			//map origin
			MAP_ORIGIN_FUNCTION *map_origin_function = new MAP_ORIGIN_FUNCTION();
			CSfunctionMapOriginList.insert(CODE_SEG_MAP_ORIGIN_FUNCTION_PAIR(*it, map_origin_function));
			//map current 
			MAP_FUNCTION *map_function = new MAP_FUNCTION();
			CSfunctionMapList.insert(CODE_SEG_MAP_FUNCTION_PAIR(*it, map_function));
			//read elf
			ReadElf *read_elf = new ReadElf(*it);
			CodeCache *code_cache = (*it)->code_cache;
			read_elf->scan_and_record_function(map_function, map_origin_function, code_cache);
			delete read_elf;
		}
	}
	return ;
}

void analysis_all_functions_stack()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				func->analysis_stack(it->second);
				func->get_stack_map(ShareStack::stack_map);
				//func->dump_function_origin();
			}
		}
	}
	return ;
}

void random_all_functions()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				//if(func->get_function_name() == "main"){
					func->random_function(it->second);
					func->get_map_origin_cc_info(map_inst_info->get_curr_mapping_oc(), map_inst_info->get_curr_mapping_co());
				//}
			}
		}
	}
	return ;
}

void intercept_all_functions()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				//if(func->get_function_name() == "main")
					func->intercept_to_random_function();
			}
		}
	}
	return ;
}

void erase_all_functions()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				Function *func = iter->second;
				//if(func->get_function_name() == "main")
					func->erase_function();			
			}
		}
	}
	return ;
}

void flush()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		CodeCache *code_cache = it->first->code_cache;
		code_cache->flush();
		for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
			Function *func = iter->second;
			func->flush_function_cc();
		}
	}
	return ;
}

void relocate_retaddr_and_pc()
{//do not concern the multi-thread
	ASSERT(main_share_stack);
	main_share_stack->relocate_return_address(map_inst_info);		
	main_share_stack->relocate_current_pc(map_inst_info);	
//TODO::handle multi-thread stack
}

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
	log.init_profile_log();//read indirect inst and target
	map_inst_info = new MapInst();
	//dump_code_segment();
	// 4.read elf to find function
	readelf_to_find_all_functions();
	// 5.find all stack map
	analysis_all_functions_stack();
	// loop for random
	//while(1){
		// 5.flush
		flush();
		map_inst_info->flush();
		// 6.random
		random_all_functions();
		// 7.send signal to stop the process
		//communication->stop_process();
		// 8.intercept
		intercept_all_functions();
		// 9.erase
		erase_all_functions();
		// 10.relocate
		//relocate_retaddr_and_pc();
		// 11.continue to run
		//communication->continue_process();
	//}
	return 0;
}
