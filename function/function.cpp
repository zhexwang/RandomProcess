#include "function.h"


Function::Function(CodeSegment *code_segment, string name, ORIGIN_ADDR origin_function_base, ORIGIN_SIZE origin_function_size)
	:_code_segment(code_segment), _function_name(name), _origin_function_base(origin_function_base), _origin_function_size(origin_function_size)
{
	_function_base = code_segment->convert_origin_process_addr_to_this(_origin_function_base);
	_function_size = (SIZE)_origin_function_size;
}

void Function::disassemble()
{
	NOT_IMPLEMENTED(wz);
}

void Function::point_to_random_function()
{
	NOT_IMPLEMENTED(wz);
}

void Function::random_function()
{
	NOT_IMPLEMENTED(wz);
}
