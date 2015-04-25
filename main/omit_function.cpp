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
};

set<string> omit_random_function_set(omit_random_function, \
	omit_random_function+sizeof(omit_random_function)/sizeof(omit_random_function[0]) );

BOOL need_omit_random_function(string function_name){
	return omit_random_function_set.find(function_name)!=omit_random_function_set.end();
}

static string omit_stack_function[]={
	"__setcontext",
	"__swapcontext",
	"__mpn_addmul_1",
	"__mpn_mul_1",
	"__mpn_submul_1",
	"__mktime_internal",
	"__GI_timelocal",
	"__GI___vfork",
	"__lll_lock_wait_private",
	"__lll_unlock_wake_private",
	"__nptl_create_event",
	"__nptl_death_event",
	"pthread_rwlock_timedrdlock",
	"pthread_rwlock_timedwrlock",
	"__pthread_cond_timedwait",
	"__GI___pthread_once",
	"sem_wait",
	"sem_timedwait",
	"__lll_lock_wait",
	"__lll_timedlock_wait",
	"__lll_unlock_wake",
	"__lll_timedwait_tid",
	"__lll_robust_lock_wait",
	"__lll_robust_timedlock_wait",
	"vfork",
	"__pthread_cond_wait",
	"__read_nocancel",
	"__GI___open64",
};

set<string> omit_stack_function_set(omit_stack_function, \
	omit_stack_function+sizeof(omit_stack_function)/sizeof(omit_stack_function[0]) );

BOOL need_omit_stack_function(string function_name){
	return omit_stack_function_set.find(function_name)!=omit_stack_function_set.end();
}

