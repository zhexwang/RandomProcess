#include "instruction.h"
#include "inst_macro.h"
#include <string.h>

Instruction::Instruction(ORIGIN_ADDR origin_addr)	
{
	_dInst.addr = origin_addr;
}

SIZE Instruction::copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr, 
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	ASSERT(isOrdinaryInst()|| isRet() || isIndirectCall() || isIndirectJmp());

	if(_dInst.flags&FLAG_RIP_RELATIVE){
		ASSERTM(_dInst.ops[0].type==O_SMEM || _dInst.ops[0].type==O_MEM 
			|| _dInst.ops[1].type==O_SMEM || _dInst.ops[1].type==O_MEM
			||_dInst.ops[2].type==O_SMEM || _dInst.ops[2].type==O_MEM, "unknown operand in RIP-operand!\n");
		INT64 offset_64 = origin_copy_addr - _dInst.addr - _dInst.disp;
		ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
		offset_64 = origin_copy_addr - _dInst.addr;
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
	//add origin_function_inst-->origin_cc_inst and origin_cc -->origin_inst
	map_origin_to_cc.insert(make_pair(_dInst.addr, origin_copy_addr));
	map_cc_to_origin.insert(make_pair(origin_copy_addr, _dInst.addr));
	
	return _dInst.size;	
}

SIZE Instruction::disassemable(SIZE security_size, const UINT8 * code)
{
	//init the code information
	_CodeInfo ci;
	ci.code = code;
	ci.codeLen = security_size;
	ci.codeOffset = _dInst.addr;
	ci.dt = Decode64Bits;
	ci.features = DF_NONE;
	//decompose the instruction
	UINT32 dinstcount = 0;
	if(code[0]==0xdf && code[1]==0xc0){
		_dInst.size = 2;
		dinstcount = 1;
		_dInst.opcode = 0;
		_dInst.flags = 0;
		_dInst.meta &= (~0x7);//FC_NONE
	}else
		distorm_decompose(&ci, &_dInst, 1, &dinstcount);
	ASSERT(dinstcount==1);	
	//record inst code
	memcpy(inst_code, code, _dInst.size);
	
	return _dInst.size;
}

void Instruction::dump()
{
	if(_dInst.size==2 && inst_code[0]==0xdf && inst_code[1]==0xc0){
		PRINT("%12lx (02) dfc0 %-20s FFREEP ST0\n", _dInst.addr, " ");
	}else{
		_DecodedInst  decodedInst;
		UINT32 decodedInstsCount = 0;
		distorm_decode(_dInst.addr, inst_code, _dInst.size, Decode64Bits, &decodedInst, _dInst.size, &decodedInstsCount);
		ASSERT(decodedInstsCount==1);
		PRINT("%12lx (%02d) %-24s  %s%s%s\n", decodedInst.offset, decodedInst.size, decodedInst.instructionHex.p,\
			(char*)decodedInst.mnemonic.p, decodedInst.operands.length != 0 ? " " : "", (char*)decodedInst.operands.p);
	}
}

