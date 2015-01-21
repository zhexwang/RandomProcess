
#include "readlog.h"
#include "utility.h"
#include "code_segment_management.h"
#include "function.h"
#include "readelf.h"
#include <iostream>
using namespace std;

CodeCache *global_code_cache = NULL;
vector<Function*> functionList;

int main(int argc, const char *argv[])
{
	//1.judge illegal
	if(argc<1){
		ERR("Usage: ./%s logFile", argv[0]);
		abort();
	}
	//2.read log file and create code_segment_vec
	ReadLog log(argv[1]);
	//3.init code cache
	global_code_cache = init_code_cache();
	//4.read elf to find function
	for(vector<CodeSegment*>::iterator it = code_segment_vec.begin(); it<code_segment_vec.end(); it++){
		if(!(*it)->is_code_cache){
			ReadElf *read_elf = new ReadElf(*it);
			read_elf->scan_and_record_function(functionList);
			delete read_elf;
		}
	}
	return 0;
}
