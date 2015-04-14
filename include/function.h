#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "type.h"
#include "utility.h"
#include "instruction.h"
#include "Basicblock.h"
#include "code_segment.h"
#include "map_function.h"
#include "codecache.h"
#include "map_inst.h"
#include <vector>
#include <map>
using namespace std;

class Function
{
private:
	typedef struct Entry{
		ORIGIN_ADDR origin_entry;
		CODE_CACHE_ADDR random_entry;
	}ENTRY_ITEM;
	CodeSegment *_code_segment;
	string _function_name;
	//function in other process
	ORIGIN_ADDR _origin_function_base;
	//function in share code region
	ADDR _function_base;
	SIZE _function_size;
	//random function in code_cache
	CODE_CACHE_ADDR _random_cc_start;
	ORIGIN_ADDR _random_cc_origin_start;
	CODE_CACHE_SIZE _random_cc_size;
	//codecache
	CodeCache *code_cache;
	//instruction list
	vector<Instruction*> _origin_function_instructions;
	BOOL is_already_disasm;
	BOOL is_already_split_into_bb;
	vector<BasicBlock*> bb_list;
	vector<ENTRY_ITEM> entry_list;
	//random
	BOOL is_already_finish_random;
	BOOL is_already_finish_intercept;
	BOOL is_already_finish_erase;
	BOOL is_function_can_be_random;
	//rbp analysis
	BOOL is_already_finish_analysis_stack;
public:
	Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size, CodeCache *cc);
	virtual ~Function();
	string get_function_name(){return _function_name;}
	void dump_function_origin();
	void dump_bb_origin();
	void dump_function_name()
	{
		PRINT("(0x%lx)%s\n", _origin_function_base, _function_name.c_str());

	}
	BOOL is_in_function(ORIGIN_ADDR addr)
	{
		if(addr>=_origin_function_base && addr<(_origin_function_base+_function_size))
			return true;
		else
			return false;
	}
	BOOL is_in_function_cc(ORIGIN_ADDR addr)
	{
		if(addr>=_random_cc_origin_start && addr<(_random_cc_origin_start+_random_cc_size))
			return true;
		else
			return false;
	}
	BasicBlock *find_bb_by_cc(ORIGIN_ADDR addr);
	BOOL add_function_entry(ORIGIN_ADDR entry_addr)
	{
		ASSERT(is_already_disasm);
		for(INT32 idx = 0; idx<(INT32)entry_list.size(); idx++){
			if(entry_list[idx].origin_entry==entry_addr)
				return false;
		}
		entry_list.push_back((ENTRY_ITEM){entry_addr, 0});
		return true;
	}
	BOOL is_entry(ORIGIN_ADDR entry_addr)
	{
		for(INT32 idx = 0; idx<(INT32)entry_list.size(); idx++){
			if(entry_list[idx].origin_entry==entry_addr)
				return true;
		}
		return false;
	}
	ADDR get_function_base(){return _function_base;}
	INT32 get_function_size(){return _function_size;}
	SIZE get_inst_num(){return _origin_function_instructions.size();}
	ORIGIN_ADDR get_origin_function_base(){return _origin_function_base;}
	void intercept_to_random_function();
	void disassemble();
	void split_into_basic_block();
	void random_function(multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, 
		map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin);
	void analysis_stack(map<ORIGIN_ADDR, STACK_TYPE> stack_map);
	BOOL check_random();
	void flush_function_cc();
	void erase_function();
};

#endif
