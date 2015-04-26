#include "type.h"
#include "utility.h"
#include "stack.h"
#include "code_segment.h"

typedef struct lib_stack_item{
	ORIGIN_ADDR off_addr;
	LIB_STACK_TYPE type;
}LIB_STACK_ITEM;

LIB_STACK_ITEM libc_stack[] = {
		{0xdfa30, {STACK_RSP, 0}},
		{0xdfb0d, {STACK_RSP, 8}},
		{0xdf74d, {STACK_RSP, 8}},
		{0xdfaad, {STACK_RSP, 8}},
};

LIB_STACK_ITEM libpthread_stack[] = {
		{0xc73c, {STACK_RSP, 0}},
		{0xb85c, {STACK_RSP, 0x28}},
		{0xc778, {STACK_RSP, 0}},
		{0x8165, {STACK_RBP, 8}},
		{0xe585, {STACK_RSP, 16}},
		{0xe3f4, {STACK_RSP, 16}},
		{0xc1f3, {STACK_RSP, 0}},
};

typedef struct unused_rbp_func{
	string func_name;
	LIB_STACK_TYPE type;
}UNUSED_RBP_FUNC;//in lib

UNUSED_RBP_FUNC unused_rbp_func_return[] = {
		{"__pthread_cond_wait", {STACK_RSP, 0x28}},
		{"__pthread_cond_broadcast", {STACK_RSP, 0x0}},
		{"__pthread_barrier_wait", {STACK_RSP, 0x0}},
		{"__pthread_cond_signal", {STACK_RSP, 0x0}},
};

void read_syscall_inst_stack_type(CodeSegment *cs)
{
	if(cs->file_path.find("/lib/libc.so.6")!=string::npos){
		INT32 num = sizeof(libc_stack)/sizeof(LIB_STACK_ITEM);
		for(INT32 idx = 0; idx<num ; idx++){
			ORIGIN_ADDR inst_addr = libc_stack[idx].off_addr + cs->code_start;
			map<ORIGIN_ADDR, LIB_STACK_TYPE>::iterator iter = ShareStack::lib_stack_map.find(inst_addr);
			iter->second.reg = libc_stack[idx].type.reg;
			iter->second.op = libc_stack[idx].type.op;
		}
	}else if(cs->file_path.find("/lib/libpthread.so.0")!=string::npos){
		INT32 num = sizeof(libpthread_stack)/sizeof(LIB_STACK_ITEM);
		for(INT32 idx = 0; idx<num ; idx++){
			ORIGIN_ADDR inst_addr = libpthread_stack[idx].off_addr + cs->code_start;
			map<ORIGIN_ADDR, LIB_STACK_TYPE>::iterator iter = ShareStack::lib_stack_map.find(inst_addr);
			iter->second.reg = libpthread_stack[idx].type.reg;
			iter->second.op = libpthread_stack[idx].type.op;
		}

	}
}

void read_unused_rbp_func_return_addr()
{
	INT32 num = sizeof(unused_rbp_func_return)/sizeof(UNUSED_RBP_FUNC);
	ShareStack::unused_func_map.clear();
	for(INT32 idx = 0; idx<num ; idx++)
		ShareStack::unused_func_map.insert(make_pair(unused_rbp_func_return[idx].func_name, unused_rbp_func_return[idx].type));
}

