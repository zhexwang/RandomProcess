#include "function.h"
#include "inst_macro.h"
#include "stack.h"
#include "debug-config.h"
#include <time.h>
#include <set>

Function::Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size, CodeCache *cc)
	: _function_name(name), _origin_function_base(origin_function_base), _function_size(origin_function_size)
	, _random_cc_start(0), _random_cc_origin_start(0), _random_cc_size(0), code_cache(cc), is_already_disasm(false), is_already_split_into_bb(false)
	, is_already_finish_random(false) , is_already_finish_intercept(false), is_already_finish_erase(false), is_function_can_be_random(false)
	, is_already_finish_analysis_stack(false), _code_segment(code_segment)
	
{
	_function_base = code_segment->convert_origin_process_addr_to_this(_origin_function_base);
	entry_list.clear();
	entry_list.push_back((ENTRY_ITEM){origin_function_base, 0});
}
Function::~Function(){
	;
}
void Function::dump_function_origin()
{
	BLUE("[0x%lx-0x%lx]",  _origin_function_base, _function_size+_origin_function_base);
	BLUE("%s",_function_name.c_str());
	BLUE("(Path:%s)[Random:0x%lx-0x%lx]\n", _code_segment->file_path.c_str(), _random_cc_origin_start, _random_cc_origin_start+_random_cc_size);
	if(is_already_disasm){
		if(is_already_finish_analysis_stack){
			PRINT("Return address calculate method:  ");
			PRINT(COLOR_BLUE"RSP   "COLOR_END);
			PRINT("RBP_PLUS_4BYTE  ");
			PRINT(COLOR_YELLOW"RSP_PLUS_4BYTE\n"COLOR_END);
		}
		for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
			if(is_already_finish_analysis_stack){
				map<ORIGIN_ADDR, STACK_TYPE>::iterator ret = ShareStack::stack_map.find((*iter)->get_inst_origin_addr());
				ASSERT(ret!=ShareStack::stack_map.end());
				STACK_TYPE type = ret->second;
				switch(type){
					case S_RSP: PRINT(COLOR_BLUE); break;
					case S_RBP_A_4: break;
					case S_RSP_A_4: PRINT(COLOR_YELLOW); break;
					default: ASSERT(0);
				}
			}
			(*iter)->dump();
			PRINT(COLOR_END);
		}
	}else
		ERR("Do not disasm!\n");
}
void Function::dump_bb_origin()
{
	BLUE("[0x%lx-0x%lx]",  _origin_function_base, _function_size+_origin_function_base);
	BLUE("%s",_function_name.c_str());
	BLUE("(Path:%s)[Random:0x%lx-0x%lx]\n", _code_segment->file_path.c_str(), _random_cc_origin_start, _random_cc_origin_start+_random_cc_size);
	if(is_already_split_into_bb){
		for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
			(*ite)->dump();
		}
	}else
		ERR("Do not split into BB!\n");
}

INT32 direct_profile_func_entry_sum = 0;

void Function::disassemble()
{
	if(is_already_disasm)
		return ;
	ORIGIN_ADDR origin_addr = _origin_function_base;
	ADDR current_addr = _function_base;
	SIZE security_size = _function_size+1;//To see one more byte

	while(security_size>1){
		Instruction *instr = new Instruction(origin_addr);
		SIZE instr_size = instr->disassemable(security_size, (UINT8*)current_addr);
		if(instr->isSyscall()){
			//record signal stop instruction
			ORIGIN_ADDR stop_instruction = origin_addr+instr_size;
			ShareStack::lib_stack_map.insert(make_pair(stop_instruction, (LIB_STACK_TYPE){STACK_NONE,0}));
		}
			
		if(instr->isDirectJmp() || instr->isConditionBranch() || instr->isDirectCall()){
			ORIGIN_ADDR target_addr = instr->getBranchTargetOrigin();
			if(!is_in_function(target_addr)){
				_code_segment->direct_profile_func_entry.push_back(target_addr);
				direct_profile_func_entry_sum++;
			}else
				ASSERT(!instr->isDirectCall() || target_addr==_origin_function_base);
		}
		if(instr->isIndirectJmp()){
			CodeSegment::INDIRECT_MAP_ITERATOR ret = _code_segment->indirect_inst_map.find(instr->get_inst_origin_addr());
			if(ret==_code_segment->indirect_inst_map.end())
				;//YELLOW("JMPIN instruction does not have profile information!\n");
		}
		origin_addr += instr_size;
		current_addr += instr_size;
		security_size -= instr_size;
		_origin_function_instructions.push_back(instr);
	}
	ASSERT(security_size==1);
	is_already_disasm = true;
}

