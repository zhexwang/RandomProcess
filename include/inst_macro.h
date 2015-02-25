#ifndef _INST_MACRO_H_
#define _INST_MACRO_H_
#include <ucontext.h>
#include "utility.h"

#if 0
enum RegisterEncode{ 
     RAX=0,RCX,RDX,RBX,RSP,RBP,RSI,RDI,
     R8,R9,R10,R11,R12,R13,R14,R15,NUM
};
inline RegisterEncode RegisterMap(uint8_t index){
	switch(index){
		case REG_R8:return R8;
		case REG_R9:return R9;
		case REG_R10:return R10;
		case REG_R11:return R11;
		case REG_R12:return R12;
		case REG_R13:return R13;
		case REG_R14:return R14;
		case REG_R15:return R15;
		case REG_RDI:return RDI;
		case REG_RSI:return RSI;
		case REG_RBP:return RBP;
		case REG_RBX:return RBX;
		case REG_RDX:return RDX;
		case REG_RAX:return RAX;
		case REG_RCX:return RCX;
		case REG_RSP:return RSP;
		case REG_RIP:
			ASSERTM(0, "reg index can not be REG_RIP!");
		default:
			ASSERTM(0, "RegisterMap failed index=%d\n",index);
	}
	return NUM;
}

inline uint8_t convertCallinPrefixToMovPrefix(uint8_t *callin_ptr, uint8_t &prefix_index, RegisterEncode re){
	uint8_t prefix = callin_ptr[prefix_index];
	if((prefix&0xf0) == 0x40){
		prefix_index++;
		if(re<=RDI)
			return prefix|0x8;
		else
			return prefix|0xc;
	}else{
		if(re<=RDI)
			return 0x48;
		else
			return 0x4c;
	}
}

inline uint8_t convertCallinPrefixToCmpPrefix(uint8_t  *callin_ptr, uint8_t &prefix_index){
	uint8_t prefix = callin_ptr[prefix_index];
	if((prefix&0xf0) == 0x40){
		prefix_index++;
		return prefix|0x8;
	}else
		return 0x48;
}

inline uint8_t convertCallinModRMToMovModRM(uint8_t callin_modRM, RegisterEncode re){
	uint8_t mod_mask = (((uint8_t)re)&0x7)<<3;
	return (callin_modRM&0xc7)|mod_mask;
}

