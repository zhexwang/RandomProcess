#include "type.h"
#include <string>
#include <set>
using namespace std;

static const char *omit_function[]={
	"__memcpy_ssse3_back",
};

set<const char*> omit_set(omit_function, omit_function+sizeof(omit_function)/sizeof(const char *) );

BOOL need_omit_function(string function_name){
	return omit_set.find(function_name.c_str())!=omit_set.end();

}
