#ifndef _STACK_H_
#define _STACK_H_
#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include <map>

class ShareStack{
private:
	ORIGIN_ADDR origin_stack_start;
	ORIGIN_ADDR origin_stack_end;
	ADDR current_stack_start; 
	SIZE stack_size;
public:
	ShareStack(CodeSegment &codeSegment)
	{
		ASSERT(codeSegment.is_stack && !codeSegment.is_code_cache);
		origin_stack_start = codeSegment.code_start;
		stack_size = codeSegment.code_size;
		origin_stack_end = origin_stack_start + stack_size;
		ASSERT(codeSegment.native_map_code_start);
		current_stack_start = (ADDR)codeSegment.native_map_code_start;
	}
	void relocate_return_address(multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_inst_info);
	void relocate_current_pc(multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_inst_info);
};


#endif
