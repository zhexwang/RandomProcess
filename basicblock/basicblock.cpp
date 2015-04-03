#include "basicblock.h"
#include "inst_macro.h"
#include <map>

SIZE BasicBlock::copy_random_insts(ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation
	, multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	ASSERT(!instruction_vec.empty() && !is_finish_generate_cc);
	ASSERT((_generate_cc_size==0)&&(_curr_copy_addr==0)&&(_origin_copy_addr==0));

	_origin_copy_addr = origin_target_addr;
	_curr_copy_addr = curr_target_addr;
	//copy instrucitons
	SIZE inst_copy_size = 0;
	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); (iter<=instruction_vec.end() && iter!=end()); iter++){
		curr_target_addr += inst_copy_size;
		origin_target_addr += inst_copy_size;
		
		if((*iter)->isOrdinaryInst()){//copy ordinary instructions
			inst_copy_size = (*iter)->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
			if((*iter)==get_last_instruction()){
				ASSERT(fallthrough_bb);
				curr_target_addr += inst_copy_size;
				origin_target_addr += inst_copy_size;
				inst_copy_size += 0x5;
				RELOCATION_ITEM reloc_info = {REL32_BB_PTR, (curr_target_addr+1), origin_target_addr+5, (ADDR)fallthrough_bb};
				relocation.push_back(reloc_info);
				JMP_REL32(0x0, curr_target_addr);				
			}
		}else{//handle special instructions 
			ASSERT((*iter)==get_last_instruction());
			inst_copy_size = random_unordinary_inst(*iter, curr_target_addr, origin_target_addr, relocation, map_origin_to_cc, map_cc_to_origin);
		}
		_generate_cc_size += inst_copy_size;
	}
	

	return _generate_cc_size;
}

