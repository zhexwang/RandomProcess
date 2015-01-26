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
	SIZE instruction_size;
	BOOL is_already_disasm;
	UINT8 *inst_encode;
public:
	Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr, SIZE instruction_max_size);
	~Instruction(){;}
	SIZE disassemable();
	DISASM &get_disasm()
	{
		return _disasm;
	}
	void dump();
};

#endif
