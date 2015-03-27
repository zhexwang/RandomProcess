#ifndef _STACK_H_
#define _STACK_H_
#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "map_inst.h"
#include "communication.h"
#include <map>
#include <ucontext.h>
#include <sys/types.h>
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
	ShareStack(CodeSegment &codeSegment, BOOL is_main_stack):_is_main_stack(is_main_stack)
	{
		ASSERT(codeSegment.is_stack && !codeSegment.is_code_cache);
		origin_stack_start = codeSegment.code_start;
		stack_size = codeSegment.code_size;
		origin_stack_end = origin_stack_start + stack_size;
		ASSERT(codeSegment.native_map_code_start);
		current_stack_start = (ADDR)codeSegment.native_map_code_start;

		info = (COMMUNICATION_INFO *)current_stack_start;

		if(_is_main_stack)
			communication->set_main_communication(info);
		
	}

	ADDR get_curr_addr(ORIGIN_ADDR origin_addr)
	{
		ASSERT((origin_addr>=origin_stack_start) && (origin_addr<=origin_stack_end));
		return origin_addr - origin_stack_start + current_stack_start;
	}

	void relocate_return_address(MapInst *map_inst);

	void relocate_current_pc(MapInst *map_inst);
};


#endif
