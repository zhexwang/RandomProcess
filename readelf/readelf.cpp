#include "readelf.h"
#include "disasm_common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
CodeSegment *cs = NULL;

ReadElf::ReadElf(CodeSegment *code_segment): elf_name(code_segment->file_path), is_so(code_segment->isSO)
	, elf_load_start(code_segment->code_start), _code_segment(code_segment), symbol_table(NULL), dynsym_table(NULL)
	, symt_num(0), dynsymt_num(0), string_table(NULL), dynstr_table(NULL), origin_init_start(0), curr_init_start(0), origin_init_end(0)
	, origin_fini_start(0), curr_fini_start(0), origin_fini_end(0), origin_text_start(0), curr_text_start(0), origin_text_end(0)
{
	cs = code_segment;
	ASSERT(!code_segment->is_code_cache);
	//1.open elf file
	elf_fd = open(elf_name.c_str(), O_RDONLY);
	PERROR(elf_fd!=-1, "open failed!");
	//2.calculate elf size
	struct stat statbuf;
	INT32 ret = fstat(elf_fd, &statbuf);
	PERROR(ret==0, "fstat failed!");
	elf_size = statbuf.st_size;
	//3.mmap elf
	map_start = mmap(NULL, elf_size, PROT_READ, MAP_PRIVATE, elf_fd, 0);
	PERROR(map_start!=MAP_FAILED, "mmap failed!");
	//4.judge is elf
	ASSERT(JudgeIsElf(map_start));
	//5.judge is so
	Elf64_Ehdr *elf_header = (Elf64_Ehdr*)map_start;
	Elf64_Half elf_type = elf_header->e_type;
	if(elf_type==ET_EXEC){
		ASSERT(!is_so);
	}else if(elf_type==ET_DYN){
		ASSERT(is_so);
	}else
		ASSERT(0);
	//6.read elf
	Elf64_Shdr *SecHdr = (Elf64_Shdr *)((ADDR)map_start + elf_header->e_shoff);
	char *SecHdrStrTab = (char *)((ADDR)map_start + SecHdr[elf_header->e_shstrndx].sh_offset);
	//search each section to find symtable and string table
	for (INT32 idx=0; idx<elf_header->e_shnum; idx++) {
		Elf64_Shdr *currentSec = SecHdr + idx;
		char *secName = SecHdrStrTab+currentSec->sh_name;
		if(strcmp(secName, ".symtab")==0) {
			symbol_table = (Elf64_Sym *)((ADDR)map_start + currentSec->sh_offset);
			symt_num = currentSec->sh_size/sizeof(Elf64_Sym);
		}else if(strcmp(secName, ".strtab")==0) 
			string_table = (char*)((ADDR)map_start + currentSec->sh_offset);
		else if(strcmp(secName, ".dynsym")==0){
			dynsym_table = (Elf64_Sym *)((ADDR)map_start + currentSec->sh_offset);
			dynsymt_num = currentSec->sh_size/sizeof(Elf64_Sym);		
		}else if (strcmp(secName, ".dynstr")==0){
			dynstr_table = (char*)((ADDR)map_start + currentSec->sh_offset);
		}else if(strcmp(secName, ".text")==0){
			origin_text_start = (ORIGIN_ADDR)(currentSec->sh_offset + elf_load_start);
			curr_text_start = (ADDR)(currentSec->sh_offset + (ADDR)map_start);
			origin_text_end = (ORIGIN_ADDR)(origin_text_start + currentSec->sh_size);
		}else if(strcmp(secName, ".init")==0){
			origin_init_start = (ORIGIN_ADDR)(currentSec->sh_offset + elf_load_start);
			curr_init_start = (ADDR)(currentSec->sh_offset + (ADDR)map_start);
			origin_init_end = (ORIGIN_ADDR)(origin_init_start + currentSec->sh_size);
		}else if(strcmp(secName, ".fini")==0){
			origin_fini_start = (ORIGIN_ADDR)(currentSec->sh_offset + elf_load_start);
			curr_fini_start = (ADDR)(currentSec->sh_offset + (ADDR)map_start);
			origin_fini_end = (ORIGIN_ADDR)(origin_fini_start + currentSec->sh_size);
		}
	}
}

ReadElf::~ReadElf()
{
	INT32 ret = close(elf_fd);
	PERROR(ret==0, "close failed!");
	ret = munmap(map_start, elf_size);
	PERROR(ret==0, "munmap failed!");
}