/*only used for memory relatively inst*/
inline uint8_t convertCallinModRMToCmpModRM(uint8_t callin_modRM){
	return callin_modRM|0x38;
*/
/*complex inst convert
    call* convert to cmp/jmp/mov
*/

//call *mem ==> cmp $0x12345678, mem
static void convertCallinToCmp(uint64_t callin_instcode, uint8_t callin_size, ADDR &code_start, uint32_t imm){
	ASSERT(callin_size<=8);
	uint8_t *callin_instcode_ptr = reinterpret_cast<uint8_t*>(callin_instcode);
	uint8_t *code_ptr = reinterpret_cast<uint8_t*>(code_start);
	uint8_t callin_index = 0;
	// 1.prefix calculate
	*(code_ptr++) = convertCallinPrefixToCmpPrefix(callin_instcode_ptr, callin_index);
	// 2.ignore callin opcode
	ASSERT(callin_instcode_ptr[callin_index++]==0xff);
	// 3.cmp opcode
	*(code_ptr++) = 0x81;
	// 4.calculate cmp ModRM 
	*(code_ptr++) = convertCallinModRMToCmpModRM(callin_instcode_ptr[callin_index++]);
	// 5.copy SIB | Displacement of callin_inst
	while(callin_index<callin_size)
		*(code_ptr++) = callin_instcode_ptr[callin_index++];
	// 6.copy imm32 to cmp
	uint32_t *code_ptr2 = reinterpret_cast<uint32_t*>(code_ptr);
	*(code_ptr2++) = imm;
	// 7.ret code_start
	code_start = reinterpret_cast<ADDR>(code_ptr2);
}

//call* mem ==> mov mem, %reg64
static void convertCallinToMov(uint64_t callin_instcode, uint8_t callin_size, ADDR &code_start, uint8_t reg_index){
	ASSERT(callin_size<=9);
	uint8_t *callin_instcode_ptr = reinterpret_cast<uint8_t*>(callin_instcode);
	uint8_t *code_ptr = reinterpret_cast<uint8_t*>(code_start);
	uint8_t callin_index = 0;
	RegisterEncode re = RegisterMap(reg_index);
	// 1.prefix calculate
	*(code_ptr++) = convertCallinPrefixToMovPrefix(callin_instcode_ptr, callin_index, re);
	// 2.ignore callin opcode
	ASSERT(callin_instcode_ptr[callin_index++]==0xff);
	// 3.mov opcode
	*(code_ptr) = 0x8b;
	// 4.calculate mov ModRM(%reg64)
	*(code_ptr++) = convertCallinModRMToMovModRM(callin_instcode_ptr[callin_index++], re);
	// 5.copy SIB | Displacement of callin_inst
	while(callin_index<callin_size)
		*(code_ptr++) = callin_instcode_ptr[callin_index++];
	// 6.ret code_start
	code_start = reinterpret_cast<ADDR>(code_ptr);
}

//call* mem ==> jmp* mem
static void convertCallinToJmpin(uint64_t callin_instcode, uint8_t callin_size, ADDR &code_start){
	ASSERT(callin_size<=9);
	uint8_t *callin_instcode_ptr = reinterpret_cast<uint8_t*>(&callin_instcode);
	uint8_t *code_ptr = reinterpret_cast<uint8_t*>(code_start);
	uint8_t callin_index = 0;
	// 1. copy prefix and opcode (Jmpin is same to callin)
	if((callin_instcode_ptr[callin_index]&0xf0)==0x40){
		*(code_ptr++) = callin_instcode_ptr[callin_index++];//prefix copy
		*(code_ptr++) = callin_instcode_ptr[callin_index++];//opcode copy
	}else
		*(code_ptr++) = callin_instcode_ptr[callin_index++];//opcode copy
	// 2.calculate Jmpin ModRM
	*(code_ptr++) = (callin_instcode_ptr[callin_index++]&0xc7)|0x20;
	// 3.copy SIB | Displacement of callin_inst
	while(callin_index<callin_size)
		*(code_ptr++) = callin_instcode_ptr[callin_index++];
	// 4.ret code start
	code_start = reinterpret_cast<ADDR>(code_ptr);
}
#endif
//macro of instructions

/*movabs $0x1234567887654321, %rbx
    jmpq *%rbx
*/
#define JMP_ASSERT_ADDR(imm, ptr) do{\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	*(ptr1++) = 0x48;\
	*(ptr1++) = 0xbb;\
	long long *ptr2 = reinterpret_cast<long long*>(ptr1);\
	*(ptr2++) = (long long)imm;\
	ptr1 = reinterpret_cast<char*>(ptr2);\
	*(ptr1++) = 0xff;\
	*(ptr1++) = 0xe3;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//mov (0x1234567887654321), %rdi
/*
	movabs $0x1234567887654321, %rdi
	mov (%rdi), %rdi
*/
#define MOV_IMMMEM_RDI(address, ptr) do{\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	*(ptr1++) = 0x48;\
	*(ptr1++) = 0xbf;\
	uint64_t *ptr2 = reinterpret_cast<uint64_t*>(ptr1);\
	*(ptr2++) = (uint64_t)address;\
	ptr1 = reinterpret_cast<char*>(ptr2);\
	*(ptr1++) = 0x48;\
	*(ptr1++) = 0x8b;\
	*(ptr1++) = 0x3f;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//mov %rdi, offset(%rsp)
#define MOV_RDI_RSP_OFFSET(offset, ptr) do{\
	uint32_t *ptr1 = reinterpret_cast<uint32_t*>(ptr);\
	*(ptr1++) = 0x247c8948;\
	char *ptr2 = reinterpret_cast<char*>(ptr1);\
	*(ptr2++) = (uint8_t)offset;\
	ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)


