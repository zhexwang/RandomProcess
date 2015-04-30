#include "type.h"
#include "utility.h"
#include <string>
#include <set>
using namespace std;

static string omit_random_function[]={
	"__memcpy_ssse3_back",
	"__strncmp_sse42",
	"__strcpy_sse2_unaligned",
	"__memset_sse2",
	"__memmove_ssse3_back",
	"__strcat_sse2_unaligned",
	//gdb debug
	"__nptl_create_event",
	"__nptl_death_event",
	"_IO_vfprintf_internal",
	"__arch_prctl",
	"arch_prctl",
	"__GI_arch_prctl",
	"__GI___arch_prctl",
};

set<string> omit_random_function_set(omit_random_function, \
	omit_random_function+sizeof(omit_random_function)/sizeof(omit_random_function[0]) );

BOOL need_omit_random_function(string function_name){
	return omit_random_function_set.find(function_name)!=omit_random_function_set.end();
}

static string omit_stack_function[]={
	"__libc_csu_init",
	"__ieee754_powl",
	"_Z12CumNormalInvd",
	"parse_cqm",
	"SHA1_Final",
	"SHA1_Update",
	"sha1_block_data_order",
	
};

set<string> omit_stack_function_set(omit_stack_function, \
	omit_stack_function+sizeof(omit_stack_function)/sizeof(omit_stack_function[0]) );

BOOL need_omit_stack_function(string function_name){
	return omit_stack_function_set.find(function_name)!=omit_stack_function_set.end();
}

static string omit_analysis_stack_so[]={
	"ld-2.17.so",
	"libpthread.so.0",
	"libc.so.6",
	"libgfortran.so.3.0.0",
};

set<string> omit_analysis_stack_set(omit_analysis_stack_so, \
	omit_analysis_stack_so+sizeof(omit_analysis_stack_so)/sizeof(omit_analysis_stack_so[0]) );

BOOL need_omit_analysis_stack_cs(string cs_name){
	unsigned found = cs_name.find_last_of("/");
	string name;
	if(found==string::npos)
		name = cs_name;
	else
		name = cs_name.substr(found+1);
	return omit_analysis_stack_set.find(name)!=omit_analysis_stack_set.end();
}

