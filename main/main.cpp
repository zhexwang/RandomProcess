
#include "readlog.h"
#include "utility.h"
#include "code_segment_management.h"
#include "function.h"
#include "readelf.h"
#include "map_function.h"
#include <iostream>
using namespace std;

CodeCache *global_code_cache = NULL;
CODE_SEG_MAP_ORIGIN_FUNCTION CSfunctionMapOriginList;
CODE_SEG_MAP_FUNCTION CSfunctionMapList;

void readelf_to_find_all_functions()
{
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it<code_segment_vec.end(); it++){
		if(!(*it)->is_code_cache){
			//map origin
			MAP_ORIGIN_FUNCTION *map_origin_function = new MAP_ORIGIN_FUNCTION();
			CSfunctionMapOriginList.insert(CODE_SEG_MAP_ORIGIN_FUNCTION_PAIR(*it, map_origin_function));
			//map current 
			MAP_FUNCTION *map_function = new MAP_FUNCTION();
			CSfunctionMapList.insert(CODE_SEG_MAP_FUNCTION_PAIR(*it, map_function));
			//read elf
			ReadElf *read_elf = new ReadElf(*it);
			read_elf->scan_and_record_function(map_function, map_origin_function);
			delete read_elf;
		}
	}
	return ;
}

void random_all_functions()
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				CODE_CACHE_ADDR curr_cc_ptr = 0;
				ORIGIN_ADDR origin_cc_ptr = 0;
				global_code_cache->getCCCurrent(curr_cc_ptr, origin_cc_ptr);
				SIZE size = iter->second->random_function(curr_cc_ptr, origin_cc_ptr);
				global_code_cache->updateCC(size);
				
				//iter->second->dump_function_origin();
				iter->second->dump_bb_origin();
				//global_code_cache->disassemble("", curr_cc_ptr, curr_cc_ptr+size);				
			}
		}
	}
	return ;
}

int main(int argc, const char *argv[])
{
	// 1.judge illegal
	if(argc!=3){
		ERR("Usage: ./%s shareCodeLogFile indirectProfileLogFile\n", argv[0]);
		abort();
	}
	// 2.read share log file and create code_segment_vec
	ReadLog log(argv[1], argv[2]);
	// 3.init log
	log.init_share_log();
	log.init_profile_log();//read indirect inst and target
	// 4.init code cache
	global_code_cache = init_code_cache();
	//5.read elf to find function
	readelf_to_find_all_functions();
	//6.random
	random_all_functions();
	return 0;
}
