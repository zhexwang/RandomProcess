#ifndef _READLOG_H_
#define _READLOG_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "code_segment_management.h"
#include "codecache.h"
#include "stack.h"
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;

extern ShareStack *main_share_stack ;
extern CodeCacheManagement *cc_management;
typedef enum {
	LIBC = 0,
	LIBM,
	LD,
	LIB_NONE,
}__attribute((packed))LIB_IDX;

typedef struct IndirectLogItem{
	UINT32 indirect_inst_offset;
	UINT32 indirect_target_offset;
	LIB_IDX indirect_inst_lib;
	LIB_IDX indirect_target_lib;
}INDIRECT_ITEM;

class ReadLog
{
public:
	static const char *lib_name[];
	static const INDIRECT_ITEM indirect_profile_by_hand[];
private:	
	INT32 shm_num;
	string share_log_name;
	BOOL share_log_is_init;
	BOOL profile_log_is_init;
	string profile_log_name;
public:
	ReadLog(const char *share_log, const char *profile_log)
		: shm_num(-1), share_log_is_init(false), profile_log_is_init(false)
	{
		share_log_name = share_log;
		profile_log_name = profile_log;
	}
	void init_share_log();
	CodeSegment *find_cs(string name)
	{
		for(vector<CodeSegment*>::iterator iter = code_segment_vec.begin(); iter!=code_segment_vec.end(); iter++){
			if(((*iter)->file_path).find(name) != string::npos){
				return *iter;
			}
		}
		ASSERT(0);
	}

	CodeSegment *find_cs(LIB_IDX lib_idx)
	{
		for(vector<CodeSegment*>::iterator iter = code_segment_vec.begin(); iter!=code_segment_vec.end(); iter++){
			if(((*iter)->file_path).find(lib_name[(UINT32)lib_idx]) != string::npos){
				return *iter;
			}
		}
		return NULL;
	}
	
	void init_profile_log();
};

#endif
