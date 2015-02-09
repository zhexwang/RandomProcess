#include "function.h"


Function::Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size)
	:_code_segment(code_segment), _function_name(name), _origin_function_base(origin_function_base), _origin_function_size(origin_function_size)
	, _random_cc_start(0), _random_cc_origin_start(0), _random_cc_size(0), is_already_disasm(false), is_already_split_into_bb(false)
{
	_function_base = code_segment->convert_origin_process_addr_to_this(_origin_function_base);
	_function_size = (SIZE)_origin_function_size;
}
Function::~Function(){
	;
}
void Function::dump_function_origin()
{
	PRINT("[0x%lx-0x%lx]",  _origin_function_base, _origin_function_size+_origin_function_base);
	INFO("%s",_function_name.c_str());
	PRINT("(Path:%s)\n", _code_segment->file_path.c_str());
	if(is_already_disasm){
		for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
			(*iter)->dump();
		}
	}else
		ERR("Do not disasm!\n");
}
void Function::dump_bb_origin()
{
	PRINT("[0x%lx-0x%lx]",  _origin_function_base, _origin_function_size+_origin_function_base);
	INFO("%s",_function_name.c_str());
	PRINT("(Path:%s)\n", _code_segment->file_path.c_str());
	if(is_already_split_into_bb){
		for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
			(*ite)->dump();
		}
	}else
		ERR("Do not split into BB!\n");
}
void Function::disassemble()
{
	ORIGIN_ADDR origin_addr = _origin_function_base;
	ADDR current_addr = _function_base;
	SIZE security_size = _origin_function_size+1;//To see one more byte

	while(security_size>1){
		Instruction *instr = new Instruction(origin_addr, current_addr, security_size);
		_map_origin_addr_to_inst.insert(make_pair(origin_addr, instr));
		SIZE instr_size = instr->disassemable();
		origin_addr += instr_size;
		current_addr += instr_size;
		security_size -= instr_size;
		_origin_function_instructions.push_back(instr);
	}
	ASSERT(security_size==1);
	is_already_disasm = true;
}

void Function::point_to_random_function()
{
	NOT_IMPLEMENTED(wz);
}

SIZE Function::random_function(CODE_CACHE_ADDR cc_curr_addr, ORIGIN_ADDR cc_origin_addr)
{
	ASSERT(is_already_split_into_bb);

	SIZE bb_copy_size = 0;
	_random_cc_start = cc_curr_addr;
	_random_cc_origin_start = cc_origin_addr;
	_random_cc_size = 0;
	for(vector<BasicBlock *>::iterator ite = bb_list.begin(); ite!=bb_list.end(); ite++){
		cc_curr_addr += bb_copy_size;
		cc_origin_addr += bb_copy_size;
		bb_copy_size = (*ite)->copy_instructions(cc_curr_addr, cc_origin_addr);
		_random_cc_size += bb_copy_size;
	}
	return _random_cc_size;
}

Instruction *Function::get_instruction_by_addr(ORIGIN_ADDR origin_addr)
{
	ASSERT(is_already_disasm);
	map<ORIGIN_ADDR, Instruction*>::iterator iter = _map_origin_addr_to_inst.find(origin_addr);
	if(iter==_map_origin_addr_to_inst.end())
		return NULL;
	else
		return iter->second;
}

typedef struct item{
	Instruction *inst;
	Instruction *fallthroughInst;
	vector<Instruction *> targetList;
	BOOL isBBEntry;
	BOOL isBBEnd;
}Item;

void Function::split_into_basic_block()
{
	ASSERT(is_already_disasm);
	SIZE inst_sum = _origin_function_instructions.size();
	Item *array = new Item[inst_sum];
	//init
	for(SIZE k=0; k<inst_sum; k++){
		array[k].isBBEntry = false;
		array[k].isBBEnd = false;
	}
	map<Instruction *, INT32> inst_map_idx;
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
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else
				ERR("%.8lx  find none target in condtionjmp!\n", curr_inst->get_inst_origin_addr());
			array[idx].isBBEnd = true;
			//add fallthrough inst
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx].fallthroughInst = *(iter+1);
				array[idx+1].isBBEntry = true;
			}else{
				ERR("%.8lx  fallthrough inst out of function!\n", curr_inst->get_inst_origin_addr());
				array[idx].fallthroughInst = NULL;
			}
		}else if(curr_inst->isDirectJmp()){
			//INFO("%.8lx directJmp!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			if(target_inst)
				array[idx].targetList.push_back(target_inst);
			else
				ERR("%.8lx  target inst is empty in directjmp\n", curr_inst->get_inst_origin_addr());
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end()){
				array[idx+1].isBBEntry = true;
			}
		}else if(curr_inst->isIndirectJmp()){
			//INFO("%.8lx indirectJmp!\n", curr_inst->get_inst_origin_addr() - _code_segment->code_start);
			//add target
			vector<ORIGIN_ADDR> *target_addr_list = _code_segment->find_target_by_inst_addr(curr_inst->get_inst_origin_addr());
			for(vector<ORIGIN_ADDR>::iterator it = target_addr_list->begin(); it!=target_addr_list->end(); it++){
				Instruction *target_inst = get_instruction_by_addr(*it);
				if(target_inst){
					array[idx].targetList.push_back(target_inst);
				}else
					ERR("%.8lx  target inst is out of function!\n", curr_inst->get_inst_origin_addr());
			}
			if(array[idx].targetList.size()==0)
				ERR("%.8lx  do not find target!\n", curr_inst->get_inst_origin_addr());
			array[idx].fallthroughInst = NULL;
			array[idx].isBBEnd = true;
			if((iter+1)!=_origin_function_instructions.end())
				array[idx+1].isBBEntry = true;
		}else if(curr_inst->isDirectCall()){
			//INFO("%.8lx directCall!\n", curr_inst->get_inst_origin_addr());
			//add target
			ORIGIN_ADDR target_addr = curr_inst->getBranchTargetOrigin();
			Instruction *target_inst = get_instruction_by_addr(target_addr);
			ASSERT(!target_inst);
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
	}/*
	for(SIZE i=0; i<inst_sum; i++){
		INFO("%.8lx Entry:%d End: %d\n", array[i].inst->get_inst_origin_addr(), array[i].isBBEntry, array[i].isBBEnd);
	}*/
	
	//create BasicBlock and insert the instructions
	BasicBlock *curr_bb = NULL;
	Instruction *first_inst_in_bb = NULL;
	map<Instruction *, BasicBlock *> inst_bb_map;
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
	is_already_split_into_bb = true;
}
