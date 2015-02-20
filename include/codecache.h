#ifndef _CODE_CACHE_H_
#define _CODE_CACHE_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "disasm_common.h"

class CodeCache
{
private:
	CODE_CACHE_ADDR code_cache_start;
	CODE_CACHE_ADDR code_cache_end;
	CODE_CACHE_ADDR current_code_cache_ptr;
	ORIGIN_ADDR origin_process_code_cache_start;
	ORIGIN_ADDR origin_process_code_cache_end;
	ORIGIN_ADDR origin_process_code_cache_ptr;
public:
	CodeCache(CodeSegment &codeSegment)
	{
		ASSERT(codeSegment.is_code_cache);
		code_cache_start = reinterpret_cast<ADDR>(codeSegment.native_map_code_start);
		code_cache_end = code_cache_start+codeSegment.code_size;
		current_code_cache_ptr = code_cache_start;
		origin_process_code_cache_start = codeSegment.code_start;
		origin_process_code_cache_end = origin_process_code_cache_start+codeSegment.code_size;
		origin_process_code_cache_ptr = origin_process_code_cache_start;
	}

	ORIGIN_ADDR get_origin_process_addr(CODE_CACHE_ADDR code_cache_addr)
	{
		ASSERT((code_cache_addr>=code_cache_start)&&(code_cache_addr<=code_cache_end));
		return code_cache_addr - code_cache_start + origin_process_code_cache_start;
	}

	void updateCC(SIZE used_size)
	{
		current_code_cache_ptr += used_size;
		origin_process_code_cache_ptr += used_size;
		ASSERT((current_code_cache_ptr<=code_cache_end) && (origin_process_code_cache_ptr<=origin_process_code_cache_end));
	}

	void getCCCurrent(CODE_CACHE_ADDR &curr_cc_ptr, ORIGIN_ADDR &origin_cc_ptr)
	{
		ASSERT((current_code_cache_ptr<=code_cache_end) && (origin_process_code_cache_ptr<=origin_process_code_cache_end));
		curr_cc_ptr = current_code_cache_ptr;
		origin_cc_ptr = origin_process_code_cache_ptr;
	}

	void emptyCC()
	{
		current_code_cache_ptr = 0;
		origin_process_code_cache_ptr = 0;
	}
	
	void disassemble(CODE_CACHE_ADDR start, CODE_CACHE_ADDR end)
	{
		ASSERT(end<=current_code_cache_ptr);
		_OffsetType origin_start = start-code_cache_start+origin_process_code_cache_start;
		//_DecodeResult res;
		_DecodedInst  *disassembled = new _DecodedInst[end-start];
		UINT32 decodedInstsCount = 0;
		distorm_decode(origin_start, (const unsigned char*)start, end-start, Decode64Bits, disassembled, end-start, &decodedInstsCount);
		for(UINT32 i=0; i<decodedInstsCount; i++){
			INFO("%12lx (%02d) %-24s  %s%s%s\n", disassembled[i].offset, disassembled[i].size, disassembled[i].instructionHex.p,\
				(char*)disassembled[i].mnemonic.p, disassembled[i].operands.length != 0 ? " " : "", (char*)disassembled[i].operands.p);
		}
		delete [] disassembled;
	}
};



#endif
