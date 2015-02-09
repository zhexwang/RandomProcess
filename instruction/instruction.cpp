#include "instruction.h"
#include <string.h>


const char * Instruction::type_to_string[] = {"EMPTY","NONE_TYPE", "CND_BRANCH_TYPE","DIRECT_CALL_TYPE","DIRECT_JMP_TYPE",\
	   "INDIRECT_CALL_TYPE","INDIRECT_JMP_TYPE","RET_TYPE"};

Instruction::Instruction(ORIGIN_ADDR origin_addr, ADDR current_addr, SIZE instruction_max_size)
	:inst_type(Instruction::EMPTY), _origin_instruction_addr(origin_addr), _current_instruction_addr(current_addr)
	, is_already_disasm(false), security_size(instruction_max_size), _curr_copy_addr(0)
	, _origin_copy_addr(0), _inst_copy_size(0)
{
	;
}

SIZE Instruction::copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr)
{
	ASSERT(is_already_disasm);
	_curr_copy_addr = curr_copy_addr;
	_origin_copy_addr = origin_copy_addr;
	
	if(_dInst.flags&FLAG_RIP_RELATIVE){
		ERR("is PC relative!\n");
	}else if(isDirectCall()){
		ERR("is direct call!\n");
	}else if (isDirectJmp()){
		ERR("is direct jmp!\n");
	}else if (isConditionBranch()){
		ERR("is condtion branch!\n");
	}else{//the instruction can be copy directly
		get_inst_code((UINT8 *)curr_copy_addr, _dInst.size);
		_inst_copy_size = _dInst.size;
		return _dInst.size;
	}
	_inst_copy_size = 0;
	return 0;
}

SIZE Instruction::disassemable()
{
	//init the code information
	_CodeInfo ci;
	ci.code = (const uint8_t*)_current_instruction_addr;
	ci.codeLen = security_size;
	ci.codeOffset = 0;
	ci.dt = Decode64Bits;
	ci.features = DF_NONE;
	//decompose the instruction
	UINT32 dinstcount = 0;
	distorm_decompose(&ci, &_dInst, 1, &dinstcount);
	ASSERT(dinstcount==1);
	//get string
	distorm_format(&ci, &_dInst, &_decodedInst);
	
	init_instruction_type();
	is_already_disasm = true;
	
	return _dInst.size;
}

void Instruction::init_instruction_type()
{
	switch(META_GET_FC(_dInst.meta)){
		case FC_NONE: 
		case FC_SYS:
		case FC_CMOV:
			inst_type = NONE_TYPE;
			break;
		case FC_CALL: 
			if(_dInst.ops[0].type==O_PC)
				inst_type = DIRECT_CALL_TYPE;
			else
				inst_type = INDIRECT_CALL_TYPE;
			break;
		case FC_RET: 
			inst_type = RET_TYPE;
			break;
		case FC_UNC_BRANCH:
			if(_dInst.ops[0].type==O_PC){
				char first_byte = _decodedInst.instructionHex.p[0];
				ASSERT(first_byte!=0xff);
				inst_type = DIRECT_JMP_TYPE;
			}else
				inst_type = INDIRECT_JMP_TYPE;
			break;
		case FC_CND_BRANCH:
			inst_type = CND_BRANCH_TYPE;
			break;
		case FC_INT:
			ASSERTM(0, "Instruction is INT. It is not allowed!\n");
			break;
		default:
			ASSERTM(0, "Illegall instruction!\n");
	}
	
}

void Instruction::dump()
{
	if(is_already_disasm){
		PRINT("%12lx (%02d) %-24s  %s%s%s", _origin_instruction_addr, _decodedInst.size, _decodedInst.instructionHex.p,\
			(char*)_decodedInst.mnemonic.p, _decodedInst.operands.length != 0 ? " " : "", (char*)_decodedInst.operands.p);
		if(inst_type==DIRECT_JMP_TYPE || inst_type==CND_BRANCH_TYPE || inst_type==DIRECT_CALL_TYPE)
			PRINT(" (%lx)", getBranchTargetOrigin());
	}else
		PRINT("Do not disasm!");
	PRINT("\n");
}

