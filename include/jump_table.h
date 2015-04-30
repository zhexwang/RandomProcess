#ifndef _JUMP_TABLE_H_
#define _JUMP_TABLE_H_
#include <string.h>
#include "type.h"
#include "utility.h"

class JumpTable
{
private:
	UINT8 curr_idx;
	ADDR curr_table_start[2];
	ADDR curr_table_end[2];
	ADDR curr_table_ptr[2];
	ORIGIN_ADDR origin_table_start[2];
	ORIGIN_ADDR origin_table_end[2];
	ORIGIN_ADDR origin_table_ptr[2];	
public:
	JumpTable(ORIGIN_ADDR _origin_table_start, ADDR _curr_table_start, SIZE table_size): curr_idx(1)
	{
		curr_table_start[0] = _curr_table_start;
		curr_table_end[0] = curr_table_start[0] + table_size/2;
		curr_table_ptr[0] = curr_table_start[0];

		origin_table_start[0] = _origin_table_start;
		origin_table_end[0] = origin_table_start[0] + table_size/2;
		origin_table_ptr[0] = origin_table_start[0];
		
		curr_table_start[1] = curr_table_end[0];
		curr_table_end[1] = curr_table_start[1] + table_size/2;
		curr_table_ptr[1] = curr_table_start[1];

		origin_table_start[1] = origin_table_end[0];
		origin_table_end[1] = origin_table_start[1] + table_size/2;
		origin_table_ptr[1] = origin_table_start[1];
	}

	void flush()
	{
		curr_idx = curr_idx==0 ? 1 : 0;
		curr_table_ptr[curr_idx] = curr_table_start[curr_idx];
		origin_table_ptr[curr_idx] = origin_table_start[curr_idx];
	}
	
	void erase_old_jump_table()
	{
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		memset((void*)curr_table_start[old_idx], 0x0, (SIZE)(curr_table_end[old_idx] - curr_table_start[old_idx]));
		curr_table_ptr[old_idx] = curr_table_start[old_idx];
		origin_table_ptr[old_idx] = origin_table_start[old_idx];
	}	

	INT32 insert_target(ORIGIN_ADDR addr)
	{
		INT32 ret = curr_table_ptr[curr_idx] - curr_table_start[curr_idx];
		*(ADDR*)curr_table_ptr[curr_idx] = addr;
		curr_table_ptr[curr_idx] += sizeof(ORIGIN_ADDR);
		origin_table_ptr[curr_idx] += sizeof(ORIGIN_ADDR);
		ASSERT(curr_table_ptr[curr_idx]<=curr_table_end[curr_idx] && origin_table_ptr[curr_idx]<=origin_table_end[curr_idx]);
		ASSERT(ret%8==0);
		return ret;
	}

	ORIGIN_ADDR get_old_table_base()
	{
		return origin_table_start[(curr_idx==0 ? 1 : 0)];
	}

	ORIGIN_ADDR get_new_table_base()
	{
		return origin_table_start[curr_idx];
	}
};

#endif