static void disassemble_to_find_direct_target_out_of_func(ORIGIN_ADDR origin_start, ADDR curr_start, SIZE size, 
	vector<ORIGIN_ADDR> &direct_target_out_of_func)
{
	_CodeInfo ci;
	ci.code = (const UINT8 *)curr_start;
	ci.codeLen = size;
	ci.codeOffset = origin_start;
	ci.dt = Decode64Bits;
	ci.features = DF_STOP_ON_UNC_BRANCH | DF_STOP_ON_UNC_BRANCH;
	//decompose the instruction
	do{
		UINT32 dinstcount = 0;
		_DInst _dInst[100];

		_DecodeResult ret = distorm_decompose(&ci, _dInst, 100, &dinstcount);
		ASSERT(ret==DECRES_SUCCESS);
		if(_dInst[dinstcount-1].ops[0].type==O_PC){
			ORIGIN_ADDR target_addr = INSTRUCTION_GET_TARGET(&_dInst[dinstcount-1]);
			if(target_addr<origin_start || target_addr>=(origin_start+size))
				direct_target_out_of_func.push_back(target_addr);
		}

		SIZE decode_size = ci.nextOffset - ci.codeOffset;
		ci.codeLen -= decode_size;
		ci.codeOffset = ci.nextOffset;
		ci.code  = (const UINT8 *)((ADDR)ci.code + decode_size);
	}while(ci.codeLen>0);
}

void ReadElf::scan_and_record_function(MAP_ORIGIN_FUNCTION *map_origin_function, CodeCache *cc)
{
	//record size0 function 
	vector<ORIGIN_ADDR> size0_func;
	//find function in symt
	for(INT32 idx=0; idx<symt_num; idx++){
		Elf64_Sym sym = symbol_table[idx];
		if((sym.st_info & 0xf) == STT_FUNC){
			ORIGIN_ADDR virtual_base = is_so ? elf_load_start : 0;
			ORIGIN_ADDR function_start = sym.st_value + virtual_base;
			ORIGIN_SIZE function_size = sym.st_size;
			if(function_start==0 || sym.st_value==0)
				continue;
			char *name = string_table + sym.st_name;
			string string_name = name;
			if(function_size==0){
				if(map_origin_function->insert(make_pair(function_start, (Function*)NULL)).second)
					size0_func.push_back(function_start);
				continue;
			}
			//create function
			Function *function = new Function(_code_segment, string_name, function_start, function_size, cc);
			//insert map_origin_function
			if(map_origin_function->insert(make_pair(function_start, function)).second == false){
				//key was present
				delete function;
				function = NULL;
			}else{
				//disasm function
				function->disassemble();
			}
		}   
	} 
	//find function in dynsymt
	for(INT32 idx=0; idx<dynsymt_num; idx++){
		Elf64_Sym sym = dynsym_table[idx];
		if((sym.st_info & 0xf) == STT_FUNC){
			ORIGIN_ADDR virtual_base = is_so ? elf_load_start : 0;
			ORIGIN_ADDR function_start = sym.st_value + virtual_base;
			ORIGIN_SIZE function_size = sym.st_size;
			if(function_start==0 || sym.st_value==0)
				continue;
			char *name =  dynstr_table + sym.st_name;
			string string_name = name;
			if(function_size==0){
				if(map_origin_function->insert(make_pair(function_start, (Function*)NULL)).second)
					size0_func.push_back(function_start);
				continue;
			}
			Function *function = new Function(_code_segment, string_name, function_start, function_size, cc);
			//insert map_origin_function
			if(map_origin_function->insert(make_pair(function_start, function)).second == false){
				//key was present
				delete function;
				function = NULL;
			}else{
				//disasm function
				function->disassemble();
			}	
		}   
	}

	//calculate the function with size=0
	for(vector<ORIGIN_ADDR>::iterator iter = size0_func.begin(); iter!=size0_func.end(); iter++){
		ORIGIN_ADDR func_start = *iter;
		MAP_ORIGIN_FUNCTION_ITERATOR ret = map_origin_function->find(func_start);
		ASSERT(ret!=map_origin_function->end() && ret->first==func_start && ret->second==NULL);
		MAP_ORIGIN_FUNCTION_ITERATOR next_inst = ret;
		next_inst++;
		ORIGIN_ADDR region_end = 0;
		if(func_start>=origin_text_start && func_start<origin_text_end)
			region_end = origin_text_end;
		else if(func_start>=origin_init_start && func_start<origin_init_end)
			region_end = origin_init_end;
		else if(func_start>=origin_fini_start && func_start<origin_fini_end)
			region_end = origin_fini_end;
		else
			continue;

		ORIGIN_ADDR func_end = 0;
		if(next_inst==map_origin_function->end())
			func_end = region_end;
		else
			func_end = next_inst->first;
		
		disassemble_to_find_direct_target_out_of_func(func_start, _code_segment->convert_origin_process_addr_to_this(func_start), 
			(SIZE)(func_end - func_start), _code_segment->direct_profile_func_entry);
	}
	//erase size0 function
	for(vector<ORIGIN_ADDR>::iterator iter = size0_func.begin(); iter!=size0_func.end(); iter++)
		map_origin_function->erase(*iter);
}