SIZE BasicBlock::random_unordinary_inst(Instruction *inst, CODE_CACHE_ADDR curr_target_addr, 
	ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation, multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, 
	map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	CODE_CACHE_ADDR cc_start = curr_target_addr;
	ORIGIN_ADDR inst_origin_addr = inst->get_inst_origin_addr();
	ASSERT(!is_finish_generate_cc);
	if(inst->isRet())
		return inst->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
	else if(inst->isDirectCall()){
		//generate first instruction 
		ORIGIN_ADDR target_addr = inst->getBranchTargetOrigin();
		INT64 offset = target_addr - origin_target_addr - 0x5;
		ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
		CALL_REL32(offset, cc_start);
		//record first instruction mapping
		ORIGIN_ADDR first_inst = origin_target_addr;
		map_origin_to_cc.insert(make_pair(inst_origin_addr, first_inst));
		map_cc_to_origin.insert(make_pair(first_inst, inst_origin_addr));
		if(fallthrough_bb){
			//generate second instruction
			RELOCATION_ITEM reloc_info = {REL32_BB_PTR, (cc_start+1), (origin_target_addr+10),(ADDR)fallthrough_bb};
			relocation.push_back(reloc_info);
			JMP_REL32(0x0, cc_start);
			//record second instruction mapping
			ORIGIN_ADDR second_inst = first_inst+0x5;
			map_origin_to_cc.insert(make_pair(inst_origin_addr, second_inst));	
			map_cc_to_origin.insert(make_pair(second_inst, inst_origin_addr));
		}
	}else if(inst->isIndirectCall())
		return inst->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
	else if(inst->isDirectJmp()){
		ASSERT((target_bb_vec.size()<=1)&&!fallthrough_bb);
		if(target_bb_vec.size()==1){
			//generate target instruction
			RELOCATION_ITEM reloc_info = {REL32_BB_PTR, (cc_start+1), (origin_target_addr+5), (ADDR)target_bb_vec[0]};
			relocation.push_back(reloc_info);
			JMP_REL32(0x0, cc_start);
			//record mapping
			map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
			map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
		}else{
			//generate target instruction
			ORIGIN_ADDR target_addr = inst->getBranchTargetOrigin();
			INT64 offset = target_addr - origin_target_addr - 0x5;
			ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
			JMP_REL32(offset, cc_start);
			ERR("JMP(0x%lx) no target(0x%lx)!\n", inst->get_inst_origin_addr(), target_addr);
			//record mapping
			map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
			map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
		}
	}else if(inst->isIndirectJmp()){
		ASSERT(!fallthrough_bb);
		if(target_bb_vec.size()==0){
			INV_INS_1(cc_start);
			cc_start+=1;
		}else{
			switch(inst->get_operand_type(0)){
				case O_REG:
					ASSERT(0);
					break;
				case O_DISP:
				case O_SMEM:
				case O_MEM:
				case O_PTR: 
					ASSERT(0);
					break;
				case O_PC:
				case O_IMM:
				case O_IMM1:
				case O_IMM2:
				case O_NONE:
					ASSERTM(0, "jmp* must have one operand!\n");
					break;
				default:
					ASSERTM(0, "jmp* operand is not recognized!\n");
			}
		}
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
				RELOCATION_ITEM target_reloc_info = {REL32_BB_PTR, (ADDR)ptr, origin_target_addr+6, (ADDR)target_bb_vec[0]};
				relocation.push_back(target_reloc_info);
				INT32 *ptr1 = reinterpret_cast<INT32 *>(ptr);
				*(ptr1++) = 0x0;
				//record first instruction mapping
				map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
				map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
				
				cc_start = reinterpret_cast<ADDR>(ptr1);
				//generate fallthrough
				RELOCATION_ITEM fallthrough_reloc_info = {REL32_BB_PTR, (cc_start+1), origin_target_addr+11, (ADDR)fallthrough_bb};
				relocation.push_back(fallthrough_reloc_info);
				JMP_REL32(0x0, cc_start);		
				//record second instructoin mapping
				map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr+0x6));
				map_cc_to_origin.insert(make_pair(origin_target_addr+0x6, inst_origin_addr));
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
		get_last_instruction()->get_inst_origin_addr(), (INT32)instruction_vec.size(), _origin_copy_addr);
	INFO("  |---Prev BasicBlock:  ");
	INT32 idx=0;
	for(vector<BasicBlock*>::iterator it = prev_bb_vec.begin(); it!=prev_bb_vec.end(); it++){
		INFO("\n  |   ");
		PRINT("  |---<%d>(BasicBlock *)%p[0x%lx-0x%lx](%d)[RandomEntry: 0x%lx]", idx,  *it, (*it)->get_first_instruction()->get_inst_origin_addr(), 
			(*it)->get_last_instruction()->get_inst_origin_addr(), (*it)->get_insts_num(), (*it)->get_origin_addr_after_random());
		idx++;
	}
	if(idx==0)
		PRINT("NULL");
	INFO("\n  |---Target BasicBlock:  ");
	idx=0;
	for(vector<BasicBlock*>::iterator it = target_bb_vec.begin(); it!=target_bb_vec.end(); it++){
		INFO("\n  |   ");
		PRINT("  |---<%d>(BasicBlock *)%p[0x%lx-0x%lx](%d)[RandomEntry: 0x%lx]", idx,  *it, (*it)->get_first_instruction()->get_inst_origin_addr(), 
			(*it)->get_last_instruction()->get_inst_origin_addr(),  (*it)->get_insts_num(), (*it)->get_origin_addr_after_random());
		idx++;
	}
	if(idx>1)
		ASSERT(0);
	if(idx==0)
		PRINT("NULL");
	INFO("\n  |---Fallthrough BasicBlock:  ");
	if(fallthrough_bb){
		INFO("\n  |   ");
		PRINT("  |---(BasicBlock *)%p[0x%lx-0x%lx](%d)[RandomEntry: 0x%lx]\n", fallthrough_bb, fallthrough_bb->get_first_instruction()->get_inst_origin_addr(),
			 fallthrough_bb->get_last_instruction()->get_inst_origin_addr(),  fallthrough_bb->get_insts_num(), fallthrough_bb->get_origin_addr_after_random());
	}else
		PRINT("NULL\n");
	INFO("  |---DUMP ALL INSTRUCTIONS:\n");
	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); iter!=instruction_vec.end(); iter++){
		INFO("  |");
		PRINT("     |---");
		(*iter)->dump();
	}

	INFO("  |---DUMP CC INSTRUCTIONS:\n");
	
	if(is_finish_generate_cc)
		code_cache->disassemble("        |---", _curr_copy_addr, _curr_copy_addr+_generate_cc_size);
	else
		PRINT("        |---      CC DO NOT GENERATE\n");

}