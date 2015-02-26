#include "map_function.h"
#include "function.h"

void dump_function_origin_list(CODE_SEG_MAP_ORIGIN_FUNCTION *CS_origin_function_map)
{
	for(CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR it = CS_origin_function_map->begin(); it!=CS_origin_function_map->end(); it++){
		INFO("==================CODE SEGMENT: %s====================\n", it->first->file_path.c_str());
		for(MAP_ORIGIN_FUNCTION_ITERATOR iter = it->second->begin(); iter!=it->second->end(); iter++){
			iter->second->dump_function_origin();
		}
		INFO("===========================END================================\n");
	}
}