void Function::intercept_to_random_function()
{
	if(!is_function_can_be_random)
		return ;

	ASSERT(is_already_finish_random && _random_cc_origin_start!=0 && _random_cc_size!=0 && _random_cc_start!=0);
	ASSERT(!is_already_finish_intercept);
	
	vector<INT64> offset;
	for(INT32 idx=0; idx<(INT32)entry_list.size(); idx++){
		ORIGIN_ADDR random_target = entry_list[idx].random_entry - _random_cc_start + _random_cc_origin_start;
		INT64 Offset = random_target - entry_list[idx].origin_entry - 0x5;
		ASSERT((Offset > 0 ? Offset : -Offset) < 0x7fffffff);
		offset.push_back(Offset);
	}

	for(INT32 idx=0; idx<(INT32)entry_list.size(); idx++){
		ADDR curr_entry = entry_list[idx].origin_entry - _origin_function_base + _function_base;
		JMP_REL32(offset[idx], curr_entry);
	}
	is_already_finish_intercept = true;
}

void Function::erase_function()
{
	if(!is_function_can_be_random)
		return ;

	if(is_already_finish_erase)
		return ;
	
	ADDR erase_start = _function_base;
	ADDR erase_end = _function_base+_function_size;
	for(ADDR erase_item = erase_start; erase_item<erase_end; erase_item++)
		INV_INS_1(erase_item);
	is_already_finish_erase = true;
}

BOOL Function::check_random()
{
	INT32 entry_num = entry_list.size();

	if(entry_num==1)
		return _function_size>=5 ? true : false; 

	for(INT32 idx=0; idx<(entry_num-1); idx++){
		for(INT32 idx2 = idx+1; idx2<entry_num; idx2++){
			INT32 len = entry_list[idx].origin_entry - entry_list[idx2].origin_entry;
			if(len>-5 && len<5)
				return false;
		}

		INT32 len_end = entry_list[idx].origin_entry - (_origin_function_base+_function_size);
		if(len_end>-5 && len_end<5)
			return false;
	}

	return true ;
}

void random_array(INT32 *array, INT32 num)
{
	ASSERT(num>0);
	if(num==1){
		array[0] = 0;
		return ;
	}
	// 1.init array
	for(INT32 idx=0; idx<num; idx++)
		array[idx] = idx;

	// 2. random seed
	srand((INT32)time(NULL));
	for(INT32 idx=num-1; idx>0; idx--){
		// 3.swap
		INT32 swap_idx = rand()%idx;
		INT32 temp = array[swap_idx];
		array[swap_idx] = array[idx];
		array[idx] = temp;
	}
}
BOOL need_trace_debug = false;

