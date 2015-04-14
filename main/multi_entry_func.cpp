#include "code_segment_management.h"
#include "function.h"
#include "type.h"
#include "utility.h"

extern CODE_SEG_MAP_ORIGIN_FUNCTION CSfunctionMapOriginList;

static CodeSegment *find_cs(ORIGIN_ADDR addr)
{
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it!=code_segment_vec.end(); it++){
		CodeSegment *cs = *it;
		if(!cs->is_stack && !cs->is_code_cache){
			if(cs->is_in_cs(addr))
				return cs;
		}
	}
	return NULL;
}

void split_function_from_target_branch()
{
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it!=code_segment_vec.end(); it++){
		if(!(*it)->is_stack && !(*it)->is_code_cache){
			//find all indirect target
			for(CodeSegment::INDIRECT_MAP_ITERATOR indirect_iter = (*it)->indirect_inst_map.begin(); \
				indirect_iter!=(*it)->indirect_inst_map.end(); indirect_iter++){
				ORIGIN_ADDR inst_addr = indirect_iter->first;
				ORIGIN_ADDR target_addr = indirect_iter->second;
				CodeSegment *target_cs = find_cs(target_addr);

				ASSERT(target_cs);
				if(target_cs!=*it){
					MAP_ORIGIN_FUNCTION  *target_func_map = CSfunctionMapOriginList.find(target_cs)->second;
					MAP_ORIGIN_FUNCTION_ITERATOR target_func_iter_ret = target_func_map->find(target_addr);
					if(target_func_iter_ret==target_func_map->end()){
						for(MAP_ORIGIN_FUNCTION_ITERATOR target_func_iter = target_func_map->begin(); \
							target_func_iter!=target_func_map->end(); target_func_iter++){
							Function *func = target_func_iter->second;
							if(func->is_in_function(target_addr)){
								//need add new entry for func
								func->add_function_entry(target_addr);
								//ASSERT(0);
							}
						}
					}
				}else{
					MAP_ORIGIN_FUNCTION  *func_map = CSfunctionMapOriginList.find(*it)->second;
					if(func_map->find(target_addr)==func_map->end()){
						Function *inst_func = NULL;
						Function *target_func = NULL;
						for(MAP_ORIGIN_FUNCTION_ITERATOR func_iter = func_map->begin(); func_iter!=func_map->end(); func_iter++){
							Function *func = func_iter->second;
							if(func->is_in_function(inst_addr))
								inst_func = func;

							if(func->is_in_function(target_addr))
								target_func = func;

							if(target_func){
								if(target_func!=inst_func){
									//need add new entry for target_func
									target_func->add_function_entry(target_addr);
									//ASSERT(0);
								}
								break;
							}
						}
					}
				}
			}
			//find all direct target
			for(vector<ORIGIN_ADDR>::iterator entry_iter = (*it)->direct_profile_func_entry.begin(); 
				entry_iter!=(*it)->direct_profile_func_entry.end(); entry_iter++){
				ORIGIN_ADDR entry_addr = *entry_iter;
				ASSERT((*it)->is_in_cs(entry_addr));
				MAP_ORIGIN_FUNCTION  *func_map = CSfunctionMapOriginList.find(*it)->second;
				if(func_map->find(entry_addr)==func_map->end()){
					for(MAP_ORIGIN_FUNCTION_ITERATOR func_iter = func_map->begin(); func_iter!=func_map->end(); func_iter++){
						Function *func = func_iter->second;
						if(func->is_in_function(entry_addr)){
							func->add_function_entry(entry_addr);
							break;
						}
					}
				}
			}
		}
	}

}

