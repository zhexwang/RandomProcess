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
using namespace std;

class CodeSegment
{
public:
	ORIGIN_ADDR code_start;
	ORIGIN_SIZE code_size;
	string file_path;
	string shm_name;
	void *native_map_code_start;
	BOOL is_code_cache;
	BOOL isSO;
	INT32 shm_fd;
public:
	CodeSegment(ORIGIN_ADDR regionStart, SIZE regionSize, string codePath, string shmName, BOOL isCodeCache)
		:code_start(regionStart), code_size(regionSize), file_path(codePath), shm_name(shmName),is_code_cache(isCodeCache)
	{
		open_shm();
		isSO = is_so_file();
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
		if(string::npos == file_path.find(".so"))
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
};

#endif
