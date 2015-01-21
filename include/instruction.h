#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include "disasm_common.h"
#include "type.h"
#include "utility.h"

class Instruction
{
private:
	ORIGIN_ADDR _origin_instruction_addr;
	ADDR _current_instruction_addr;
	DISASM _disasm;
public:
	Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr);
	void disassemable();
};

#endif
