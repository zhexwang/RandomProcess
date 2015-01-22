#ifndef _READELF_H_
#define _READELF_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "function.h"
#include "map_function.h"

class ReadElf
{
private:
	INT32 elf_fd;
	string elf_name;
	BOOL is_so;
	ORIGIN_ADDR elf_load_start;
	SIZE elf_size;
	void *map_start;
	CodeSegment *_code_segment;
public:
	ReadElf(CodeSegment *code_segment);
	~ReadElf();
	static bool JudgeIsElf(void *start)
	{
		UINT32 *bs = (UINT32 *)start;
		return (*bs == 0x464c457f);
	}
	void scan_and_record_function(MAP_FUNCTION *map_function, MAP_ORIGIN_FUNCTION *map_origin_function);
};

#endif
