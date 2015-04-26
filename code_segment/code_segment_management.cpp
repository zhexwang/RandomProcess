#include "code_segment.h"
#include "codecache.h"
#include <vector>
using namespace std;

vector<CodeSegment*> code_segment_vec;

void dump_code_segment()
{
	INFO("===============================dump code segment================================\n");
	INFO("origin_start       origin_end        shm_name        code_file_path       isCodeCache   isStack   isSO    is_libsc_privated    native_map_start\n");
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it!=code_segment_vec.end(); it++){
		PRINT("0x%lx-0x%lx %s %s %s %s %s %s %p\n", (*it)->code_start, (*it)->code_size + (*it)->code_start, (*it)->shm_name.c_str(), (*it)->file_path.c_str(),
			btoa((*it)->is_code_cache), btoa((*it)->is_stack),  btoa((*it)->isSO), btoa((*it)->is_libsc_privated), (*it)->native_map_code_start);
	}
	INFO("==================================END=======================================\n");
}
