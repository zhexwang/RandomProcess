#include "instruction.h"
#include "inst_macro.h"
#include <string.h>


const char * Instruction::type_to_string[] = {"EMPTY","NONE_TYPE", "CND_BRANCH_TYPE","DIRECT_CALL_TYPE","DIRECT_JMP_TYPE",\
	   "INDIRECT_CALL_TYPE","INDIRECT_JMP_TYPE","RET_TYPE"};

Instruction::Instruction(ORIGIN_ADDR origin_addr)
	:_origin_instruction_addr(origin_addr), _dInst(NULL), inst_type(Instruction::EMPTY)
{
	;
}

SIZE Instruction::copy_instruction(CODE_CACHE_ADDR curr_copy_addr, ORIGIN_ADDR origin_copy_addr, 
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> &map_origin_to_cc, map<ORIGIN_ADDR, ORIGIN_ADDR> &map_cc_to_origin)
{
	ASSERT((inst_type==NONE_TYPE || inst_type==RET_TYPE\
		|| inst_type==INDIRECT_CALL_TYPE || inst_type==INDIRECT_JMP_TYPE));

	if(_dInst->flags&FLAG_RIP_RELATIVE){
		ASSERTM(_dInst->ops[0].type==O_SMEM || _dInst->ops[0].type==O_MEM 
			|| _dInst->ops[1].type==O_SMEM || _dInst->ops[1].type==O_MEM, "unknown operand in RIP-operand!\n");
		INT64 offset_64 = origin_copy_addr - _origin_instruction_addr - _dInst->disp;
		ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
		offset_64 = origin_copy_addr - _origin_instruction_addr;
		ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
		INT32 offset = (INT32)offset_64;
		//copy inst
		get_inst_code((UINT8 *)curr_copy_addr, inst_size);
		//find disp
		INT32 *disp_pos = reinterpret_cast<INT32*>(curr_copy_addr);
		INT32 *end = reinterpret_cast<INT32*>(curr_copy_addr + inst_size - 3);
		BOOL match = false;
		INT32 *record_pos = NULL;
		while(disp_pos!=end){
			if((*disp_pos)==(INT32)_dInst->disp){
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
		get_inst_code((UINT8 *)curr_copy_addr, inst_size);
	//add origin_function_inst-->origin_cc_inst and origin_cc -->origin_inst
	map_origin_to_cc.insert(make_pair(_origin_instruction_addr, origin_copy_addr));
	map_cc_to_origin.insert(make_pair(origin_copy_addr, _origin_instruction_addr));
	
	return inst_size;	
}

SIZE Instruction::disassemable(SIZE security_size, const UINT8 * code)
{
	//init the code information
	_CodeInfo ci;
	ci.code = code;
	ci.codeLen = security_size;
	ci.codeOffset = _origin_instruction_addr;
	ci.dt = Decode64Bits;
	ci.features = DF_NONE;
	//decompose the instruction
	UINT32 dinstcount = 0;
	_dInst = new _DInst();
	distorm_decompose(&ci, _dInst, 1, &dinstcount);
	ASSERT(dinstcount==1 && _dInst->addr==_origin_instruction_addr);	
	//record inst code
	inst_size = _dInst->size;
	memcpy(inst_code, code, inst_size);
	
	init_instruction_type();
	
	return inst_size;
}

void Instruction::init_instruction_type()
{
	switch(META_GET_FC(_dInst->meta)){
		case FC_NONE: 
		case FC_SYS:
		case FC_CMOV:
			inst_type = NONE_TYPE;
			break;
		case FC_CALL: 
			if(_dInst->ops[0].type==O_PC)
				inst_type = DIRECT_CALL_TYPE;
			else
				inst_type = INDIRECT_CALL_TYPE;
			break;
		case FC_RET: 
			inst_type = RET_TYPE;
			break;
		case FC_UNC_BRANCH:
			if(_dInst->ops[0].type==O_PC){
				ASSERT(inst_code[0]!=0xff);
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
	_DecodedInst  decodedInst;
	UINT32 decodedInstsCount = 0;
	distorm_decode(_origin_instruction_addr, inst_code, inst_size, Decode64Bits, &decodedInst, inst_size, &decodedInstsCount);
	ASSERT(decodedInstsCount==1);
	PRINT("%12lx (%02d) %-24s  %s%s%s\n", decodedInst.offset, decodedInst.size, decodedInst.instructionHex.p,\
		(char*)decodedInst.mnemonic.p, decodedInst.operands.length != 0 ? " " : "", (char*)decodedInst.operands.p);
}

