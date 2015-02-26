#ifndef _MAP_FUNCTION_H_
#define _MAP_FUNCTION_H_

#include "code_segment.h"
#include "type.h"
#include <map>
using namespace std;

class Function;

typedef map<ORIGIN_ADDR, Function *> MAP_ORIGIN_FUNCTION;
typedef pair<ORIGIN_ADDR, Function *> MAP_ORIGIN_FUNCTION_PAIR;
typedef map<ORIGIN_ADDR, Function *>::iterator MAP_ORIGIN_FUNCTION_ITERATOR;
typedef map<CodeSegment *, MAP_ORIGIN_FUNCTION *> CODE_SEG_MAP_ORIGIN_FUNCTION;
typedef pair<CodeSegment *, MAP_ORIGIN_FUNCTION *> CODE_SEG_MAP_ORIGIN_FUNCTION_PAIR;
typedef map<CodeSegment *, MAP_ORIGIN_FUNCTION *>::iterator CODE_SEG_MAP_ORIGIN_FUNCTION_ITERATOR;


typedef map<ADDR, Function *> MAP_FUNCTION;
typedef pair<ADDR, Function *> MAP_FUNCTION_PAIR;
typedef map<ADDR, Function *>::iterator MAP_FUNCTION_ITERATOR;
typedef map<CodeSegment *, MAP_FUNCTION *> CODE_SEG_MAP_FUNCTION;
typedef pair<CodeSegment *, MAP_FUNCTION *> CODE_SEG_MAP_FUNCTION_PAIR;

typedef map<CODE_CACHE_ADDR, Function *> MAP_CC_FUNCTION;
typedef map<CODE_CACHE_ADDR, Function *>::iterator MAP_CC_FUNCTION_ITERATOR;
typedef map<CodeSegment *, MAP_CC_FUNCTION *> CODE_SEG_MAP_CC_FUNCTION;

extern void dump_function_origin_list(CODE_SEG_MAP_ORIGIN_FUNCTION *CS_origin_function_map);
#endif
