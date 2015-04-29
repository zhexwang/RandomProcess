#include "basicblock.h"
#include "inst_macro.h"
#include "debug-config.h"
#include <map>
extern BOOL need_trace_debug;

SIZE BasicBlock::copy_random_insts(ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation
	, multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	ASSERT(!instruction_vec.empty() && !is_finish_generate_cc);
	ASSERT((_generate_cc_size==0)&&(_curr_copy_addr==0)&&(_origin_copy_addr==0));

	_origin_copy_addr = origin_target_addr;
	_curr_copy_addr = curr_target_addr;
	SIZE cc_size = 0;
	//copy instrucitons
	SIZE inst_copy_size = 0;
	
#ifdef _DEBUG_BB_TRACE
	//insert debug inst
	if(need_trace_debug){
		ADDR curr_target_bk = curr_target_addr;
		ORIGIN_ADDR entry_addr = get_origin_addr_before_random();
		MOVL_IMM32(0x10000, (INT32)entry_addr, curr_target_bk);
		MOVL_IMM32(0x10004, (INT32)(entry_addr>>32), curr_target_bk);
		inst_copy_size = curr_target_bk - curr_target_addr;
		cc_size +=inst_copy_size;
	}
#endif

	for(vector<Instruction*>::iterator iter = instruction_vec.begin(); (iter<=instruction_vec.end() && iter!=end()); iter++){
		curr_target_addr += inst_copy_size;
		origin_target_addr += inst_copy_size;
		
		if((*iter)->isOrdinaryInst()){//copy ordinary instructions
			inst_copy_size = (*iter)->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
			if((*iter)==get_last_instruction()){
				if(fallthrough_bb){
					curr_target_addr += inst_copy_size;
					origin_target_addr += inst_copy_size;
					inst_copy_size += 0x5;
					RELOCATION_ITEM reloc_info = {REL32_BB_PTR, (curr_target_addr+1), origin_target_addr+5, (ADDR)fallthrough_bb};
					relocation.push_back(reloc_info);
					//instrument fallthrough
					JMP_REL32(0x0, curr_target_addr);
				}else{
					switch((*iter)->get_inst_opcode()){
						case I_NOP:{
							ORIGIN_ADDR next_inst_addr = (*iter)->get_inst_origin_addr() + (*iter)->get_inst_size();
							INT64 offset = next_inst_addr - origin_target_addr - 0x5;
							ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
							JMP_REL32(offset, curr_target_addr);
							inst_copy_size = 5;
							}break;
						case I_HLT:
						case I_UD2:{
							INV_INS_1(curr_target_addr);
							inst_copy_size = 1;
							}break;							
						default:
							ASSERT(0);
					}						
				}
				
				//record
				ORIGIN_ADDR inst_origin_addr = (*iter)->get_inst_origin_addr();
				map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));	
				map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
			}
		}else{//handle special instructions 
			ASSERT((*iter)==get_last_instruction());
			inst_copy_size = random_unordinary_inst(*iter, curr_target_addr, origin_target_addr, relocation, map_origin_to_cc, map_cc_to_origin);
		}
		
		cc_size += inst_copy_size;
	}

	ASSERT(cc_size<=0x7fffffff);
	_generate_cc_size = cc_size;

	return _generate_cc_size;
}

static  BOOL can_hash_low_32bits(vector<BasicBlock*> &vec)
{
	UINT32 high32_base = 0;
	for(vector<BasicBlock*>::iterator iter = vec.begin(); iter!=vec.end(); iter++){
		ORIGIN_ADDR addr64 = (*iter)->get_origin_addr_before_random();
		UINT32 high32 = (addr64>>32)&0xffffffff;
		if(iter==vec.begin())
			high32_base = high32;
		
		if(high32_base!=high32)
			return false;
	}
	return true;
}