void Function::random_function(multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, 
		map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	//get cc info
	CODE_CACHE_ADDR cc_curr_addr = 0;
	ORIGIN_ADDR cc_origin_addr = 0;
	code_cache->getCCCurrent(cc_curr_addr, cc_origin_addr);
	// 1.disasm
	ASSERT(is_already_disasm);
	is_function_can_be_random = check_random();
	if(!is_function_can_be_random)
		return ;
	// 2.split into bb
	split_into_basic_block();
	// 3.random
	ASSERT(is_already_split_into_bb);
	vector<RELOCATION_ITEM> relocation, relocation_temp;
	// 3.1 copy random insts
	ASSERT((_random_cc_start==0) && (_random_cc_origin_start==0) && (_random_cc_size==0));
#ifdef _DEBUG_BB_TRACE
	if(_function_name.find("_IO_vfprintf")!=string::npos){
		need_trace_debug = true;
	}else
		need_trace_debug = false;
#endif
	SIZE bb_copy_size = 0;	
	_random_cc_start = cc_curr_addr;
	_random_cc_origin_start = cc_origin_addr;
	INT32 *array = new INT32[bb_list.size()];
	random_array(array, bb_list.size());
	for(SIZE idx=0; idx<bb_list.size(); idx++){
		cc_curr_addr += bb_copy_size;
		cc_origin_addr += bb_copy_size;
		BasicBlock *random_bb = bb_list[array[idx]];
		//random some insts in BB
		bb_copy_size = random_bb->copy_random_insts(cc_curr_addr, cc_origin_addr, relocation, map_origin_to_cc, map_cc_to_origin);
		//record function entry
		for(INT32 idx=0; idx<(INT32)entry_list.size(); idx++){
			if(entry_list[idx].origin_entry==random_bb->get_origin_addr_before_random())
				entry_list[idx].random_entry = cc_curr_addr;
		}
		_random_cc_size += bb_copy_size;
	}
	delete []array;

	for(INT32 idx=0; idx<(INT32)entry_list.size(); idx++)
		ASSERT(entry_list[idx].random_entry!=0);
	
	// 3.2 relocate the address and finish 
	for(vector<RELOCATION_ITEM>::iterator iter = relocation.begin(); iter!=relocation.end(); iter++){
		RELOCATION_ITEM reloc_info = *iter;
		if(reloc_info.type==REL8_BB_PTR)
			ASSERT(0);
		else if(reloc_info.type==REL32_BB_PTR){
			ORIGIN_ADDR target_bb_entry = ((BasicBlock*)reloc_info.target_bb_ptr)->get_origin_addr_after_random();
			INT64 offset = target_bb_entry - reloc_info.origin_base_addr;
			ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
			*(UINT32*)(reloc_info.relocate_pos) = offset;
		}else if(reloc_info.type==REL32_BB_PTR_A_1){
			ORIGIN_ADDR target_bb_entry = ((BasicBlock*)reloc_info.target_bb_ptr)->get_origin_addr_after_random();
			INT64 offset = target_bb_entry + 1 - reloc_info.origin_base_addr;
			ASSERT((offset > 0 ? offset : -offset) < 0x7fffffff);
			*(UINT32*)(reloc_info.relocate_pos) = offset;
		}else if(reloc_info.type==ABSOLUTE_BB_PTR)
			ASSERT(0);
		else
			ASSERT(0);
	}
	// 4 finish random
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		(*ite)->finish_generate_cc();
	}
	relocation.swap(relocation_temp);
	//update cc
	code_cache->updateCC(_random_cc_size);
	is_already_finish_random = true;
	
	return ;
}

void Function::analysis_stack()
{
	if(is_already_finish_analysis_stack)
		return;
	// 1.disasm
	disassemble();
	// 2.split into bb
	split_into_basic_block();
	
	BOOL isRbpStored = false;
	BOOL isMov = false;
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		if(ite==bb_list.begin()){
			//entry block
			BOOL isRbpRestored = false;
			for(BB_INS_ITER it = (*ite)->begin(); it!=(*ite)->end(); it++){
				if((*it)->isPushRbp()){
					ASSERT(!isMov);
					ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
					isRbpStored = true;
				}else if((*it)->isMovRspRbp()){
					ASSERT(isRbpStored);
					ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP_A_4));
					isMov = true;
				}else if((*it)->isPopRbp() || (*it)->isLeave()){
					ASSERT(isMov && isRbpStored && !isRbpRestored);
					ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
					isRbpRestored = true;
				}else{
					if(isRbpStored){						
						if(isMov){
							if(isRbpRestored){
								ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
							}else
								ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
						}else{
							ASSERT(!isRbpRestored);
							ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP_A_4));
						}
					}else{
						ASSERT(!isMov && !isRbpRestored);
						ASSERT(!(*it)->isPush());
						ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
					}
				}
			}
			
			if(isRbpRestored)
				ASSERT((*ite)->is_target_empty() && (*ite)->is_fallthrough_empty());
		}else{
			BOOL isRbpRestored = false;
			for(BB_INS_ITER it = (*ite)->begin(); it!=(*ite)->end(); it++){
				if((*it)->isPopRbp() || (*it)->isLeave()){
					ASSERT(isMov && isRbpStored && !isRbpRestored);
					ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
					isRbpRestored = true;
				}else{
					if(isRbpStored){						
						if(isMov){
							if(isRbpRestored){
								ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
							}else
								ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
						}else{
							ASSERT(!isRbpRestored);
							ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP_A_4));
						}
					}else{
						ASSERT(!isMov && !isRbpRestored);
						ASSERT(!(*it)->isPush());
						ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
					}
				}
			}
			
			if(isRbpRestored)
				ASSERT((*ite)->is_target_empty() && (*ite)->is_fallthrough_empty());
		}
	}

	is_already_finish_analysis_stack = true;
}

