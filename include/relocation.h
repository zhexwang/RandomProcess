#ifndef _RELOCATION_H_
#define _RELOCATION_H_

#include "type.h"

typedef enum RELOCATION_TYPE{
	REL8_BB_PTR = 0,
	REL32_BB_PTR,
	ABSOLUTE_BB_PTR,
	RELOCATION_TYPE_NUM,
}RELOCATION_TYPE;

typedef struct relocation_item{
	RELOCATION_TYPE type;
	ADDR relocate_pos;
	ORIGIN_ADDR origin_base_addr;
	ADDR target_bb_ptr;
}RELOCATION_ITEM;

#endif