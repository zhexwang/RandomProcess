
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

int main(int argc, const char *argv[])
{
	//1.judge illegal
	if(argc<1){
		ERR("Usage: ./%s logFile", argv[0]);
		abort();
	}
	//2.read log file and create code_segment_vec
	ReadLog log(argv[1]);
	//3.init code cache
	global_code_cache = init_code_cache();
	//4.read elf to find function
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
	//5.dump
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CSfunctionMapOriginList.begin(); it!=CSfunctionMapOriginList.end(); it++){
		if(!it->first->isSO){
			for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
				iter->second->disassemble();
				iter->second->dump_function_origin();
			}
		}
	}

	return 0;
}