void Function::analysis_stack_v2()
{
	if(is_already_finish_analysis_stack)
		return;
	// 1.disasm
	disassemble();
	// 2.split into bb
	split_into_basic_block();
	
	BOOL isRbpStored = false;
	BOOL isRbpRestored = false;
	BOOL isMov = false;
	BOOL need_recover = false;
	
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		if(need_recover){
			need_recover = false;
			isRbpRestored = false;
		}
		
		for(BB_INS_ITER it = (*ite)->begin(); it!=(*ite)->end(); it++){
			if((*it)->isPushRbp()){
				ASSERT(!isMov);
				ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
				isRbpStored = true;
			}else if((*it)->isMovRspRbp()){
				ASSERT(isRbpStored);
				ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP_A_4));
				isMov = true;
			}else if((*it)->isPopRbp() || (*it)->isLeave()){
				ASSERT(isMov && isRbpStored && !isRbpRestored);
				ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
				isRbpRestored = true;
			}else{
				if(isRbpStored){						
					if(isMov){
						if(isRbpRestored){
							ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
						}else
							ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RBP_A_4));
					}else{
						ASSERT(!isRbpRestored);
						ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP_A_4));
					}
				}else{
					ASSERT(!isMov && !isRbpRestored);
					ASSERT(!(*it)->isPush());//must have stack frame
					ASSERT(!(*it)->isAddRsp());
					ASSERT(!(*it)->isSubRsp());
					ShareStack::stack_map.insert(make_pair((*it)->get_inst_origin_addr(), S_RSP));
				}
			}
		}
		
		if((*ite)->is_fallthrough_empty())
			need_recover = true;
	}

	is_already_finish_analysis_stack = true;
}

void Function::flush_function_cc()
{
	//flush record in function
	_random_cc_start = 0;
	_random_cc_size = 0;
	_random_cc_origin_start = 0;
	is_already_finish_random = false;
	is_already_finish_intercept = false;
	//flush record in bb
	for(vector<BasicBlock*>::iterator iter = bb_list.begin(); iter!=bb_list.end(); iter++)
		(*iter)->flush();
	for(INT32 idx=0; idx<(INT32)entry_list.size(); idx++)
		entry_list[idx].random_entry = 0;
}

typedef struct item{
	Instruction *inst;
	Instruction *fallthroughInst;
	vector<Instruction *> targetList;
	BOOL isBBEntry;
	BOOL isBBEnd;
}Item;

BasicBlock *Function::find_bb_by_cc(ORIGIN_ADDR addr)
{
	for(vector<BasicBlock*>::iterator iter = bb_list.begin(); iter!=bb_list.end(); iter++){
		if((*iter)->is_in_bb_cc(addr))
			return *iter;
	}
	return NULL;
}

BasicBlock *Function::find_bb_by_addr(ORIGIN_ADDR addr)
{
	for(vector<BasicBlock*>::iterator iter = bb_list.begin(); iter!=bb_list.end(); iter++){
		if((*iter)->is_in_bb_addr(addr))
			return *iter;
	}
	return NULL;
}

