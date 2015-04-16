#include "type.h"
#include "utility.h"
#include <string>
#include <set>
using namespace std;

static string omit_function[]={
	"__memcpy_ssse3_back",
	"__strncmp_sse42",
	"__strcpy_sse2_unaligned",
};

set<string> omit_set(omit_function, omit_function+sizeof(omit_function)/sizeof(omit_function[0]) );

BOOL need_omit_function(string function_name){
	return omit_set.find(function_name)!=omit_set.end();
}
