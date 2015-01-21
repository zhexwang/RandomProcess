#ifndef _CODE_SEGMENT_MANAGEMENT_H_
#define _CODE_SEGMENT_MANAGEMENT_H_

#include "codecache.h"
#include <vector>

extern vector<CodeSegment*> code_segment_vec;
extern CodeSegment *code_cache_segment;
extern CodeCache *init_code_cache();
extern void dump_code_segment();

#endif