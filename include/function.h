#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "type.h"
#include "utility.h"
#include "instruction.h"
#include "Basicblock.h"
#include "code_segment.h"
#include <vector>
#include <map>
using namespace std;


class Function
{
private:
	CodeSegment *_code_segment;
	string _function_name;
	//function in other process
	ORIGIN_ADDR _origin_function_base;
	ORIGIN_SIZE _origin_function_size;
	//function in share code region
	ADDR _function_base;
	SIZE _function_size;
	//random function in code_cache
	CODE_CACHE_ADDR _random_cc_start;
	ORIGIN_ADDR _random_cc_origin_start;
	CODE_CACHE_SIZE _random_cc_size;
	//instruction list
	vector<Instruction*> _origin_function_instructions;
	vector<Instruction*> _random_function_instructions;
	map<ORIGIN_ADDR, Instruction*> _map_origin_addr_to_inst;
	BOOL is_already_disasm;
	BOOL is_already_split_into_bb;
	vector<BasicBlock*> bb_list;
public:
	Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size);
	virtual ~Function();
	void dump_function_origin();
	void dump_bb_origin();
	ADDR get_function_base(){return _function_base;}
	ORIGIN_ADDR get_origin_function_base(){return _origin_function_base;}
	void point_to_random_function();
	void disassemble();
	void split_into_basic_block();
	Instruction *get_instruction_by_addr(ORIGIN_ADDR origin_addr);
	SIZE random_function(CODE_CACHE_ADDR cc_curr_addr, ORIGIN_ADDR cc_origin_addr);
};

#endif
