#ifndef _STACK_H_
#define _STACK_H_
#include "type.h"
#include "utility.h"
#include "map_inst.h"
#include "communication.h"
#include <map>
#include <ucontext.h>
#include <sys/types.h>
#include <string>
extern Communication *communication;

class ShareStack{
private:
	ORIGIN_ADDR origin_stack_start;
	ORIGIN_ADDR origin_stack_end;
	ADDR current_stack_start; 
	SIZE stack_size;
	BOOL _is_main_stack;

	COMMUNICATION_INFO *info;
public:
	static map<ORIGIN_ADDR, STACK_TYPE> stack_map; 
	static map<ORIGIN_ADDR, LIB_STACK_TYPE> lib_stack_map;
	static map<string, LIB_STACK_TYPE> unused_func_map;
	ShareStack(ORIGIN_ADDR origin_start, SIZE size, ADDR curr_start, BOOL is_main_stack)
		: origin_stack_start(origin_start), current_stack_start(curr_start), stack_size(size), _is_main_stack(is_main_stack)
	{
		origin_stack_end = origin_stack_start + stack_size;
		info = (COMMUNICATION_INFO *)current_stack_start;
		if(_is_main_stack)
			communication->set_main_communication(info);
		else
			communication->set_child_communication(info);
	}

	ADDR get_curr_addr(ORIGIN_ADDR origin_addr)
	{
		ASSERT((origin_addr>=origin_stack_start) && (origin_addr<=origin_stack_end));
		return origin_addr - origin_stack_start + current_stack_start;
	}
	BOOL can_stop(){return info->can_stop==0 ? false : true;}
	void relocate_return_address(MapInst *map_inst);
	BOOL check_relocate(MapInst *map_inst);
	BOOL return_address_in_unused_rbp_function(MapInst *map_inst, ORIGIN_ADDR return_address, ORIGIN_ADDR &origin_rsp);
	void relocate_current_pc(MapInst *map_inst);
	static void dump_stack_type_by_origin_addr(ORIGIN_ADDR addr);
};


#endif
