#include "type.h"
#include "utility.h"

//jmpq *mem(not rip relative) ==> cmpl $0x12345678, mem
void convertJmpinToCmpLImm32(UINT8 *jmpin_instcode, UINT32 jmpin_size, UINT32 imm32, ADDR &code_start){
	ASSERT(jmpin_size<=8);
	UINT8 *code_ptr = reinterpret_cast<UINT8*>(code_start);
	UINT32 jmpin_index = 0;
	// 1. copy prefix 
	if((jmpin_instcode[jmpin_index])!=0xff){//copy prefix
		*(code_ptr++) = jmpin_instcode[jmpin_index++];//copy prefix
		ASSERT(jmpin_instcode[jmpin_index]==0xff);
	}
	// 2.set opcode
	*(code_ptr++) = 0x81;
	jmpin_index++;
	// 3.calculate cmp ModRM
	*(code_ptr++) = jmpin_instcode[jmpin_index++]|0x38;
	// 4.copy SIB | Displacement of jmpin_inst
	while(jmpin_index<jmpin_size)
		*(code_ptr++) = jmpin_instcode[jmpin_index++];
	// 5.set imm32
	UINT32 *code_ptr2 = reinterpret_cast<UINT32*>(code_ptr);
	*(code_ptr2++) = imm32;
	// 6.ret code start
	code_start = reinterpret_cast<ADDR>(code_ptr2);
}

//jmpq *mem(rip relative) ==> cmpl $0x12345678, mem
void convertJmpinRipToCmpLImm32(UINT8 *jmpin_instcode, UINT32 jmpin_size, ADDR jmpin_addr, \
	UINT32 imm32, ADDR &code_start, ORIGIN_ADDR origin_cc_start){
	
	ASSERT(jmpin_size<=8);
	UINT8 *code_ptr = reinterpret_cast<UINT8*>(code_start);
	UINT32 jmpin_index = 0;
	// 1. copy prefix 
	if((jmpin_instcode[jmpin_index])!=0xff){//copy prefix
		*(code_ptr++) = jmpin_instcode[jmpin_index++];//copy prefix
		ASSERT(jmpin_instcode[jmpin_index]==0xff);
	}
	// 2.set opcode
	*(code_ptr++) = 0x81;
	jmpin_index++;
	// 3.calculate cmp ModRM
	*(code_ptr++) = jmpin_instcode[jmpin_index++]|0x38;
	// 4.copy displacement 
	UINT32 *record_disp_pos = (UINT32*)code_ptr;
	while(jmpin_index<jmpin_size)
		*(code_ptr++) = jmpin_instcode[jmpin_index++];
	// 5.set imm32
	UINT32 *code_ptr2 = reinterpret_cast<UINT32*>(code_ptr);
	*(code_ptr2++) = imm32;
	// 6.calculate the offset
	INT64 offset_64 = origin_cc_start - jmpin_addr + 0x4;
	ASSERT((offset_64 > 0 ? offset_64 : -offset_64) < 0x7fffffff);
	INT32 offset = (INT32)offset_64;
	// 7.ret code start
	code_start = reinterpret_cast<ADDR>(code_ptr2);
	// 8.relocate
	*record_disp_pos -= offset;
}