//mov offset(%rsp),%rdi
#define MOV_RSP_OFFSET_RDI(offset, ptr) do{\
	uint32_t *ptr1 = reinterpret_cast<uint32_t*>(ptr);\
	*(ptr1++) = 0x24bc8b48;\
	char *ptr2 = reinterpret_cast<char*>(ptr1);\
	*(ptr2++) = (uint8_t)offset;\
}while(0)



//push $imm32
#define PUSH_IMM32(imm, ptr) do{ \
     char *ptr1 = reinterpret_cast<char*>(ptr);\
     *(ptr1++) = 0x68;\
     int *ptr2 = reinterpret_cast<int*>(ptr1);\
     *(ptr2++) = (int)imm;\
     ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)

//push %reg64
#define PUSH_REG64(reg_num, ptr) do{\
     char *ptr1 = reinterpret_cast<char*>(ptr);\
     RegisterEncode regEncode = RegisterMap(reg_num);\
     if(regEncode<=RDI){ \
         *(ptr1++) = 0x50+(uint8_t)regEncode;\
     }else{\
         *(ptr1++) = 0x41;\
         *(ptr1++) = 0x50+(uint8_t)(regEncode-R8);\
     }\
     ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//pop %reg64
#define POP_REG64(reg_num, ptr) do{\
     char *ptr1 = reinterpret_cast<char*>(ptr);\
     RegisterEncode regEncode = RegisterMap(reg_num);\
     if(regEncode<=RDI){ \
         *(ptr1++) = 0x58+(uint8_t)regEncode;\
     }else{\
         *(ptr1++) = 0x41;\
         *(ptr1++) = 0x58+(uint8_t)(regEncode-R8);\
     }\
     ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//ret
#define RET(ptr) do{\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	*(ptr1++) = 0xc3;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//jmp offset
#define JMP_REL32(offset, ptr) do{\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	*(ptr1++) = 0xe9;\
	int *ptr2 = reinterpret_cast<int*>(ptr1);\
	*(ptr2++) = (int)offset;\
	ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)

//jmp* %reg64
#define JMP_REG64(reg_num, ptr) do{\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	RegisterEncode regEncode = RegisterMap(reg_num);\
	if(regEncode<=RDI){\
		*(ptr1++) = 0xff;\
		*(ptr1++) = 0xe0+(uint8_t)regEncode;\
	}else{\
		*(ptr1++) = 0x41;\
		*(ptr1++) = 0xff;\
		*(ptr1++) = 0xe0 +(uint8_t)(regEncode-R8);\
	}\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//mov %reg64, %rdi
#define MOV_REG64_RDI(reg_num, ptr) do{\
	RegisterEncode regEncode = RegisterMap(reg_num);\
	if(regEncode!=RDI){\
		char *ptr1 = reinterpret_cast<char*>(ptr);\
		if(regEncode<RDI){\
			*(ptr1++) = 0x48;\
			*(ptr1++) = 0x89;\
			*(ptr1++) = 0xc7+((uint8_t)regEncode)<<3;\
		}else{\
			*(ptr1++) = 0x4c;\
			*(ptr1++) = 0x89;\
			*(ptr1++) = 0xc7+((uint8_t)(regEncode-R8))<<3;\
		}\
		ptr = reinterpret_cast<ADDR>(ptr1);\
	}\
}while(0)

//cmp $imm32, [%rsp]
#define CMP_IMM32_MEMRSP(imm, ptr) do{\
	uint32_t *ptr1 = reinterpret_cast<uint32_t*>(ptr);\
	*(ptr1++) = 0x243c8148;\
	*(ptr1++) = (uint32_t)imm;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//nop
#define NOP(ptr) do{\
	uint8_t *ptr1 = reinterpret_cast<uint8_t*>(ptr);\
	*(ptr1++) = 0x90;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//cmp $imm32,%reg64
