#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "instruction.h"
#include "relocation.h"
#include "codecache.h"
#include <vector>
using namespace std;

class BasicBlock;
typedef vector<Instruction*>::iterator BB_INS_ITER;
typedef vector<BasicBlock*>::iterator BB_ITER;

class BasicBlock
{
private:
	vector<Instruction *> instruction_vec;
	vector<BasicBlock *> prev_bb_vec;
	vector<BasicBlock *> target_bb_vec;
	BasicBlock *fallthrough_bb;
	CODE_CACHE_ADDR _curr_copy_addr;
	ORIGIN_ADDR _origin_copy_addr;
	UINT32 _generate_cc_size;
	//used for random
	BOOL is_finish_generate_cc;
public:
	BasicBlock():fallthrough_bb(NULL), _curr_copy_addr(0), _origin_copy_addr(0), _generate_cc_size(0)
		, is_finish_generate_cc(false)
	{
		;		
	}
	virtual ~BasicBlock()
	{
		;
	}
	BOOL is_in_bb_cc(ORIGIN_ADDR addr)
	{
		if(addr>=_origin_copy_addr && addr<(_origin_copy_addr+_generate_cc_size))
			return true;
		else
			return false;
	}
	BOOL is_in_bb_addr(ORIGIN_ADDR addr)
	{
		for(vector<Instruction*>::iterator iter = instruction_vec.begin();  iter!=end(); iter++){
			if(addr==(*iter)->get_inst_origin_addr())
				return true;			
		}
		return false;
	}
	ORIGIN_ADDR get_origin_addr_before_random()
	{
		return instruction_vec.front()->get_inst_origin_addr();
	}
	ORIGIN_ADDR get_origin_addr_after_random()
	{
		ASSERT(_origin_copy_addr!=0);
		return _origin_copy_addr;
	}
	INT32 get_insts_num()
	{
		return instruction_vec.size();
	}
	void insert_instruction(Instruction *inst)
	{
		instruction_vec.push_back(inst);
	}
	Instruction *get_first_instruction()
	{
		ASSERT(!instruction_vec.empty());
		return instruction_vec.front();
	}
	Instruction *get_last_instruction()
	{
		ASSERT(!instruction_vec.empty());
		return instruction_vec.back();
	}
	BB_INS_ITER begin()
	{
		return instruction_vec.begin();
	}
	BB_INS_ITER end()
	{
		return instruction_vec.end();
	}
	
	BB_ITER prev_begin()
	{
		return prev_bb_vec.begin();
	}
	BB_ITER prev_end()
	{
		return prev_bb_vec.end();
	}
	
	BB_ITER target_begin()
	{
		return target_bb_vec.begin();
	}
	BB_ITER target_end()
	{
		return target_bb_vec.end();
	}
	BOOL is_target_empty()
	{
		return target_bb_vec.empty();
	}
	BOOL is_fallthrough_empty()
	{
		return fallthrough_bb ? false:true;
	}
	BasicBlock *get_fallthrough_bb()
	{
		return fallthrough_bb;
	}
	void add_prev_bb(BasicBlock *prev_bb)
	{
		prev_bb_vec.push_back(prev_bb);
	}
	void add_target_bb(BasicBlock *target_bb)
	{
		target_bb_vec.push_back(target_bb);
	}
	void add_fallthrough_bb(BasicBlock *fall_bb)
	{
		fallthrough_bb = fall_bb;
	}
	BOOL is_call_bb()
	{
		return get_last_instruction()->isCall();
	}
	BOOL is_ret_bb()
	{
		return get_last_instruction()->isRet();
	}
	void finish_generate_cc()
	{
		ASSERT(!is_finish_generate_cc);
		is_finish_generate_cc = true;
	}
	void flush()
	{
		_curr_copy_addr = 0;
		_origin_copy_addr = 0;
		_generate_cc_size = 0;
		is_finish_generate_cc = false;
	}
	
	SIZE random_unordinary_inst(Instruction *inst, CODE_CACHE_ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, 
		vector<RELOCATION_ITEM> &relocation, multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, 
		map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin);
	SIZE copy_random_insts(CODE_CACHE_ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation, 
		multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin);
	BB_INS_ITER find_first_least_size_instruction(SIZE least_size);
	void dump();
};

#endif
