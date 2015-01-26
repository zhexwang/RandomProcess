#include "instruction.h"
#include <string.h>

Instruction::Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr, SIZE instruction_max_size)
	:_origin_instruction_addr(origin_addr), _current_instruction_addr(current_addr), is_already_disasm(false)
	, inst_encode(NULL)
{
	memset(&_disasm, 0, sizeof(_disasm));
	_disasm.EIP = current_addr;
	_disasm.VirtualAddr = origin_addr;
	_disasm.Archi = 64;
	_disasm.Options = ATSyntax|PrefixedNumeral|Tabulation;
	_disasm.SecurityBlock = instruction_max_size;
}

SIZE Instruction::disassemable()
{
	instruction_size = Disasm(&_disasm);
	ASSERTM(instruction_size!=(SIZE)OUT_OF_BLOCK,"Disasm engine is not allowed to read more memory!\n");
	ASSERTM(instruction_size!=(SIZE)UNKNOWN_OPCODE, "unknown opcode!\n");
	inst_encode = new UINT8(instruction_size);
	memcpy(inst_encode, (UINT8*)_disasm.EIP, instruction_size);
	is_already_disasm = true;
	return instruction_size;
}

void Instruction::dump()
{
	if(is_already_disasm)
		PRINT("%.8lx %s\n", _origin_instruction_addr, _disasm.CompleteInstr);
	else
		PRINT("none!\n");
}

