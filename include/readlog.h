#ifndef _READLOG_H_
#define _READLOG_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "code_segment_management.h"
#include <fstream>
#include <vector>
using namespace std;

class ReadLog
{
private:
	INT32 code_cache_idx;
public:
	ReadLog(const char *log_name)
	{
		//1.create ifstream file
		ifstream ifs(log_name, ifstream::in);
		//2.read log 
		string code_cache_info;
		ifs>>code_cache_info>>code_cache_idx;
		for(INT32 idx=0; idx<=code_cache_idx; idx++){
			ORIGIN_ADDR region_start;
			ORIGIN_ADDR region_end;
			string shm_name;
			string code_path;
			char c;
			BOOL isCodeCache;
			if(idx==code_cache_idx)
				isCodeCache = true;
			else
				isCodeCache = false;
			ifs>>hex>>region_start>>c>>region_end>>shm_name>>code_path;
			CodeSegment *cs = new CodeSegment(region_start, region_end-region_start, code_path, shm_name, isCodeCache);
			code_segment_vec.push_back(cs);
			if(isCodeCache)
				code_cache_segment = cs;
		}
	}
};

#endif
