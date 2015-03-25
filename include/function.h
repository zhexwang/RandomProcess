#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "type.h"
#include "utility.h"
#include "instruction.h"
#include "Basicblock.h"
#include "code_segment.h"
#include "map_function.h"
#include "codecache.h"
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
	//codecache
	CodeCache *code_cache;
	//instruction list
	vector<Instruction*> _origin_function_instructions;
	vector<Instruction*> _random_function_instructions;
	map<ORIGIN_ADDR, Instruction*> _map_origin_addr_to_inst;
	BOOL is_already_disasm;
	BOOL is_already_split_into_bb;
	BOOL is_already_finish_random;
	BOOL is_already_finish_intercept;
	BOOL is_already_finish_erase;
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> _map_origin_addr_to_cc_addr;
	vector<BasicBlock*> bb_list;
public:
	Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size, CodeCache *cc);
	virtual ~Function();
	string get_function_name(){return _function_name;}
	void dump_function_origin();
	void dump_bb_origin();
	ADDR get_function_base(){return _function_base;}
	ORIGIN_ADDR get_origin_function_base(){return _origin_function_base;}
	void intercept_to_random_function();
	void disassemble();
	void split_into_basic_block(MAP_ORIGIN_FUNCTION *func_map);
	Instruction *get_instruction_by_addr(ORIGIN_ADDR origin_addr);
	CODE_CACHE_ADDR get_cc_addr_by_inst_origin_addr(ORIGIN_ADDR origin_addr)
	{
		map<ORIGIN_ADDR, CODE_CACHE_ADDR>::iterator iter = _map_origin_addr_to_cc_addr.find(origin_addr);
		if(iter==_map_origin_addr_to_cc_addr.end())
			return -1;
		else
			return iter->second;
	}

	BOOL is_origin_inst_in_func(ORIGIN_ADDR origin_addr)
	{
		if((origin_addr>=_origin_function_base)&&(origin_addr<=(_origin_function_base+_origin_function_size))){
			ASSERT(get_instruction_by_addr(origin_addr));
			return true;
		}else
			return false;
	}
	void random_function(MAP_ORIGIN_FUNCTION *func_map);
	void get_map_origin_cc_info(multimap<ORIGIN_ADDR, CODE_CACHE_ADDR> &map_info)
	{
		map_info.insert(_map_origin_addr_to_cc_addr.begin(), _map_origin_addr_to_cc_addr.end());
	}
	void flush_function_cc();
	void erase_function();
};

#endif