void Function::split_into_basic_block()
{
	if(is_already_split_into_bb)
		return ;
	ASSERT(is_already_disasm);
	map<ORIGIN_ADDR, Instruction*> map_origin_addr_to_inst, temp;
	SIZE inst_sum = _origin_function_instructions.size();
	for(SIZE idx=0; idx<inst_sum; idx++){
		map_origin_addr_to_inst.insert(make_pair(_origin_function_instructions[idx]->get_inst_origin_addr(), _origin_function_instructions[idx]));
	}
	Item *array = new Item[inst_sum];
	//init
	for(SIZE k=0; k<inst_sum; k++){
		array[k].isBBEntry = false;
		array[k].isBBEnd = false;
	}
	map<Instruction *, INT32> inst_map_idx, inst_map_idx_temp;
	//BasicBlock *entryBlock = new BasicBlock();
	//scan to find target and fallthrough
	INT32 idx = 0;
	for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
		Instruction *curr_inst = *iter;
		array[idx].inst = curr_inst;
		inst_map_idx.insert(make_pair(curr_inst, idx));
		if(curr_inst->isConditionBranch()){
			//INFO("%.8lx conditionBranch!\n", curr_inst->get_inst_origin_addr());
			//add target inst
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			map<ORIGIN_ADDR, Instruction*>::iterator target_iter = map_origin_addr_to_inst.find(target_addr);
			Instruction *target_inst = target_iter==map_origin_addr_to_inst.end() ? NULL : target_iter->second;
			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else{
				if(is_in_function(target_addr)){
					//handle inst : je 1f; lock 1f: andl...
					map<ORIGIN_ADDR, Instruction*>::iterator target_iter = map_origin_addr_to_inst.find(target_addr-1);
					Instruction *target_inst = target_iter==map_origin_addr_to_inst.end() ? NULL : target_iter->second;
					ASSERT(target_inst && target_inst->get_inst_code_first_byte()==0xf0);//lock prefix
					array[idx].targetList.push_back(target_inst);
				}else
					;//ASSERTM(0, "%.8lx  find none target in condtionjmp!\n", curr_inst->get_inst_origin_addr());
			}
			array[idx].isBBEnd = true;
			//add fallthrough inst
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else{
				//PRINT("%.8lx  fallthrough inst out of function!\n", curr_inst->get_inst_origin_addr());
				array[idx].fallthroughInst = NULL;
			}
		}else if(curr_inst->isDirectJmp()){
			//INFO("%.8lx directJmp!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			map<ORIGIN_ADDR, Instruction*>::iterator target_iter = map_origin_addr_to_inst.find(target_addr);
			Instruction *target_inst = target_iter==map_origin_addr_to_inst.end() ? NULL : target_iter->second;

			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else{//TODO: PLT
			/*
				ASSERTM(((func_map->find(target_addr)) != (func_map->end())), \
					"%.8lx  target inst is empty in directjmp && target is function\n", curr_inst->get_inst_origin_addr());
			*/}
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx+1].isBBEntry = true;
			}
		}else if(curr_inst->isIndirectJmp()){
			//INFO("%.8lx indirectJmp!\n", curr_inst->get_inst_origin_addr());
			//add target
			BOOL call_out_of_func = false;
			vector<ORIGIN_ADDR> *target_addr_list = _code_segment->find_target_by_inst_addr(curr_inst->get_inst_origin_addr());
			for(vector<ORIGIN_ADDR>::iterator it = target_addr_list->begin(); it!=target_addr_list->end(); it++){
				map<ORIGIN_ADDR, Instruction*>::iterator target_iter = map_origin_addr_to_inst.find(*it);
				Instruction *target_inst = target_iter==map_origin_addr_to_inst.end() ? NULL : target_iter->second;
				if(target_inst){
					array[idx].targetList.push_back(target_inst);
				}else{
					ASSERT(!is_in_function(*it));
					call_out_of_func = true;
				}
			}
			target_addr_list->clear();
			if(call_out_of_func)
				ASSERTM(array[idx].targetList.size()==0, "jmpin inst jump in and out of function!");
			
			if(array[idx].targetList.size()==0)
				;//ASSERTM(!call_out_of_func, "%.8lx  do not find target!\n", curr_inst->get_inst_origin_addr());
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx+1].isBBEntry = true;
		}else if(curr_inst->isDirectCall()){
			//INFO("%.8lx directCall!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			map<ORIGIN_ADDR, Instruction*>::iterator target_iter = map_origin_addr_to_inst.find(target_addr);
			Instruction *target_inst = target_iter==map_origin_addr_to_inst.end() ? NULL : target_iter->second;
			ASSERT(!target_inst || (target_inst==_origin_function_instructions.front()));
			array[idx].isBBEnd = true;
			//add fallthrough
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else
				array[idx].fallthroughInst = NULL;
		}else if (curr_inst->isIndirectCall()){
			//INFO("%.8lx indirectCall!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			//add fallthrough
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else
				array[idx].fallthroughInst = NULL;
		}else if(curr_inst->isRet()){
			//INFO("%.8lx ret!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			array[idx].fallthroughInst = NULL;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx+1].isBBEntry = true;
		}else{
			//INFO("%.8lx none!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = false;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx].fallthroughInst = *(iter+1);
			else
				array[idx].fallthroughInst = NULL;
		}
		
		idx++;
	}
	
	array[0].isBBEntry = true;
	array[idx-1].isBBEnd = true;


	//calculate the BB entry
	for(idx = 0;idx<(INT32)inst_sum; idx++){
		for(vector<Instruction*>::iterator it = array[idx].targetList.begin(); it!=array[idx].targetList.end(); it++){
			INT32 targetIdx = inst_map_idx.find(*it)->second;
			//ERR("%.8lx->%.8lx\n", array[idx].inst->get_inst_origin_addr(), array[targetIdx].inst->get_inst_origin_addr());
			array[targetIdx].isBBEntry = true;
			if(targetIdx!=0 && !array[targetIdx-1].isBBEnd){
				array[targetIdx-1].isBBEnd = true;
			}
		}
	}

	for(INT32 array_idx = 0; array_idx<(INT32)inst_sum; array_idx++){
		ORIGIN_ADDR inst_addr = array[array_idx].inst->get_inst_origin_addr();
		if(is_entry(inst_addr) && !array[array_idx].isBBEntry){
			array[array_idx].isBBEntry = true;
			ASSERT(!array[array_idx-1].isBBEnd);
			array[array_idx-1].isBBEnd = true;
		}
	}


	/*
	for(SIZE i=0; i<inst_sum; i++){
		INFO("%.8lx Entry:%d End: %d\n", array[i].inst->get_inst_origin_addr(), array[i].isBBEntry, array[i].isBBEnd);
	}*/
	
	//create BasicBlock and insert the instructions
	BasicBlock *curr_bb = NULL;
	Instruction *first_inst_in_bb = NULL;
	map<Instruction *, BasicBlock *> inst_bb_map, inst_bb_map_temp;
	for(SIZE i=0; i<inst_sum; i++){
		Item *item = array+i;
		if(item->isBBEntry){
			curr_bb = new BasicBlock();
			bb_list.push_back(curr_bb);
			first_inst_in_bb = item->inst;
		}
		
		curr_bb->insert_instruction(item->inst);
		
		if(item->isBBEnd)
			inst_bb_map.insert(make_pair(first_inst_in_bb, curr_bb));
	}
	
	//add prev, target, fallthrough
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		BasicBlock *curr_bb = *ite;
		Item *item = array + inst_map_idx.find(curr_bb->get_last_instruction())->second;
		ASSERT(item->isBBEnd);
		if(item->fallthroughInst){
			BasicBlock *fallthroughBB = inst_bb_map.find(item->fallthroughInst)->second;
			curr_bb->add_fallthrough_bb(fallthroughBB);
			fallthroughBB->add_prev_bb(curr_bb);
		}
		for(vector<Instruction*>::iterator it = item->targetList.begin(); it!=item->targetList.end(); it++){
			BasicBlock *targetBB = inst_bb_map.find(*it)->second;
			curr_bb->add_target_bb(targetBB);
			targetBB->add_prev_bb(curr_bb);
		}
	}
	//free
	for(SIZE k=0; k<inst_sum; k++){
		vector<Instruction *> temp;
		array[k].targetList.swap(temp);
	}
	delete [] array;
	inst_map_idx.swap(inst_map_idx_temp);
	inst_bb_map.swap(inst_bb_map_temp);
	map_origin_addr_to_inst.swap(temp);
	
	is_already_split_into_bb = true;
}
#if 0
typedef enum{
	EMPTY_RANDOM = 0,
	FALSE_RANDOM,
	TRUE_RANDOM,
	UNKNOWN_RANDOM,
	NONE_RESULT,
	SUM_RANDOM,
}ELEMENT_TYPE;

