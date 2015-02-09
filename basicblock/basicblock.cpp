#include "basicblock.h"

SIZE BasicBlock::copy_instructions(ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr)
{
	ASSERT(!instruction_vec.empty());
	_generate_copy_size = 0;
	SIZE inst_copy_size = 0;
	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); iter!=instruction_vec.end(); iter++){
		curr_target_addr += inst_copy_size;
		origin_target_addr += inst_copy_size;
		inst_copy_size = (*iter)->copy_instruction(curr_target_addr, origin_target_addr);
		_generate_copy_size += inst_copy_size;
	} 
	return _generate_copy_size;
}

void BasicBlock::dump()
{
	ASSERT(!instruction_vec.empty());
	INFO("(BasicBlock *)%p[0x%lx-0x%lx](%d)\n", this, get_first_instruction()->get_inst_origin_addr(), 
		get_last_instruction()->get_inst_origin_addr(), (INT32)instruction_vec.size());
	INFO("  |---Prev BasicBlock:  ");
	INT32 idx=0;
	for(vector<BasicBlock*>::iterator it = prev_bb_vec.begin(); it!=prev_bb_vec.end(); it++){
		INFO("\n  |   ");
		PRINT("  |---<%d>(BasicBlock *)%p[0x%lx-0x%lx]", idx,  *it, (*it)->get_first_instruction()->get_inst_origin_addr(), 
			(*it)->get_last_instruction()->get_inst_origin_addr());
		idx++;
	}
	if(idx==0)
		PRINT("NULL");
	INFO("\n  |---Target BasicBlock:  ");
	idx=0;
	for(vector<BasicBlock*>::iterator it = target_bb_vec.begin(); it!=target_bb_vec.end(); it++){
		INFO("\n  |   ");
		PRINT("  |---<%d>(BasicBlock *)%p[0x%lx-0x%lx]", idx,  *it, (*it)->get_first_instruction()->get_inst_origin_addr(), 
			(*it)->get_last_instruction()->get_inst_origin_addr());
		idx++;
	}
	if(idx>1)
		ASSERT(0);
	if(idx==0)
		PRINT("NULL");
	INFO("\n  |---Fallthrough BasicBlock:  ");
	if(fallthrough_bb){
		INFO("\n  |   ");
		PRINT("  |---(BasicBlock *)%p[0x%lx-0x%lx]\n", fallthrough_bb, fallthrough_bb->get_first_instruction()->get_inst_origin_addr(),
			 fallthrough_bb->get_last_instruction()->get_inst_origin_addr());
	}else
		PRINT("NULL\n");
	INFO("  |---DUMP ALL INSTRUCTIONS:\n");
	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); iter!=instruction_vec.end(); iter++){
		PRINT("        |---");
		(*iter)->dump();
	}
}