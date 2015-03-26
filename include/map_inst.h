#ifndef _MAP_INST_H_
#define _MAP_INST_H_

#include "utility.h"
#include "type.h"
#include <map>
using namespace std;

class MapInst
{
protected:
	typedef multimap<ORIGIN_ADDR, ORIGIN_ADDR>::iterator MAP_OC_ITERATOR;
	typedef pair<MAP_OC_ITERATOR, MAP_OC_ITERATOR> MAP_OC_PAIR;
	typedef map<ORIGIN_ADDR, ORIGIN_ADDR>::iterator MAP_CO_ITERATOR;
	
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> map_origin_to_cc[2];//origin_inst-->code_cache_inst
	map<ORIGIN_ADDR, ORIGIN_ADDR> map_cc_to_origin[2];//code_cache_inst-->origin_inst
	UINT8 curr_idx;
public:
	MapInst() :curr_idx(1) 
	{
		;
	}
	//get mapping from origin inst to cc
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> &get_curr_mapping_oc()
	{
		return map_origin_to_cc[curr_idx];
	}
	//get mapping from cc inst to origin
	map<ORIGIN_ADDR, ORIGIN_ADDR> &get_curr_mapping_co()
	{
		return map_cc_to_origin[curr_idx];
	}	
	void flush()
	{
		curr_idx = curr_idx==0 ? 1 : 0;
		map_origin_to_cc[curr_idx].clear();
		map_cc_to_origin[curr_idx].clear();
	}
	ORIGIN_ADDR get_new_addr_from_old(ORIGIN_ADDR old_inst_addr, BOOL is_in_cc);
};

#endif
