#include "readelf.h"
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

ReadElf::ReadElf(CodeSegment *code_segment): elf_name(code_segment->file_path), is_so(code_segment->isSO)
	, elf_load_start(code_segment->code_start), _code_segment(code_segment)
{
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
}

ReadElf::~ReadElf()
{
	INT32 ret = close(elf_fd);
	PERROR(ret==0, "close failed!");
	ret = munmap(map_start, elf_size);
	PERROR(ret==0, "munmap failed!");
}

void ReadElf::scan_and_record_function(MAP_FUNCTION *map_function, MAP_ORIGIN_FUNCTION *map_origin_function)
{
	Elf64_Ehdr *elf_header = (Elf64_Ehdr*)map_start;
	Elf64_Shdr *SecHdr = (Elf64_Shdr *)((ADDR)map_start + elf_header->e_shoff);
	char *SecHdrStrTab = (char *)((ADDR)map_start + SecHdr[elf_header->e_shstrndx].sh_offset);
	Elf64_Sym *symbol_table = NULL;
	Elf64_Sym *dynsym_table = NULL;
	char *string_table = NULL;
	char *dynstr_table = NULL;
	INT32 symt_num = 0;
	INT32 dynsymt_num = 0;
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
		}
	}       
	//find function in symt
	for(INT32 idx=0; idx<symt_num; idx++){
		Elf64_Sym sym = symbol_table[idx];
		if((sym.st_info & 0xf) == STT_FUNC){
			ORIGIN_ADDR virtual_base = is_so ? elf_load_start : 0;
			ORIGIN_ADDR function_start = sym.st_value + virtual_base;
			ORIGIN_SIZE function_size = sym.st_size;
			if(function_start==0 || function_size==0)
				continue;
			char *name = string_table + sym.st_name;
			string string_name = name;
			//create function
			Function *function = new Function(_code_segment, string_name, function_start, function_size);
			//get function and origin function base
			ADDR current_function_base = function->get_function_base();
			ORIGIN_ADDR origin_function_base = function->get_origin_function_base();
			//insert map_function
			if(map_function->insert(make_pair(current_function_base, function)).second == false){
				//key was present
				delete function;
				function = NULL;
			}
			//insert map_origin function
			if(map_origin_function->insert(make_pair(origin_function_base, function)).second == false){
				ASSERT(!function);
			}else{
				ASSERT(function);
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
			if(function_start==0 || function_size==0)
				continue;
			char *name =  dynstr_table + sym.st_name;
			string string_name = name;
			Function *function = new Function(_code_segment, string_name, function_start, function_size);
			//get function and origin function base
			ADDR current_function_base = function->get_function_base();
			ORIGIN_ADDR origin_function_base = function->get_origin_function_base();
			//insert map_function
			if(map_function->insert(make_pair(current_function_base, function)).second == false){
				//key was present
				delete function;
				function = NULL;
			}
			//insert map_origin function
			if(map_origin_function->insert(make_pair(origin_function_base, function)).second == false){
				ASSERT(!function);
			}else{
				ASSERT(function);
			}		
		}   
	}
}