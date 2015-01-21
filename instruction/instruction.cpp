#include "instruction.h"

Instruction::Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr)
	:_origin_instruction_addr(origin_addr), _current_instruction_addr(current_addr)
{
	NOT_IMPLEMENTED(wz);
}

void Instruction::disassemable()
{
	NOT_IMPLEMENTED(wz);
}

