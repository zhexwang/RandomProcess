#include "function.h"


Function::Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size)
	:_code_segment(code_segment), _function_name(name), _origin_function_base(origin_function_base), _origin_function_size(origin_function_size)
	, _random_function_base(0), _random_function_size(0), is_already_disasm(false)
{
	_function_base = code_segment->convert_origin_process_addr_to_this(_origin_function_base);
	_function_size = (SIZE)_origin_function_size;
}
Function::~Function(){
	;
}
void Function::dump_function_origin()
{
	PRINT("%s(0x%lx-0x%lx)\n", _function_name.c_str(), _origin_function_base, _origin_function_size+_origin_function_base);
	if(is_already_disasm){
		for(vector<Instruction*>::iterator iter = _origin_function_instructions.begin(); iter!=_origin_function_instructions.end(); iter++){
			(*iter)->dump();
		}
	}
}
void Function::disassemble()
{
	ORIGIN_ADDR origin_addr = _origin_function_base;
	ADDR current_addr = _function_base;
	SIZE security_size = _origin_function_size;

	while(security_size>0){
		Instruction *instr = new Instruction(origin_addr, current_addr, security_size);
		SIZE instr_size = instr->disassemable();
		origin_addr += instr_size;
		current_addr += instr_size;
		security_size -= instr_size;
		_origin_function_instructions.push_back(instr);
	}
	ASSERT(security_size==0);
	is_already_disasm = true;
}

void Function::point_to_random_function()
{
	NOT_IMPLEMENTED(wz);
}

void Function::random_function()
{
	NOT_IMPLEMENTED(wz);
}
