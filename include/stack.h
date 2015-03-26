#ifndef _STACK_H_
#define _STACK_H_
#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "map_inst.h"
#include <map>
#include <ucontext.h> 

extern UINT8 *main_communication_flag;

class ShareStack{
private:
	ORIGIN_ADDR origin_stack_start;
	ORIGIN_ADDR origin_stack_end;
	ADDR current_stack_start; 
	SIZE stack_size;
	ADDR *origin_rbp;
	struct ucontext *origin_uc;
public:
	ShareStack(CodeSegment &codeSegment)
	{
		ASSERT(codeSegment.is_stack && !codeSegment.is_code_cache);
		origin_stack_start = codeSegment.code_start;
		stack_size = codeSegment.code_size;
		origin_stack_end = origin_stack_start + stack_size;
		ASSERT(codeSegment.native_map_code_start);
		current_stack_start = (ADDR)codeSegment.native_map_code_start;

		origin_rbp = (ADDR*)current_stack_start;
		origin_uc = (struct ucontext*)(current_stack_start+sizeof(ADDR));

		main_communication_flag = (UINT8*)(current_stack_start+2*sizeof(ADDR));
	}
	void relocate_return_address(MapInst *map_inst);
	void relocate_current_pc(MapInst *map_inst);
};


#endif