static UINT8 convert_reg64_to_reg32(UINT8 reg_index)
{
	ASSERT(reg_index>=R_RAX && reg_index<=R_R15);
	UINT8 d_value = R_EAX - R_RAX;
	return reg_index + d_value;
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
		}else
			INV_INS_1(cc_start);
		//record second instruction mapping
		ORIGIN_ADDR second_inst = first_inst+0x5;
		map_origin_to_cc.insert(make_pair(inst_origin_addr, second_inst));	
		map_cc_to_origin.insert(make_pair(second_inst, inst_origin_addr));
	}else if(inst->isIndirectCall()){
		SIZE copy_size = inst->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
		curr_target_addr += copy_size;
		origin_target_addr += copy_size;
		if(fallthrough_bb){
			//generate second instruction
			RELOCATION_ITEM reloc_info = {REL32_BB_PTR, (curr_target_addr+1), (origin_target_addr+5),(ADDR)fallthrough_bb};
			relocation.push_back(reloc_info);
			//record second instruction mapping
			map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));	
			map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
			JMP_REL32(0x0, curr_target_addr);
			return copy_size+0x5;
		}else{
			//record second instruction mapping
			map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));	
			map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
			INV_INS_1(curr_target_addr);
			return copy_size+0x1;
		}
	}else if(inst->isDirectJmp()){
		ASSERT((target_bb_vec.size()<=1)&&!fallthrough_bb);
		if(target_bb_vec.size()==1){
			ASSERT(target_bb_vec[0]->get_origin_addr_before_random()==inst->getBranchTargetOrigin());
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
			//ERR("JMP(0x%lx) no target(0x%lx)!\n", inst->get_inst_origin_addr(), target_addr);
			//record mapping
			map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
			map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
		}
	}else if(inst->isIndirectJmp()){
		ASSERT(!fallthrough_bb);
		if(target_bb_vec.size()==0){
			//ERR("JMPIN(0x%lx) no target!\n", inst->get_inst_origin_addr());
			return inst->copy_instruction(curr_target_addr, origin_target_addr, map_origin_to_cc, map_cc_to_origin);
		}else{
			BOOL ret =  can_hash_low_32bits(target_bb_vec);
			ASSERT(ret);
			switch(inst->get_operand_type(0)){
				case O_REG:{
					UINT8 reg32 = convert_reg64_to_reg32(inst->get_reg_operand_index(0));
					for(UINT32 idx = 0; idx<target_bb_vec.size(); idx++){
						ADDR bk_cc_start = cc_start;
						ORIGIN_ADDR addr64 = target_bb_vec[idx]->get_origin_addr_before_random();
						CMP_REG32_IMM32(reg32, (addr64&0xffffffff), cc_start);
						//record cmp inst
						map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
						map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));	
						origin_target_addr += cc_start - bk_cc_start;
						//je
						UINT8 *ptr = reinterpret_cast<UINT8*>(cc_start);
						*(ptr++) = 0x0f;
						*(ptr++) = 0x84;
						INT32 *ptr1 = reinterpret_cast<INT32 *>(ptr);
						*(ptr1++) = 0x0;
						cc_start = reinterpret_cast<ADDR>(ptr1);
						RELOCATION_ITEM target_reloc_info = {REL32_BB_PTR, (ADDR)ptr, origin_target_addr+6, (ADDR)(target_bb_vec[idx])};
						relocation.push_back(target_reloc_info);
						//record je 0x0 inst
						map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
						map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));	
						origin_target_addr += 6;		
					}
					INV_INS_1(cc_start);
					cc_start+=1;
					//record illegel inst
					map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
					map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
					}break;
				case O_DISP:
				case O_SMEM:
				case O_MEM:{ 
					//ASSERT(0);
					SIZE jmpin_size = inst->get_inst_size();
					UINT8 *jmpin_instcode = new UINT8[jmpin_size];
					inst->get_inst_code(jmpin_instcode, jmpin_size);
					for(UINT32 idx = 0; idx<target_bb_vec.size(); idx++){
						ADDR bk_cc_start = cc_start;
						ORIGIN_ADDR addr64 = target_bb_vec[idx]->get_origin_addr_before_random();
						if(inst->get_reg_operand_index(0) == R_RIP){
							ASSERT(inst->isRipRelative());
							convertJmpinRipToCmpLImm32(jmpin_instcode, jmpin_size, inst->get_inst_origin_addr(),\
								(addr64&0xffffffff), cc_start, origin_target_addr);
						}else{
							ASSERT(!inst->isRipRelative());
							convertJmpinToCmpLImm32(jmpin_instcode, jmpin_size, (addr64&0xffffffff), cc_start);
						}
						//record cmp inst
						map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
						map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));	
						origin_target_addr += cc_start - bk_cc_start;
						//je
						UINT8 *ptr = reinterpret_cast<UINT8*>(cc_start);
						*(ptr++) = 0x0f;
						*(ptr++) = 0x84;
						INT32 *ptr1 = reinterpret_cast<INT32 *>(ptr);
						*(ptr1++) = 0x0;
						cc_start = reinterpret_cast<ADDR>(ptr1);
						RELOCATION_ITEM target_reloc_info = {REL32_BB_PTR, (ADDR)ptr, origin_target_addr+6, (ADDR)(target_bb_vec[idx])};
						relocation.push_back(target_reloc_info);
						//record je 0x0 inst
						map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
						map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));	
						origin_target_addr += 6;		
					}
					delete []jmpin_instcode;
					INV_INS_1(cc_start);
					cc_start+=1;
					//record illegel inst
					map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
					map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
					}break;
				case O_PTR:
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
		ASSERT(target_bb_vec.size()<=1);
		/*
		 * Indicates the instruction is one of:
		 * JCXZ, JO, JNO, JB, JAE, JZ, JNZ, JBE, JA, JS, JNS, JP, JNP, JL, JGE, JLE, JG, LOOP, LOOPZ, LOOPNZ.
		 */
		switch(inst->get_inst_opcode()){
			case I_JA: case I_JAE: case I_JB: case I_JBE: case I_JCXZ: case I_JECXZ: case I_JG:
			case I_JGE: case I_JL: case I_JLE: case I_JNO: case I_JNP: case I_JNS: case I_JNZ:
			case I_JO: case I_JP: case I_JRCXZ: case I_JS: case I_JZ:{
				UINT8 *ptr = reinterpret_cast<UINT8*>(cc_start);
				//judge is jump short or jump near and handle prefix 
				UINT8 first_byte = inst->get_inst_code_first_byte();
				UINT8 second_byte = inst->get_inst_code_second_byte();
				UINT8 base_opcode;
				UINT8 first_inst_size;
				if(first_byte==0x3e){
					*(ptr++) = 0x3e;
					UINT8 third_byte = inst->get_inst_code_third_byte();
					base_opcode = second_byte==0x0f ? third_byte : second_byte+0x10;
					first_inst_size = 7;
				}else{
					base_opcode = first_byte==0x0f ? second_byte : first_byte+0x10;
					first_inst_size = 6;
				}
				//generate target				
				*(ptr++) = 0x0f;
				*(ptr++) = base_opcode;
				INT32 *ptr1 = reinterpret_cast<INT32 *>(ptr);
				if(target_bb_vec.size()==1){
					*(ptr1++) = 0x0;
					ORIGIN_ADDR target_addr = target_bb_vec[0]->get_origin_addr_before_random();
					ORIGIN_ADDR real_target_addr = inst->getBranchTargetOrigin();
					if(target_addr!=real_target_addr){//assert lock prefix
						ASSERT((target_addr+1)==real_target_addr);
						ASSERT(target_bb_vec[0]->get_first_instruction()->get_inst_code_first_byte()==0xf0);
					}
					RELOCATION_TYPE type = target_addr==real_target_addr ? REL32_BB_PTR : REL32_BB_PTR_A_1;
	 				RELOCATION_ITEM target_reloc_info = {type, (ADDR)ptr, origin_target_addr+first_inst_size, (ADDR)target_bb_vec[0]};
					relocation.push_back(target_reloc_info);
				}else{
					//ERR("CND Branch(0x%lx) out of function, target is 0x%lx!\n", inst->get_inst_origin_addr(), inst->getBranchTargetOrigin());
					*(ptr1++) = inst->getBranchTargetOrigin() - (origin_target_addr+first_inst_size);
				}
				//record first instruction mapping
				map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr));
				map_cc_to_origin.insert(make_pair(origin_target_addr, inst_origin_addr));
				
				cc_start = reinterpret_cast<ADDR>(ptr1);
				UINT8 second_inst_size = 5;
				//generate fallthrough
				if(fallthrough_bb){
					RELOCATION_ITEM fallthrough_reloc_info = {REL32_BB_PTR, (cc_start+1),\
						origin_target_addr+second_inst_size+first_inst_size, (ADDR)fallthrough_bb};
					relocation.push_back(fallthrough_reloc_info);
					JMP_REL32(0x0, cc_start);	
				}else{//false call
					INT64 offset = inst->get_inst_origin_addr() + inst->get_inst_size() - (origin_target_addr+second_inst_size+first_inst_size);
					ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
					JMP_REL32(offset, cc_start);
				}	
				//record second instructoin mapping
				map_origin_to_cc.insert(make_pair(inst_origin_addr, origin_target_addr+first_inst_size));
				map_cc_to_origin.insert(make_pair(origin_target_addr+first_inst_size, inst_origin_addr));
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
	PRINT(COLOR_HIGH_GREEN"(BasicBlock *)%p[0x%lx-0x%lx](%d)[RandomEntry: 0x%lx]\n"COLOR_END, this, get_first_instruction()->get_inst_origin_addr(), 
		get_last_instruction()->get_inst_origin_addr(), (INT32)instruction_vec.size(), _origin_copy_addr);
	INT32 idx=0;
	INFO("  |---Prev BasicBlock:  ");
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
	
	if(is_finish_generate_cc){
		_DecodedInst  *disassembled = new _DecodedInst[_generate_cc_size];
		UINT32 decodedInstsCount = 0;
		distorm_decode(_origin_copy_addr, (UINT8*)_curr_copy_addr, _generate_cc_size, Decode64Bits, disassembled, _generate_cc_size, &decodedInstsCount);
		for(UINT32 i=0; i<decodedInstsCount; i++){
			PRINT("        |---%12lx (%02d) %-24s  %s%s%s\n", disassembled[i].offset, disassembled[i].size, disassembled[i].instructionHex.p,\
				(char*)disassembled[i].mnemonic.p, disassembled[i].operands.length != 0 ? " " : "", (char*)disassembled[i].operands.p);
		}
		delete [] disassembled;
	}else
		PRINT("        |---      CC DO NOT GENERATE\n");

}
