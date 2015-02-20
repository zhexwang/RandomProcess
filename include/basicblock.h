#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "instruction.h"
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
	SIZE _generate_copy_size;
	//used for random
	BOOL is_already_finish_analyse;
	BOOL entry_can_be_random;
	BB_INS_ITER first_random_inst;//when entry can not be random, it may be partly random. So find out the first random inst
public:
	BasicBlock():fallthrough_bb(NULL), _curr_copy_addr(0), _origin_copy_addr(0), _generate_copy_size(0)
		, is_already_finish_analyse(false), entry_can_be_random(false), first_random_inst(instruction_vec.end())
	{
		;		
	}
	virtual ~BasicBlock()
	{
		;
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
	void finish_random_analyse()
	{
		is_already_finish_analyse = true;
	}
	BOOL can_be_random()
	{
		ASSERT(is_already_finish_analyse);
		return entry_can_be_random;
	}
	void set_random_type(BOOL _can_be_random, BB_INS_ITER random_begin_inst)
	{
		ASSERT(!is_already_finish_analyse);
		ASSERT(_can_be_random && (random_begin_inst==end()));
		entry_can_be_random = _can_be_random;	
		first_random_inst = random_begin_inst;
	}
	BOOL finish_analyse()
	{
		ASSERT(!is_already_finish_analyse);
		return is_already_finish_analyse;
	}
	SIZE copy_instructions(CODE_CACHE_ADDR curr_target_addr, ORIGIN_ADDR origin_target_addr);
	BB_INS_ITER find_first_least_size_instruction(SIZE least_size);
	void dump();
};

#endif
