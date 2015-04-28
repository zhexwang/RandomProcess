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
private:
	_DInst _dInst;
	UINT8 inst_code[INST_MAX_SIZE];
public:
	Instruction(ORIGIN_ADDR origin_addr);
	~Instruction()
	{
		;
	}

	UINT16 get_inst_opcode()
	{
		return _dInst.opcode;
	}

	UINT8 get_operand_type(INT32 operand_idx)
	{
		return _dInst.ops[operand_idx].type;
	}

	UINT8 get_reg_operand_index(INT32 operand_idx)
	{
		return _dInst.ops[operand_idx].index;
	}
	
	UINT8 get_inst_code_first_byte()
	{
		ASSERT( _dInst.size>=1);
		return inst_code[0];
	}

	UINT8 get_inst_code_second_byte()
	{
		ASSERT(_dInst.size>=2);
		return inst_code[1];
	}

	UINT8 get_inst_code_third_byte()
	{
		ASSERT(_dInst.size>=3);
		return inst_code[2];
	}		
	void get_inst_code(UINT8 *encode, SIZE size)
	{
		ASSERT(size==_dInst.size);
		memcpy(encode, inst_code, size);
		return  ;
	}
	SIZE get_inst_size()
	{
		return _dInst.size;
	}
	
	ORIGIN_ADDR get_inst_origin_addr()
	{
		return _dInst.addr;
	}
	BOOL isRipRelative()
	{
		return _dInst.flags&FLAG_RIP_RELATIVE ? true : false;
	}
	
	BOOL isOrdinaryInst()
	{
		switch(META_GET_FC(_dInst.meta)){
			case FC_NONE: 
			case FC_SYS:
			case FC_CMOV:
			case FC_INT:
				return true;
			default:
				return false;
		}
	}
	BOOL isSyscall()
	{
		return _dInst.opcode==I_SYSCALL;
	}
	BOOL isCall()
	{
		return META_GET_FC(_dInst.meta)==FC_CALL;
	}
	BOOL isIndirectCall()
	{
		return (META_GET_FC(_dInst.meta)==FC_CALL) && (_dInst.ops[0].type!=O_PC);
	}
	BOOL isIndirectJmp()
	{
		return (META_GET_FC(_dInst.meta)==FC_UNC_BRANCH) && (_dInst.ops[0].type!=O_PC);
	}
	BOOL isDirectCall()
	{
		return (META_GET_FC(_dInst.meta)==FC_CALL) && (_dInst.ops[0].type==O_PC);
	}
	BOOL isDirectJmp()
	{
		BOOL ret = (META_GET_FC(_dInst.meta)==FC_UNC_BRANCH) && (_dInst.ops[0].type==O_PC);
		if(ret)
			ASSERT(inst_code[0]!=0xff);
		return ret;
	}
	BOOL isConditionBranch()
	{
		return META_GET_FC(_dInst.meta)==FC_CND_BRANCH;
	}
	BOOL isUnConditionBranch()
	{
		return META_GET_FC(_dInst.meta)==FC_UNC_BRANCH;
	}
	BOOL isRet()
	{
		return META_GET_FC(_dInst.meta)==FC_RET;
	}
	BOOL isMovRspRbp()
	{
		return (_dInst.opcode==I_MOV) && (_dInst.ops[0].type==O_REG) && \
			(_dInst.ops[1].type==O_REG) && (_dInst.ops[0].index==R_RBP) && (_dInst.ops[1].index==R_RSP);
	}
	BOOL isMovRbpRsp()
	{
		return (_dInst.opcode==I_MOV) && (_dInst.ops[0].type==O_REG) && \
			(_dInst.ops[1].type==O_REG) && (_dInst.ops[0].index==R_RSP) && (_dInst.ops[1].index==R_RBP);
	}
	BOOL isLeave()
	{
		return _dInst.opcode==I_LEAVE;
	}
	BOOL isPushRbp()
	{
		return (_dInst.opcode==I_PUSH) && (_dInst.ops[0].type==O_REG) && (_dInst.ops[0].index==R_RBP);
	}
	BOOL isPush()
	{
		return _dInst.opcode==I_PUSH;
	}
	BOOL isPopRbp()
	{
		return (_dInst.opcode==I_POP) && (_dInst.ops[0].type==O_REG) && (_dInst.ops[0].index==R_RBP);
	}
	BOOL isAddRsp()
	{
		return (_dInst.opcode==I_ADD) && (_dInst.ops[0].type==O_REG) && (_dInst.ops[0].index==R_RSP) && (_dInst.ops[1].type==O_IMM);
	}
	BOOL isSubRsp()
	{
		return (_dInst.opcode==I_SUB) && (_dInst.ops[0].type==O_REG) && (_dInst.ops[0].index==R_RSP) && (_dInst.ops[1].type==O_IMM);
	}
	ORIGIN_ADDR getBranchTargetOrigin()
	{
		ASSERT(isDirectJmp()|| isConditionBranch()|| isDirectCall());
		return INSTRUCTION_GET_TARGET(&_dInst);
	}
	ORIGIN_ADDR getBranchFallthroughOrigin()
	{
		ASSERT(!isOrdinaryInst() && !isUnConditionBranch());
		return _dInst.addr + _dInst.size;
	}
	SIZE disassemable(SIZE security_size, const UINT8 * code);
	SIZE copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr, 
		multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin);
	void dump();
};

#endif

