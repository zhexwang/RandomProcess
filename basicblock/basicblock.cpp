#include "basicblock.h"
#include "inst_macro.h"

SIZE BasicBlock::copy_random_insts(ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation)
{
	ASSERT(!instruction_vec.empty() && is_already_finish_analyse && !is_finish_generate_cc);
	ASSERT((_generate_cc_size==0)&&(_curr_copy_addr==0)&&(_origin_copy_addr==0));
	//calculate the origin_entry_addr
	if(random_inst_start!=end()){
		_origin_copy_addr = origin_target_addr;
		_curr_copy_addr = curr_target_addr;
		if(entry_can_be_random)
			_origin_entry_addr_after_random = _origin_copy_addr;
		else
			_origin_entry_addr_after_random = instruction_vec.front()->get_inst_origin_addr();
	}else
		_origin_entry_addr_after_random = instruction_vec.front()->get_inst_origin_addr();
	
	//copy instrucitons
	SIZE inst_copy_size = 0;
	for(vector<Instruction*>::iterator iter = random_inst_start; (iter<=random_inst_end && iter!=end()); iter++){
		curr_target_addr += inst_copy_size;
		origin_target_addr += inst_copy_size;
		if((*iter)->isOrdinaryInst())//copy ordinary instructions
			inst_copy_size = (*iter)->copy_instruction(curr_target_addr, origin_target_addr);
		else{//handle special instructions 
			ASSERT(iter==random_inst_end);
			inst_copy_size = random_unordinary_inst(*iter, curr_target_addr, origin_target_addr, relocation);
		}
		_generate_cc_size += inst_copy_size;
	}

	//whether jump back or not
	if(random_last_inst_need_jump_back){
		ASSERT((random_inst_end!=end())&&((*random_inst_end)->isOrdinaryInst()));
		curr_target_addr += inst_copy_size;
		origin_target_addr += inst_copy_size;
		ORIGIN_ADDR jmp_target_origin = (*(random_inst_end+1))->get_inst_origin_addr();
		INT64 offset = jmp_target_origin- origin_target_addr - 0x5;
		ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
		INT8 *ptr1 = reinterpret_cast<INT8*>(curr_target_addr);
		*(ptr1++) = 0xe9;//jump opcode
		INT32 *ptr2 = reinterpret_cast<INT32*>(ptr1);
		*(ptr2++) = (INT32)offset;
		_generate_cc_size += 0x5;
	}
	return _generate_cc_size;
}

SIZE BasicBlock::random_unordinary_inst(Instruction *inst, CODE_CACHE_ADDR curr_target_addr, 
	ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation)
{
	CODE_CACHE_ADDR cc_start = curr_target_addr;
	ASSERT(!is_finish_generate_cc);
	if(inst->isRet())
		return inst->copy_instruction(curr_target_addr, origin_target_addr);
	else if(inst->isCall())
		ASSERTM(0, "call inst can not exist in this function!");
	else if(inst->isDirectJmp()){
		ASSERT(target_bb_vec.size()<=1);
		if(target_bb_vec.size()==1){
			RELOCATION_ITEM reloc_info = {JMP_REL32_BB_PTR, (cc_start+1), (ADDR)target_bb_vec[0]};
			relocation.push_back(reloc_info);
			JMP_REL32(0x0, cc_start);
		}else{
			ORIGIN_ADDR target_addr = inst->getBranchTargetOrigin();
			INT64 offset = target_addr - origin_target_addr - 0x5;
			ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
			JMP_REL32(offset, cc_start);
			ERR("%p, JMP no target!\n", this);
		}
	}else if(inst->isIndirectJmp()){
		ASSERT(0);
	}else if(inst->isConditionBranch()){
		ASSERT((target_bb_vec.size()==1)&&fallthrough_bb);
		/*
		 * Indicates the instruction is one of:
		 * JCXZ, JO, JNO, JB, JAE, JZ, JNZ, JBE, JA, JS, JNS, JP, JNP, JL, JGE, JLE, JG, LOOP, LOOPZ, LOOPNZ.
		 */
		switch(inst->get_inst_opcode()){
			case I_JA: case I_JAE: case I_JB: case I_JBE: case I_JCXZ: case I_JECXZ: case I_JG:
			case I_JGE: case I_JL: case I_JLE: case I_JNO: case I_JNP: case I_JNS: case I_JNZ:
			case I_JO: case I_JP: case I_JRCXZ: case I_JS: case I_JZ:{
				//judge is jump short or jump near
				UINT8 first_byte = inst->get_inst_code_first_byte();
				UINT8 base_opcode = first_byte==0x0f ? inst->get_inst_code_second_byte() : first_byte+0x10;
				//generate target
				UINT8 *ptr = reinterpret_cast<UINT8*>(cc_start);
				*(ptr++) = 0x0f;
				*(ptr++) = base_opcode;
				RELOCATION_ITEM target_reloc_info = {JMP_REL32_BB_PTR, (ADDR)ptr, (ADDR)target_bb_vec[0]};
				relocation.push_back(target_reloc_info);
				INT32 *ptr1 = reinterpret_cast<INT32 *>(ptr);
				*(ptr1++) = 0x0;
				cc_start = reinterpret_cast<ADDR>(ptr1);
				//generate fallthrough
				RELOCATION_ITEM fallthrough_reloc_info = {JMP_REL32_BB_PTR, (cc_start+1), (ADDR)fallthrough_bb};
				relocation.push_back(fallthrough_reloc_info);
				JMP_REL32(0x0, cc_start);				
				break;}
			case I_LOOP: case I_LOOPZ: case I_LOOPNZ:{
				ASSERTM(0, "LOOP unhandle!\n");
				break;}
			default:
				ERR("Unknown condition branch inst: ");
				inst->dump();
				ASSERT(0);
		}
	}else
		ASSERT(0);
	return cc_start - curr_target_addr;
}

