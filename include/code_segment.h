#ifndef _CODE_SEGMENT_H_
#define _CODE_SEGMENT_H_

#include "type.h"
#include "utility.h"
#include <sys/mman.h>
#include <sys/stat.h>		 /* For mode constants */
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>			 /* For O_* constants */
#include <string>
#include <map>
#include <vector>
using namespace std;

class CodeCache;

class CodeSegment
{
public:
	ORIGIN_ADDR code_start;
	ORIGIN_SIZE code_size;
	string file_path;
	string shm_name;
	void *native_map_code_start;
	BOOL is_code_cache;
	BOOL is_stack;
	BOOL isSO;
	INT32 shm_fd;
	CodeCache *code_cache;
	//profile data
	multimap<ORIGIN_ADDR, ORIGIN_ADDR> indirect_inst_map;
	typedef multimap<ORIGIN_ADDR, ORIGIN_ADDR>::iterator INDIRECT_MAP_ITERATOR;
	//direct inst target
	vector<ORIGIN_ADDR> direct_profile_func_entry;
public:
	CodeSegment(ORIGIN_ADDR regionStart, SIZE regionSize, string codePath, string shmName, BOOL isCodeCache, BOOL isStack)
		:code_start(regionStart), code_size(regionSize), file_path(codePath), shm_name(shmName),is_code_cache(isCodeCache), is_stack(isStack)
		, code_cache(NULL)
	{
		open_shm();

		if(is_code_cache || is_stack)
			isSO = false;
		else
			isSO = is_so_file();

	}
	
	void map_CC_to_CS(CodeCache *cc)
	{
		if(is_code_cache || is_stack){
			ASSERT(!cc);
		}else
			code_cache = cc;
	}
	
	void open_shm()
	{
		//1. open shm
		shm_fd = shm_open(shm_name.c_str(), O_RDWR|O_CREAT, 0644);
		PERROR(shm_fd!=-1, "shm_open failed!");
		//2.judge file size
		struct stat statbuf;
		INT32 ret = fstat(shm_fd, &statbuf);
		ASSERTM(ret==0, "get log file information failed!\n");
		ASSERT(statbuf.st_size==(off_t)code_size);
		//3.map file 
		native_map_code_start = mmap(NULL, code_size, PROT_WRITE|PROT_READ, MAP_SHARED, shm_fd, 0);
		PERROR(native_map_code_start!=MAP_FAILED, "mmap failed!");
	}
	BOOL is_so_file()
	{
		if(string::npos == shm_name.find(".so."))
			return false;
		else
			return true;
			
	}
	ADDR convert_origin_process_addr_to_this(ORIGIN_ADDR origin_addr)
	{
		ASSERT((origin_addr>=code_start)&&(origin_addr<=(code_start+code_size)));
		return origin_addr - code_start + reinterpret_cast<ADDR>(native_map_code_start);
	}
	ORIGIN_ADDR convert_addr_to_origin_process(ADDR addr)
	{
		ADDR start = reinterpret_cast<ADDR>(native_map_code_start);
		ADDR end = start + (SIZE)code_size;
		ASSERT((addr>=start)&&(addr<=end));
		return addr - start + code_start;
	}
	BOOL has_profile_data()
	{
		return !indirect_inst_map.empty();
	}
	vector<ORIGIN_ADDR> *find_target_by_inst_addr(ORIGIN_ADDR inst_addr)
	{
		vector<ORIGIN_ADDR> *ret = new vector<ORIGIN_ADDR>();
		pair<INDIRECT_MAP_ITERATOR, INDIRECT_MAP_ITERATOR> range = indirect_inst_map.equal_range(inst_addr);
		for(INDIRECT_MAP_ITERATOR iter = range.first; iter!=range.second; iter++){
			ret->push_back(iter->second);
		}
		return ret;
	}
	BOOL is_in_cs(ORIGIN_ADDR addr)
	{
		return addr>=code_start && addr<(code_start+code_size);
	}
};

#endif
