#ifndef _READLOG_H_
#define _READLOG_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "code_segment_management.h"
#include <fstream>
#include <vector>
#include <iostream>
using namespace std;

class ReadLog
{
private:
	INT32 code_cache_idx;
	string share_log_name;
	BOOL share_log_is_init;
	BOOL profile_log_is_init;
	string profile_log_name;
public:
	ReadLog(const char *share_log, const char *profile_log)
		: code_cache_idx(-1), share_log_is_init(false), profile_log_is_init(false)
	{
		share_log_name = share_log;
		profile_log_name = profile_log;
	}
	void init_share_log()
	{
		//1.create ifstream file
		ifstream ifs(share_log_name.c_str(), ifstream::in);
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
		share_log_is_init = true;
	}
	CodeSegment *find_cs(string name)
	{
		for(vector<CodeSegment*>::iterator iter = code_segment_vec.begin(); iter!=code_segment_vec.end(); iter++){
			if(((*iter)->file_path).find(name) != string::npos){
				return *iter;
			}
		}
		ASSERT(0);
	}
	void init_profile_log()
	{
		ASSERT(share_log_is_init);
		//1.create ifstream file
		ifstream ifs(profile_log_name.c_str(), ifstream::in);
		//2.read log
		INT32 image_num = 0;
		string str_image_num;
		INT32 num;
		
		ifs>>str_image_num>>image_num;
		CodeSegment **array = new CodeSegment*[image_num+1];
		for(INT32 idx=1; idx<=image_num; idx++){
			string image_path, name;
			ifs>>num>>image_path;
			unsigned found = image_path.find_last_of("/");
			if(found==string::npos)
				name = image_path;
			else
				name = image_path.substr(found+1);

			array[idx] = find_cs(name);
		}

		string padding;
		char c;
		INT32 inst_num = 0;
		ifs>>hex>>padding>>inst_num;
		for(INT32 idx=1; idx<=inst_num; idx++){
			SIZE inst, target;
			INT32 inst_idx, target_idx;
			ifs>>hex>>inst>>c>>inst_idx>>padding>>target>>c>>target_idx>>c;
			if(inst_idx==target_idx){
				CodeSegment *cs = array[inst_idx];
				cs->indirect_inst_map.insert(make_pair(cs->code_start+inst, cs->code_start+target));
			}else{
				//TODO: 
				//cout<<hex<<inst<<"("<<inst_idx<<")==>"<<target<<"("<<target_idx<<")"<<endl;
			}
		}
	}
};

#endif