void dump_random_matrix(ELEMENT_TYPE **random_matrix, INT32 size)
{
	for(INT32 column=0; column<size; column++){
		if(column==0)
			INFO("   ");
		INFO("%3d ", column);
	}
	INFO("\n");
	for(INT32 row=0; row<size; row++){
	 	INFO("%3d  ", row);
		for(INT32 column=0; column<size; column++){
			switch(random_matrix[row][column]){
				case EMPTY_RANDOM: PRINT("E"); break;
				case FALSE_RANDOM: PRINT(COLOR_RED"F"COLOR_END); break;
				case TRUE_RANDOM: PRINT(COLOR_BLUE"T"COLOR_END); break;
				case UNKNOWN_RANDOM: PRINT(COLOR_YELLOW"U"COLOR_END); break;
				case NONE_RESULT: PRINT(COLOR_GREEN"N"COLOR_END);break;
				default: ASSERT(0);
			}
			PRINT("   ");
		}
		PRINT("\n");
	 }
}
typedef multimap<INT32, INT32> ROW_MAP;
typedef multimap<INT32, INT32>::iterator ROW_MAP_ITER;
typedef pair<ROW_MAP_ITER, ROW_MAP_ITER> ROW_MAP_RANGE;

void solution_random_matrix(ELEMENT_TYPE **matrix, INT32 size, ROW_MAP unknown_row_map, INT32 *unknown_column_num,
	INT32 *false_column_num, INT32 *true_column_num)
{
	INT32 unknown_num = unknown_row_map.size();
	while(1){
		//calculate diagonal value
		for(INT32 idx=0; idx<size; idx++){
			ELEMENT_TYPE current_res  =matrix[idx][idx];
			if(current_res==NONE_RESULT){
				if(idx==0)//entry basic block
					matrix[idx][idx] = FALSE_RANDOM;
				else{
					if(false_column_num[idx]==0){
						if(unknown_column_num[idx]==0){
							if(true_column_num[idx]!=0);
								matrix[idx][idx] = TRUE_RANDOM;
						}
					}else
						matrix[idx][idx] = FALSE_RANDOM;
				}
				if(matrix[idx][idx]!=NONE_RESULT){
					//scan row to change UNKNOW_RANDOM to current_res
					ROW_MAP_RANGE range = unknown_row_map.equal_range(idx);
					for(ROW_MAP_ITER iter = range.first; iter!=range.second; iter++){
						INT32 row_idx = iter->first;
						ASSERT(row_idx == idx);
						INT32 column_idx = iter->second;
						//set column num
						if(matrix[idx][idx]==FALSE_RANDOM){
							false_column_num[column_idx]++;
							matrix[row_idx][column_idx] = FALSE_RANDOM;
						}else if(matrix[idx][idx]==TRUE_RANDOM){
							true_column_num[column_idx]++;
							matrix[row_idx][column_idx] = TRUE_RANDOM;
						}else
							ASSERT(0);
						unknown_column_num[column_idx]--;
					}
					unknown_row_map.erase(idx);		
				}
			}
		}
		INT32 unknown_num_iterator = unknown_row_map.size();
		if(unknown_num_iterator==unknown_num){
			if(unknown_num_iterator==0)
				break;
			else{//search one loop
				BOOL find_loop = false;
				for(INT32 idx=0; idx<size; idx++){
					if((matrix[idx][idx]==NONE_RESULT)&&(unknown_row_map.find(idx)!=unknown_row_map.end())
						&&(unknown_column_num[idx]!=0) &&(false_column_num[idx]==0) &&(true_column_num[idx]!=0)){
						matrix[idx][idx] = TRUE_RANDOM;
						//scan row to change UNKNOW_RANDOM to TRUE_RANDOM
						ROW_MAP_RANGE range = unknown_row_map.equal_range(idx);
						for(ROW_MAP_ITER iter = range.first; iter!=range.second; iter++){
							INT32 row_idx = iter->first;
							ASSERT(row_idx == idx);
							INT32 column_idx = iter->second;
							//set column num
							true_column_num[column_idx]++;
							matrix[row_idx][column_idx] = TRUE_RANDOM;
							unknown_column_num[column_idx]--;
						}
						unknown_row_map.erase(idx);	
						find_loop=true;
						break;
					}
				}
				ASSERT(find_loop);
			}
		}else{
			ASSERT(unknown_num_iterator<unknown_num);
			unknown_num = unknown_num_iterator;
		}
	}
}

