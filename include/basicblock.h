#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "instruction.h"
#include "relocation.h"
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
	BasicBlock *fallthrough_bb;
	vector<BasicBlock *> target_bb_vec;
	CODE_CACHE_ADDR _curr_copy_addr;
	ORIGIN_ADDR _origin_copy_addr;
	SIZE _generate_cc_size;
	ORIGIN_ADDR _origin_entry_addr_after_random;
	//used for random
	BOOL is_already_finish_analyse;
	BOOL entry_can_be_random;
	BB_INS_ITER random_inst_start;
	BB_INS_ITER random_inst_end;
	BOOL random_last_inst_need_jump_back;
	BOOL is_already_finish_inst_analyse;
	BOOL is_finish_generate_cc;
	BOOL _is_self_loop;
public:
	BasicBlock():fallthrough_bb(NULL), _curr_copy_addr(0), _origin_copy_addr(0), _generate_cc_size(0), _origin_entry_addr_after_random(0)
		, is_already_finish_analyse(false), entry_can_be_random(false), is_already_finish_inst_analyse(false), is_finish_generate_cc(false), _is_self_loop(false)
	{
		;		
	}
	virtual ~BasicBlock()
	{
		;
	}
	ORIGIN_ADDR get_origin_addr_before_random()
	{
		return instruction_vec.front()->get_inst_origin_addr();
	}
	ORIGIN_ADDR get_origin_addr_after_random()
	{
		ASSERT(_origin_entry_addr_after_random!=0);
		return _origin_entry_addr_after_random;
	}
	ORIGIN_ADDR modify_inst_origin_addr()
	{
		ASSERT(!instruction_vec.empty() && is_already_finish_analyse);
		if(random_inst_start!=instruction_vec.end()){
			if(!entry_can_be_random)
				return (*random_inst_start)->get_inst_origin_addr();
			else
				return 0;
		}else
			return -1;
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
		if(target_bb==this)
			_is_self_loop = true;
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
	BOOL is_self_loop()
	{
		return _is_self_loop;
	}
	BOOL can_be_random()
	{
		ASSERT(is_already_finish_analyse);
		return entry_can_be_random;
	}
	void set_random_type(BOOL _entry_can_be_random)
	{
		ASSERT(!is_already_finish_analyse);
		entry_can_be_random = _entry_can_be_random;	
	}
	void finish_analyse()
	{
		ASSERT(!is_already_finish_analyse);
		is_already_finish_analyse = true;
	}
	void finish_generate_cc()
	{
		ASSERT(!is_finish_generate_cc);
		is_finish_generate_cc = true;
	}
	void flush_generate_cc()
	{
		ASSERT(is_finish_generate_cc);
		_origin_entry_addr_after_random = 0;
		_curr_copy_addr = 0;
		_origin_copy_addr = 0;
		_generate_cc_size = 0;
		is_finish_generate_cc = false;
	}
	void intercept_jump();
	void random_analysis();
	SIZE random_unordinary_inst(Instruction *inst, CODE_CACHE_ADDR curr_target_addr, 
		ORIGIN_ADDR origin_target_addr, vector<RELOCATION_ITEM> &relocation);
	SIZE copy_random_insts(CODE_CACHE_ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr, 
		vector<RELOCATION_ITEM> &relocation);
	BB_INS_ITER find_first_least_size_instruction(SIZE least_size);
	void dump();
};

#endif
