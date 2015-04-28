#include "stack.h"#include "codecache.h"#include <unistd.h>#include "function.h"#include "basicblock.h"extern CodeCacheManagement *cc_management;map<ORIGIN_ADDR, STACK_TYPE> ShareStack::stack_map;map<ORIGIN_ADDR, LIB_STACK_TYPE> ShareStack::lib_stack_map;map<string, LIB_STACK_TYPE> ShareStack::unused_func_map;extern Function *find_function_by_addr(ORIGIN_ADDR addr);extern CodeSegment *find_cs(ORIGIN_ADDR addr);map<ORIGIN_ADDR, LIB_STACK_TYPE>::iterator find_lib_stack_inst(ORIGIN_ADDR origin_addr, BOOL is_in_cc){	if(!is_in_cc)		return ShareStack::lib_stack_map.find(origin_addr);	else{		Function *func = find_function_by_addr(origin_addr);		BasicBlock *bb = func->find_bb_by_addr(origin_addr);		if(bb->get_last_instruction()->isSyscall() && bb->get_last_instruction()->get_inst_origin_addr()==origin_addr)			return ShareStack::lib_stack_map.find(origin_addr+bb->get_last_instruction()->get_inst_size());//handle syscall is the bb last instruction		else			return ShareStack::lib_stack_map.find(origin_addr);	}}BOOL ShareStack::check_relocate(MapInst *map_inst){	ASSERT(cc_management);	if(info->process_id==0)		return true;		struct ucontext *uc = (struct ucontext*)get_curr_addr((ORIGIN_ADDR)(info->origin_uc));	ORIGIN_ADDR pc = uc->uc_mcontext.gregs[REG_RIP];	ORIGIN_ADDR origin_addr = map_inst->get_old_origin_addr(pc, cc_management->is_in_cc(pc));	map<ORIGIN_ADDR, STACK_TYPE>::iterator ret = stack_map.find(origin_addr);	if(ret!=stack_map.end())		return true;		map<ORIGIN_ADDR, LIB_STACK_TYPE>::iterator lib_ret = find_lib_stack_inst(origin_addr, cc_management->is_in_cc(pc));	if(lib_ret!=lib_stack_map.end()){		Function *func = find_function_by_addr(origin_addr);		ASSERT(func);		ORIGIN_ADDR off = origin_addr - func->_code_segment->code_start;				if(lib_ret->second.reg!=STACK_NONE)			return true;		else{			ERR("find syscall unhandled, 0x%lx(%s)!\n", off, func->_code_segment->file_path.c_str());			ASSERT(0);			return false;		}	}/*	Function *func = find_function_by_addr(origin_addr);	ASSERT(func);	ORIGIN_ADDR off = origin_addr - func->_code_segment->code_start;	ERR("unkown pc= 0x%lx, %s\n", off, func->_code_segment->file_path.c_str());*/	return false;}void ShareStack::dump_stack_type_by_origin_addr(ORIGIN_ADDR addr){		static string stack_name[] = {"S_RBP_A_4", "S_RSP", "S_RSP_A_4"};	map<ORIGIN_ADDR, STACK_TYPE>::iterator ret = stack_map.find(addr);	if(ret==stack_map.end())		INFO("NONE\n");	else		INFO("%s\n", stack_name[ret->second].c_str());}BOOL ShareStack::return_address_in_unused_rbp_function(MapInst *map_inst, 	ORIGIN_ADDR return_address, ORIGIN_ADDR &origin_rsp){	BOOL is_in_cc = cc_management->is_in_cc(return_address);	ORIGIN_ADDR origin_addr = map_inst->get_old_origin_addr(return_address, is_in_cc);	Function *func = find_function_by_addr(origin_addr);	if(!func)		return false;	map<string, LIB_STACK_TYPE>::iterator ret = ShareStack::unused_func_map.find(func->get_function_name());	if(ret!=ShareStack::unused_func_map.end()){		ASSERT(ret->second.reg==STACK_RSP);		origin_rsp += ret->second.op;		return true;	}else		return false;}void ShareStack::relocate_return_address(MapInst *map_inst){	ASSERT(cc_management);	if(info->process_id==0)		return ;		struct ucontext *uc = (struct ucontext*)get_curr_addr((ORIGIN_ADDR)(info->origin_uc));	ORIGIN_ADDR pc = uc->uc_mcontext.gregs[REG_RIP];	ORIGIN_ADDR origin_addr = map_inst->get_old_origin_addr(pc, cc_management->is_in_cc(pc));	map<ORIGIN_ADDR, STACK_TYPE>::iterator ret = stack_map.find(origin_addr);		STACK_TYPE type;	if(ret!=stack_map.end())	 	type = ret->second;	else		type = S_NONE;	ORIGIN_ADDR origin_rbp = uc->uc_mcontext.gregs[REG_RBP];	ORIGIN_ADDR origin_rsp = uc->uc_mcontext.gregs[REG_RSP];	//caculate current stack frame	ADDR curr_rbp = get_curr_addr(origin_rbp);	ADDR curr_rsp = get_curr_addr(origin_rsp);	ORIGIN_ADDR return_address = 0;	ORIGIN_ADDR new_ret_addr = 0;		switch(type){		case S_RSP:{			return_address = *(ADDR*)(curr_rsp);			new_ret_addr = map_inst->get_new_addr_from_old(return_address, \				cc_management->is_in_cc(return_address));			*(ADDR*)curr_rsp = new_ret_addr;			origin_rsp += 8;			}			break;		case S_RSP_A_4:{			return_address = *((ADDR*)curr_rsp + 1);			new_ret_addr = map_inst->get_new_addr_from_old(return_address, \				cc_management->is_in_cc(return_address));			*((ADDR*)curr_rsp + 1) = new_ret_addr;			origin_rsp += 16;			}			break;		case S_RBP_A_4:{			return_address = *((ADDR*)curr_rbp + 1);			new_ret_addr = map_inst->get_new_addr_from_old(return_address, \				cc_management->is_in_cc(return_address));			*((ADDR*)curr_rbp + 1) = new_ret_addr;			origin_rsp = origin_rbp+16;			origin_rbp = *(ADDR*)curr_rbp;			}			break;		default:{  			map<ORIGIN_ADDR, LIB_STACK_TYPE>::iterator lib_ret = find_lib_stack_inst(origin_addr, cc_management->is_in_cc(pc));			ASSERT(lib_ret!=lib_stack_map.end());			STACK_REG reg = lib_ret->second.reg;			INT8 op = lib_ret->second.op;			if(reg==STACK_RSP){				return_address = *(ADDR*)(curr_rsp + op);				new_ret_addr = map_inst->get_new_addr_from_old(return_address, \				cc_management->is_in_cc(return_address));				*(ADDR*)(curr_rsp + op) = new_ret_addr;					origin_rsp += (op+8);			}else if(reg==STACK_RBP){				return_address = *(ADDR*)(curr_rbp + op);				new_ret_addr = map_inst->get_new_addr_from_old(return_address, \					cc_management->is_in_cc(return_address));				*(ADDR*)(curr_rbp + op) = new_ret_addr;				origin_rsp += (origin_rbp+op+8);				origin_rbp = *(ADDR*)curr_rbp;			}else				ASSERT(0);		}	}	PRINT(COLOR_RED"            [%5d STACK MIGRATION]: "COLOR_END"old_return_address(0x%lx)--->new_return_address(0x%lx)\n", \		info->process_id, return_address, new_ret_addr);		//unwind main executable's stack frame	while(origin_rbp!=0){		if(return_address_in_unused_rbp_function(map_inst, return_address, origin_rsp)){//do not use rbp function			curr_rsp = get_curr_addr(origin_rsp);			return_address = *(ADDR*)curr_rsp;			new_ret_addr = map_inst->get_new_addr_from_old(return_address, cc_management->is_in_cc(return_address));			*(ADDR*)curr_rsp = new_ret_addr;			origin_rsp += 8;		}else{//use rbp function			curr_rbp = get_curr_addr(origin_rbp);			//get return address			return_address = *((ADDR*)curr_rbp + 1);			//update return address			new_ret_addr = map_inst->get_new_addr_from_old(return_address, cc_management->is_in_cc(return_address));			*((ADDR*)curr_rbp + 1) = new_ret_addr;			origin_rsp = origin_rbp + 16;			//prev rbp			origin_rbp = *(ADDR *)curr_rbp;		}		PRINT(COLOR_RED"            [%5d STACK MIGRATION]: "COLOR_END"old_return_address(0x%lx)--->new_return_address(0x%lx)\n", \			info->process_id, return_address, new_ret_addr);	}	return ;}
void ShareStack::relocate_current_pc(MapInst *map_inst){
	ASSERT(cc_management);	if(info->process_id==0)		return ;	struct ucontext *uc = (struct ucontext*)get_curr_addr((ORIGIN_ADDR)(info->origin_uc));	ORIGIN_ADDR pc = uc->uc_mcontext.gregs[REG_RIP];	ORIGIN_ADDR new_pc = map_inst->get_new_addr_from_old(pc, cc_management->is_in_cc(pc));	//update old pc	uc->uc_mcontext.gregs[REG_RIP] = new_pc;	PRINT(COLOR_YELLOW"            [%5d   PC  MIGRATION]: "COLOR_END"old_pc(0x%lx)--->new_pc(0x%lx)\n",info->process_id, pc, new_pc);}