#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_
#include <string.h>

#include "disasm_common.h"
#include "type.h"
#include "utility.h"
#include "codecache.h"

#define INST_MAX_SIZE 12

class Instruction
{
public:
	typedef enum{
		EMPTY = 0,
		NONE_TYPE, /*Indicates the instruction is not the flow-control instruction*/
		CND_BRANCH_TYPE,/* Indicates the instruction is one of: JCXZ, JO -- JG, LOOP, LOOPZ, LOOPNZ. */
		DIRECT_CALL_TYPE,/* Indicates the instruction is one of: CALL Relative */
		DIRECT_JMP_TYPE,/* Indicates the instruction is one of: JMP Relative. */
		INDIRECT_CALL_TYPE,/* Indicates the instruction is one of: CALLIN CALL FAR */
		INDIRECT_JMP_TYPE,/*Indicates the instruction is one of: JMPIN */
		RET_TYPE,
	}__attribute((packed)) INSTRUCTION_TYPE;
	static const char * type_to_string[];
private:
	ORIGIN_ADDR _origin_instruction_addr;
	_DInst *_dInst;
	UINT8 inst_size;
	UINT8 inst_code[INST_MAX_SIZE];
	INSTRUCTION_TYPE inst_type;
public:
	Instruction(ORIGIN_ADDR origin_addr);
	~Instruction()
	{
		if(_dInst)
			delete _dInst;
	}

	UINT16 get_inst_opcode()
	{
		return _dInst->opcode;
	}

	UINT8 get_operand_type(INT32 operand_idx)
	{
		return _dInst->ops[operand_idx].type;
	}

	UINT8 get_reg_operand_index(INT32 operand_idx)
	{
		return _dInst->ops[operand_idx].index;
	}
	
	UINT8 get_inst_code_first_byte()
	{
		ASSERT( inst_size>=1);
		return inst_code[0];
	}

	UINT8 get_inst_code_second_byte()
	{
		ASSERT(inst_size>=2);
		return inst_code[1];
	}
		
	void get_inst_code(UINT8 *encode, SIZE size)
	{
		ASSERT(size==inst_size);
		memcpy(encode, inst_code, size);
		return  ;
	}
	SIZE get_inst_size()
	{
		return inst_size;
	}
	
	const char *get_inst_type()
	{
		return type_to_string[inst_type];
	}
	ORIGIN_ADDR get_inst_origin_addr()
	{
		return _origin_instruction_addr;
	}
	BOOL isRipRelative()
	{
		return _dInst->flags&FLAG_RIP_RELATIVE ? true : false;
	}
	BOOL isOrdinaryInst()
	{
		return inst_type==NONE_TYPE;
	}
	BOOL isCall()
	{
		return (inst_type==INDIRECT_CALL_TYPE)||(inst_type==DIRECT_CALL_TYPE);
	}
	BOOL isIndirectCall()
	{
		return inst_type==INDIRECT_CALL_TYPE;
	}
	BOOL isIndirectJmp()
	{
		return inst_type==INDIRECT_JMP_TYPE;
	}
	BOOL isDirectCall()
	{
		return inst_type==DIRECT_CALL_TYPE;
	}
	BOOL isDirectJmp()
	{
		return inst_type==DIRECT_JMP_TYPE;
	}
	BOOL isConditionBranch()
	{
		return inst_type==CND_BRANCH_TYPE;
	}
	BOOL isUnConditionBranch()
	{
		return inst_type==DIRECT_JMP_TYPE || inst_type==INDIRECT_JMP_TYPE;
	}
	BOOL isBranch()
	{
		return inst_type!=NONE_TYPE;
	}
	BOOL isRet()
	{
		return inst_type==RET_TYPE;
	}
	BOOL isMovRspRbp()
	{
		return (_dInst->opcode==I_MOV) && (_dInst->ops[0].type==O_REG) && \
			(_dInst->ops[1].type==O_REG) && (_dInst->ops[0].index==R_RBP) && (_dInst->ops[1].index==R_RSP);
	}
	BOOL isMovRbpRsp()
	{
		return (_dInst->opcode==I_MOV) && (_dInst->ops[0].type==O_REG) && \
			(_dInst->ops[1].type==O_REG) && (_dInst->ops[0].index==R_RSP) && (_dInst->ops[1].index==R_RBP);
	}
	BOOL isLeave()
	{
		return _dInst->opcode==I_LEAVE;
	}
	BOOL isPushRbp()
	{
		return (_dInst->opcode==I_PUSH) && (_dInst->ops[0].type==O_REG) && (_dInst->ops[0].index==R_RBP);
	}
	BOOL isPopRbp()
	{
		return (_dInst->opcode==I_POP) && (_dInst->ops[0].type==O_REG) && (_dInst->ops[0].index==R_RBP);
	}
	ORIGIN_ADDR getBranchTargetOrigin()
	{
		ASSERT(inst_type==DIRECT_JMP_TYPE || inst_type==CND_BRANCH_TYPE || inst_type==DIRECT_CALL_TYPE);
		return INSTRUCTION_GET_TARGET(_dInst);
	}
	ORIGIN_ADDR getBranchFallthroughOrigin()
	{
		ASSERT(inst_type!=NONE_TYPE && inst_type!=DIRECT_JMP_TYPE && inst_type!=INDIRECT_JMP_TYPE);
		return _origin_instruction_addr + inst_size;
	}
	SIZE disassemable(SIZE security_size, const UINT8 * code);
	SIZE copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr, 
		multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin);
	void dump();
private:
	void init_instruction_type();
};

#endif