#define CMP_IMM32_REG64(imm, reg_num, ptr) do{ \
    RegisterEncode regEncode = RegisterMap(reg_num);\
    char *ptr1 = reinterpret_cast<char*>(ptr); \
    if(regEncode==RAX){ \
         *(ptr1++) = 0x48; \
         *(ptr1++) = 0x3d; \
     }else if(regEncode<R8 && regEncode>RAX){ \
         *(ptr1++) = 0x48;\
         *(ptr1++) = 0x81; \
         *(ptr1++) = 0xf8+(uint8_t)regEncode;\
     }else if(regEncode>=R8 && regEncode<NUM){ \
        *(ptr1++) = 0x49; \
         *(ptr1++) = 0x81;\
         *(ptr1++) = 0xf8+(uint8_t)(regEncode-R8);\
     }\
     uint32_t *ptr2 = reinterpret_cast<uint32_t*>(ptr1);\
     *(ptr2++) = (uint32_t)imm;\
     ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)

//jle label
#define JLE_REL8(inst_pos, ptr) do{\
	inst_pos = ptr;\
	char *ptr1 = reinterpret_cast<char*>(ptr);\
	*(ptr1++) = 0x7e;\
	*(ptr1++) = 0x00;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//jg label
#define JG_REL8(inst_pos, ptr) do{\
        inst_pos = ptr;\
        char *ptr1 = reinterpret_cast<char*>(ptr);\
        *(ptr1++) = 0x7f;\
        *(ptr1++) = 0x00;\
        ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//set label
#define SET_LABEL(label, ptr) (label=ptr)

//set jle offset
#define SET_JLE_OFFSET(target_addr, jle_addr) do{\
	char offset = target_addr-jle_addr-2;\
	char *jle_ptr = reinterpret_cast<char*>(jle_addr);\
	*(++jle_ptr) = offset;\
}while(0)


//mov $imm32, rsi
#define MOV_IMM32_RSI(imm, ptr) do{\
	unsigned char *ptr1 = reinterpret_cast<unsigned char*>(ptr);\
	*(ptr1++) = 0x48;\
	*(ptr1++) = 0xc7;\
	*(ptr1++) = 0xc6;\
	uint32_t *ptr2 = reinterpret_cast<uint32_t*>(ptr1);\
	*(ptr2++) = (uint32_t)imm;\
	ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)

//mov $imm32, [mDirect]
#define MOV_IMM32_mDirect(imm, addr, ptr) do{\
	uint32_t *ptr1 = reinterpret_cast<uint32_t *>(ptr);\
	*(ptr1++) = 0x2504c748;\
	*(ptr1++) = (uint32_t)addr;\
	*(ptr1++) = (uint32_t)imm;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//mov $imm32, 4(rsp)
#define MOV_IMM32_mRSPHIGH(imm, ptr) do{\
	uint32_t *ptr1 = reinterpret_cast<uint32_t *>(ptr);\
	*(ptr1++) = 0x042444c7;\
	*(ptr1++) = (uint32_t)imm;\
	ptr = reinterpret_cast<ADDR>(ptr1);\
}while(0)

//jmpin off(%%rsp)
#define JMPIN_offRSP(offset, ptr) do{\
	uint8_t *ptr1 = reinterpret_cast<uint8_t *>(ptr);\
	*(ptr1++) = 0xff;\
	*(ptr1++) = 0xa4;\
	*(ptr1++) = 0x24;\
	uint32_t *ptr2 = reinterpret_cast<uint32_t*>(ptr1);\
	*(ptr2++) = (uint32_t)offset;\
	ptr = reinterpret_cast<ADDR>(ptr2);\
}while(0)


#define INV_INS_1(ptr)      do{*(UINT8 *)ptr = 0xd6;} while(0)
#define INV_INS_2(ptr)      do{*(UINT16 *)ptr = 0x360f;} while(0)
#define INV_INS_3(ptr)      do{*(UINT16 *)ptr = 0x260f;} while(0)


#endif  
