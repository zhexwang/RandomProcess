#include "map_inst.h"
#include "function.h"

ORIGIN_ADDR MapInst::get_curr_addr_by_origin(ORIGIN_ADDR addr)
{
	MAP_CO_ITERATOR ret = map_cc_to_origin[curr_idx].find(addr);
	return ret==map_cc_to_origin[curr_idx].end() ? 0 : ret->second;
}

extern INT32 random_times;
extern Function *find_function_by_addr(ORIGIN_ADDR addr);

ORIGIN_ADDR MapInst::get_new_addr_from_old(ORIGIN_ADDR old_inst_addr, BOOL is_in_cc)
{
	if(is_in_cc){
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		
		MAP_CO_ITERATOR old_iter = map_cc_to_origin[old_idx].find(old_inst_addr);
		ASSERT(old_iter!=map_cc_to_origin[old_idx].end());
		ORIGIN_ADDR origin_inst_addr = old_iter->second;
		
		//find old index
		MAP_OC_PAIR old_range = map_origin_to_cc[old_idx].equal_range(origin_inst_addr);
		ASSERT((old_range.first)!=(old_range.second));
		INT32 old_count = map_origin_to_cc[old_idx].count(origin_inst_addr);
		INT32 target_idx = 0;
		for(MAP_OC_ITERATOR it = old_range.first; it!=old_range.second; it++, target_idx++){
			if((it->second)==old_inst_addr)
				break;
		}

		MAP_OC_PAIR new_range = map_origin_to_cc[curr_idx].equal_range(origin_inst_addr);
		ASSERT((new_range.first)!=(new_range.second));
		INT32 new_count = map_origin_to_cc[curr_idx].count(origin_inst_addr);
		ASSERT(new_count==old_count);
		INT32 idx = 0;
		MAP_OC_ITERATOR ret_iter = new_range.first;
		for(; ret_iter!=new_range.second; ret_iter++, idx++)
			if(idx==target_idx)
				break;
		ASSERT(idx==target_idx);
		
		return ret_iter->second;
	}else{
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		
		INT32 count_num = map_origin_to_cc[old_idx].count(old_inst_addr);
		if(random_times==1)
			ASSERT(count_num==0);//old map should not has this instruction
		else{
			if(count_num!=0){//intercept jmp inst
				Function * func = find_function_by_addr(old_inst_addr);
				ASSERT(func);
				ASSERT(func->is_entry(old_inst_addr));
				return old_inst_addr;
			}
		}

		MAP_OC_PAIR new_range = map_origin_to_cc[curr_idx].equal_range(old_inst_addr);
		if(new_range.first==new_range.second){
			//do not random this instruction
			return old_inst_addr;
		}else{
			//first cc instruction
			MAP_OC_ITERATOR ret_iter = new_range.first;
			return ret_iter->second;
		}	
	}
}

ORIGIN_ADDR MapInst::get_old_origin_addr(ORIGIN_ADDR addr, BOOL is_in_cc)
{
	if(is_in_cc){
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		
		MAP_CO_ITERATOR old_iter = map_cc_to_origin[old_idx].find(addr);
		ASSERT(old_iter!=map_cc_to_origin[old_idx].end());
		ORIGIN_ADDR origin_inst_addr = old_iter->second;
		return origin_inst_addr;
	}else
		return addr;
}

ORIGIN_ADDR MapInst::get_new_origin_addr(ORIGIN_ADDR addr, BOOL is_in_cc)
{
	if(is_in_cc){
		UINT8 new_idx = curr_idx==0 ? 1 : 0;
		
		MAP_CO_ITERATOR new_iter = map_cc_to_origin[new_idx].find(addr);
		ASSERT(new_iter!=map_cc_to_origin[new_idx].end());
		ORIGIN_ADDR origin_inst_addr = new_iter->second;
		return origin_inst_addr;
	}else
		return addr;
}

