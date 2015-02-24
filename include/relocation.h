#ifndef _RELOCATION_H_
#define _RELOCATION_H_

#include "type.h"

typedef enum RELOCATION_TYPE{
	JMP_REL8_BB_PTR = 0,
	JMP_REL32_BB_PTR,
	ABSOLUTE_BB_PTR,
	RELOCATION_TYPE_NUM,
}RELOCATION_TYPE;

typedef struct relocation_item{
	RELOCATION_TYPE type;
	ADDR relocate_pos;
	ADDR target_bb_ptr;
}RELOCATION_ITEM;

#endif