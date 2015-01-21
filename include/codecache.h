#ifndef _CODE_CACHE_H_
#define _CODE_CACHE_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"

class CodeCache
{
private:
	CODE_CACHE_ADDR code_cache_start;
	CODE_CACHE_ADDR code_cache_end;
	CODE_CACHE_ADDR current_code_cache_ptr;
	ORIGIN_ADDR origin_process_code_cache_start;
	ORIGIN_ADDR origin_process_code_cache_end;
public:
	CodeCache(CodeSegment &codeSegment)
	{
		ASSERT(codeSegment.is_code_cache);
		code_cache_start = codeSegment.code_start;
		code_cache_end = code_cache_start+codeSegment.code_size;
		current_code_cache_ptr = code_cache_start;
		origin_process_code_cache_start = reinterpret_cast<ADDR>(codeSegment.native_map_code_start);
		origin_process_code_cache_end = origin_process_code_cache_start+codeSegment.code_size;
	}

	ORIGIN_ADDR get_origin_process_addr(CODE_CACHE_ADDR code_cache_addr)
	{
		ASSERT((code_cache_addr>=code_cache_start)&&(code_cache_addr<=code_cache_end));
		return code_cache_addr - code_cache_start + origin_process_code_cache_start;
	}

};



#endif