void BasicBlock::intercept_jump()
{
	ASSERT(!instruction_vec.empty() && is_already_finish_analyse && (_origin_copy_addr!=0) && (_curr_copy_addr!=0) 
		&& is_finish_generate_cc);
	if(random_inst_start!=instruction_vec.end()){
		if(!entry_can_be_random){
			//get addr in this process
			ADDR intercept_curr_addr = (*random_inst_start)->get_inst_current_addr();
			ORIGIN_ADDR intercept_origin_addr = (*random_inst_start)->get_inst_origin_addr();
			INT64 offset = _origin_copy_addr - intercept_origin_addr - 0x5;
			ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
			INT8 *ptr1 = reinterpret_cast<INT8*>(intercept_curr_addr);
			*ptr1 = 0xd6;//illegal opcode
			INT32 *ptr2 = reinterpret_cast<INT32*>(++ptr1);
			*(ptr2++) = (INT32)offset;
			*ptr1 = 0xe9;//jump opcode
		}
	}
}

void BasicBlock::random_analysis()
{
	if(is_already_finish_inst_analyse)
		return ;
	ASSERT(!instruction_vec.empty() && is_already_finish_analyse);
	//calculate the begin
	if(entry_can_be_random)
		random_inst_start = instruction_vec.begin();
	else
		random_inst_start = find_first_least_size_instruction(5);
	//calculate the end
	if(is_call_bb()){
		if(random_inst_start==(instruction_vec.end()-1)){
			random_inst_start = instruction_vec.end();
			random_inst_end = instruction_vec.end();
			random_last_inst_need_jump_back = false;
		}else if(random_inst_start == instruction_vec.end()){
			random_inst_end = instruction_vec.end();
			random_last_inst_need_jump_back = false;
		}else{
			random_inst_end = instruction_vec.end()-2;
			random_last_inst_need_jump_back = true;
		}	
	}else{
		if(random_inst_start==instruction_vec.end()){
			random_inst_end = instruction_vec.end();
			random_last_inst_need_jump_back = false;
		}else{
			random_inst_end = instruction_vec.end()-1;
			random_last_inst_need_jump_back = false;
		}
	}
		
	is_already_finish_inst_analyse = true;
}

BB_INS_ITER BasicBlock::find_first_least_size_instruction(SIZE least_size)
{
	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); iter!=instruction_vec.end(); iter++){
		if((*iter)->get_inst_size()>=least_size)
			return iter;
	}
	return instruction_vec.end();
}

void BasicBlock::dump()
{
	ASSERT(!instruction_vec.empty());
	PRINT(COLOR_BLUE"(BasicBlock *)%p[0x%lx-0x%lx](%d)[RandomEntry: 0x%lx]\n"COLOR_END, this, get_first_instruction()->get_inst_origin_addr(), 
		get_last_instruction()->get_inst_origin_addr(), (INT32)instruction_vec.size(), _origin_entry_addr_after_random);
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
		INFO("  |");
		PRINT("     |---");
		(*iter)->dump();
	}
	if(is_already_finish_inst_analyse){
		ORIGIN_ADDR modify_addr = modify_inst_origin_addr();
		switch(modify_addr){
			case 0: INFO("  |---DUMP CC INSTRUCTIONS:(ENTRY RANDOM, INTERCEPT INST: UNKNOWN)\n"); break;
			case -1: INFO("  |---DUMP CC INSTRUCTIONS:(ALL INSTS DO NOT RANDOM)\n"); break;
			default:
				INFO("  |---DUMP CC INSTRUCTIONS:(ENTRY NOT RANDOM, INTERCEPT INST: %lx)\n", modify_addr);
		}
		
		if(random_inst_start!=instruction_vec.end()){
			if(is_finish_generate_cc)
				global_code_cache->disassemble("        |---", _curr_copy_addr, _curr_copy_addr+_generate_cc_size);
			else
				PRINT("        |---      CC DO NOT GENERATE\n");
		}else
			PRINT("        |---      CC NONE\n");
	}else
		INFO("  |---DO NOT ANALYSE RANDOM");
}