void Function::analyse_random_bb()
{
	if(is_already_random_analysis)return;
	ASSERT(is_already_split_into_bb);
	// 1.initialize the bb map idx;
	INT32 bb_num = bb_list.size();
	map<BasicBlock*, INT32> bb_list_map_idx;
	for(INT32 idx=0; idx<bb_num; idx++)
		bb_list_map_idx.insert(make_pair(bb_list[idx], idx));
	// 2.initialize the matrix and multimap
	ROW_MAP unknown_row_map;
	INT32 *false_column_num = new INT32[bb_num]();
	INT32 *true_column_num = new INT32[bb_num]();
	INT32 *unknown_column_num = new INT32[bb_num]();
	ELEMENT_TYPE **random_matrix = new ELEMENT_TYPE*[bb_num];
	for(INT32 row=0; row<bb_num; row++){
		random_matrix[row] = new ELEMENT_TYPE[bb_num];
		// 2.1 give a initialized num
		for(INT32 column=0; column<bb_num; column++)
			random_matrix[row][column] = row==column ? NONE_RESULT : EMPTY_RANDOM;
		// 2.2 calculate the succ basicblock
		BasicBlock *src_bb = bb_list[row];
		BOOL is_call_bb = src_bb->is_call_bb();
		if(is_call_bb){	
			ASSERT(src_bb->is_target_empty());
			if(!src_bb->is_fallthrough_empty()){
				INT32 dest_bb_idx = bb_list_map_idx.find(src_bb->get_fallthrough_bb())->second;
				ASSERT(dest_bb_idx!=row);
				random_matrix[row][dest_bb_idx] = FALSE_RANDOM;
				//set record map
				false_column_num[dest_bb_idx]++;
			}
		}else{
			ELEMENT_TYPE succ_bb_type = src_bb->find_first_least_size_instruction(5) == src_bb->end() ? UNKNOWN_RANDOM : TRUE_RANDOM;
			//set target bb's matrix type
			for(BB_ITER iter = src_bb->target_begin(); iter!=src_bb->target_end(); iter++){
				INT32 dest_succ_idx = bb_list_map_idx.find(*iter)->second;
				if(dest_succ_idx!=row){//used for handling self loop
					random_matrix[row][dest_succ_idx] = succ_bb_type;
					//set record num
					if(succ_bb_type==UNKNOWN_RANDOM){
						unknown_column_num[dest_succ_idx]++;
						unknown_row_map.insert(make_pair(row, dest_succ_idx));
					}else
						true_column_num[dest_succ_idx]++;
				}
			}
			//set fallthrough 
			if(src_bb->get_fallthrough_bb()){
				INT32 dest_succ_idx = bb_list_map_idx.find(src_bb->get_fallthrough_bb())->second;
				ASSERT(dest_succ_idx!=row);
				random_matrix[row][dest_succ_idx] = succ_bb_type;
				//set record num
				if(succ_bb_type==UNKNOWN_RANDOM){
					unknown_column_num[dest_succ_idx]++;
					unknown_row_map.insert(make_pair(row, dest_succ_idx));
				}else
					true_column_num[dest_succ_idx]++;
			}
		}
	}
	//INFO("====INIT matrix====\n");
	//dump_random_matrix(random_matrix, bb_num);
	// 3.solution the matrix
	solution_random_matrix(random_matrix, bb_num, unknown_row_map, unknown_column_num, false_column_num, true_column_num);
	// 4.set the basicblock random type
	for(INT32 idx=0; idx<bb_num; idx++){
		BasicBlock *curr_bb = bb_list[idx];
		switch(random_matrix[idx][idx]){
			case FALSE_RANDOM:
				curr_bb->set_random_type(false);
				break;
			case TRUE_RANDOM:
				curr_bb->set_random_type(true);
				break;
			default:
				ASSERT(0);
		}
		curr_bb->finish_analyse();
	}
	// 5.set already analysis
	is_already_random_analysis = true;
	//INFO("====FINI matrix====\n");
	//dump_random_matrix(random_matrix, bb_num);
	// 6.free heap data
	delete [] false_column_num;
	delete [] true_column_num;
	delete [] unknown_column_num;
	for(INT32 idx=0; idx<bb_num; idx++)
		delete [] random_matrix[idx];	
}
#endif
