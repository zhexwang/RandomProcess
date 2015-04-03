#include "map_inst.h"

ORIGIN_ADDR MapInst::get_new_addr_from_old(ORIGIN_ADDR old_inst_addr, BOOL is_in_cc)
{
	if(is_in_cc){
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		
		MAP_CO_ITERATOR old_iter = map_cc_to_origin[old_idx].find(old_inst_addr);
		ASSERT(old_iter!=map_cc_to_origin[old_idx].end());
		ORIGIN_ADDR origin_inst_addr = old_iter->second;

		MAP_OC_PAIR old_range = map_origin_to_cc[old_idx].equal_range(origin_inst_addr);
		ASSERT((old_range.first)!=(old_range.second));
		INT32 old_count = map_origin_to_cc[old_idx].count(origin_inst_addr);
		INT32 target_idx = 0;
		for(MAP_OC_ITERATOR it = old_range.first; it!=old_range.second; it++, target_idx++){
			if((it->second)==origin_inst_addr)
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
		//old map should not has this instruction
		INT32 count_num = map_origin_to_cc[old_idx].count(old_inst_addr);
		ASSERT(count_num==0);	

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
