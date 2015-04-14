#ifndef _READELF_H_
#define _READELF_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "function.h"
#include "map_function.h"
#include "codecache.h"
#include <elf.h>
#include <vector>

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
	//section information
	// 1.symbol
	Elf64_Sym *symbol_table;
	Elf64_Sym *dynsym_table;
	INT32 symt_num;
	INT32 dynsymt_num;
	// 2.string
	char *string_table;
	char *dynstr_table;
public:	
	// 3.init section
	ORIGIN_ADDR origin_init_start;
	ADDR curr_init_start;
	ORIGIN_ADDR origin_init_end;
	// 4.fini section
	ORIGIN_ADDR origin_fini_start;
	ADDR curr_fini_start;
	ORIGIN_ADDR origin_fini_end;
	// 5.text section
	ORIGIN_ADDR origin_text_start;
	ADDR curr_text_start;
	ORIGIN_ADDR origin_text_end;
public:
	ReadElf(CodeSegment *code_segment);
	~ReadElf();
	static bool JudgeIsElf(void *start)
	{
		UINT32 *bs = (UINT32 *)start;
		return (*bs == 0x464c457f);
	}
	void scan_and_record_function(MAP_ORIGIN_FUNCTION *map_origin_function, CodeCache *cc);
};

#endif
