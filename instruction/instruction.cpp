#include "instruction.h"
#include "inst_macro.h"
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
/*
SIZE Instruction::random_call_inst_handle()
{
	ASSERT(isCall()&&(_curr_copy_addr!=0)&&(_origin_copy_addr!=0));
	//jmp to the call inst, the call inst will not be copy
	//if address gap is within -0x7fffffff ~ 0x7fffffff, use relative jump
	ORIGIN_ADDR code_start = _curr_copy_addr;
	INT64 offset = _origin_instruction_addr - _origin_copy_addr - 0x5;
	if((offset > 0 ? offset : -offset) < 0x7fffffff){
		JMP_REL32(offset, code_start);
	}else{
		INT32 high32 = _origin_instruction_addr>>32;
		INT32 low32 = _origin_instruction_addr;
		PUSH_IMM32(low32, code_start);
		MOV_IMM32_mRSPHIGH(high32, code_start);
		RET(code_start);
	}
	return code_start - _curr_copy_addr;
}
*/
SIZE Instruction::copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr)
{
	ASSERT(is_already_disasm && (inst_type==NONE_TYPE || inst_type==RET_TYPE || inst_type==INDIRECT_CALL_TYPE));
	_curr_copy_addr = curr_copy_addr;
	_origin_copy_addr = origin_copy_addr;

	if(_dInst.flags&FLAG_RIP_RELATIVE){
		ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM 
			|| _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM, "unknown operand in RIP-operand!\n");
		INT64 offset_64 = _origin_copy_addr - _origin_instruction_addr - _dInst.disp;
		ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
		offset_64 = _origin_copy_addr - _origin_instruction_addr;
		ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
		INT32 offset = (INT32)offset_64;
		//copy inst
		get_inst_code((UINT8 *)curr_copy_addr, _dInst.size);
		//find disp
		INT32 *disp_pos = reinterpret_cast<INT32*>(curr_copy_addr);
		INT32 *end = reinterpret_cast<INT32*>(curr_copy_addr + _dInst.size - 3);
		BOOL match = false;
		INT32 *record_pos = NULL;
		while(disp_pos!=end){
			if((*disp_pos)==(INT32)_dInst.disp){
				ASSERT(!match);
				record_pos = disp_pos;
				match = true;
			}
			
			UINT8 *move = reinterpret_cast<UINT8*>(disp_pos);
			move++;
			disp_pos = reinterpret_cast<INT32*>(move);
		}
		ASSERT(match);
		*record_pos -=  offset;
	}else //the instruction can be copy directly
		get_inst_code((UINT8 *)curr_copy_addr, _dInst.size);

	_inst_copy_size = _dInst.size;
	return _inst_copy_size;
}

SIZE Instruction::disassemable()
{
	//init the code information
	_CodeInfo ci;
	ci.code = (const uint8_t*)_current_instruction_addr;
	ci.codeLen = security_size;
	ci.codeOffset = _origin_instruction_addr;
	ci.dt = Decode64Bits;
	ci.features = DF_NONE;
	//decompose the instruction
	UINT32 dinstcount = 0;
	distorm_decompose(&ci, &_dInst, 1, &dinstcount);
	ASSERT(dinstcount==1 && _dInst.addr==_origin_instruction_addr);
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
				char first_c = _decodedInst.instructionHex.p[0];
				char second_c = _decodedInst.instructionHex.p[1];;
				ASSERT((first_c!='f')&&(second_c!='f'));
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
		PRINT("%12lx (%02d) %-24s  %s%s%s", _decodedInst.offset, _decodedInst.size, _decodedInst.instructionHex.p,\
			(char*)_decodedInst.mnemonic.p, _decodedInst.operands.length != 0 ? " " : "", (char*)_decodedInst.operands.p);
	}else
		PRINT("Do not disasm!");
	PRINT("\n");
}

