#ifndef _CODE_CACHE_H_
#define _CODE_CACHE_H_

#include "type.h"
#include "utility.h"
#include "code_segment.h"
#include "disasm_common.h"
#include <string.h>

class CodeCache
{
private:
	UINT8 curr_idx;
	CODE_CACHE_ADDR code_cache_start[2];
	CODE_CACHE_ADDR code_cache_end[2];
	CODE_CACHE_ADDR current_code_cache_ptr[2];
	ORIGIN_ADDR origin_process_code_cache_start[2];
	ORIGIN_ADDR origin_process_code_cache_end[2];
	ORIGIN_ADDR origin_process_code_cache_ptr[2];
public:
	CodeCache(CodeSegment &codeSegment)
	{
		ASSERT(codeSegment.is_code_cache);
		curr_idx = 1;
		
		code_cache_start[0] = reinterpret_cast<ADDR>(codeSegment.native_map_code_start);
		code_cache_end[0] = code_cache_start[0]+codeSegment.code_size/2;
		current_code_cache_ptr[0] = code_cache_start[0];
		
		code_cache_start[1] = code_cache_end[0];
		code_cache_end[1] = code_cache_start[1]+codeSegment.code_size/2;
		current_code_cache_ptr[1] = code_cache_start[1];
		
		origin_process_code_cache_start[0] = codeSegment.code_start;
		origin_process_code_cache_end[0] = origin_process_code_cache_start[0]+codeSegment.code_size/2;
		origin_process_code_cache_ptr[0] = origin_process_code_cache_start[0];

		origin_process_code_cache_start[1] = origin_process_code_cache_end[0];
		origin_process_code_cache_end[1] = origin_process_code_cache_start[1]+codeSegment.code_size/2;
		origin_process_code_cache_ptr[1] = origin_process_code_cache_start[1];
	}

	ORIGIN_ADDR get_origin_process_addr(CODE_CACHE_ADDR code_cache_addr)
	{
		ASSERT((code_cache_addr>=code_cache_start[curr_idx])&&(code_cache_addr<=code_cache_end[curr_idx]));
		return code_cache_addr - code_cache_start[curr_idx] + origin_process_code_cache_start[curr_idx];
	}

	void freeCC(CODE_CACHE_ADDR curr_addr, ORIGIN_ADDR origin_addr, SIZE cc_size)
	{
		ASSERT((curr_addr-code_cache_start[curr_idx]+origin_process_code_cache_start[curr_idx])==curr_addr);
	}
	
	void updateCC(SIZE used_size)
	{
		current_code_cache_ptr[curr_idx] += used_size;
		origin_process_code_cache_ptr[curr_idx] += used_size;
		ASSERT((current_code_cache_ptr[curr_idx]<=code_cache_end[curr_idx]) && \
			(origin_process_code_cache_ptr[curr_idx]<=origin_process_code_cache_end[curr_idx]));
	}

	void getCCCurrent(CODE_CACHE_ADDR &curr_cc_ptr, ORIGIN_ADDR &origin_cc_ptr)
	{
		ASSERT((current_code_cache_ptr[curr_idx]<=code_cache_end[curr_idx]) && \
			(origin_process_code_cache_ptr[curr_idx]<=origin_process_code_cache_end[curr_idx]));
		curr_cc_ptr = current_code_cache_ptr[curr_idx];
		origin_cc_ptr = origin_process_code_cache_ptr[curr_idx];
	}

	void flush()
	{
		curr_idx = curr_idx==0 ? 1 : 0;
		current_code_cache_ptr[curr_idx] = code_cache_start[curr_idx];
		origin_process_code_cache_ptr[curr_idx] = origin_process_code_cache_start[curr_idx];
		/*
		current_code_cache_ptr[1] = code_cache_start[1];
		origin_process_code_cache_ptr[1] = origin_process_code_cache_start[1];
		*/
	}

	void erase_old_cc()
	{
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		memset((void*)code_cache_start[old_idx], 0xd6, (SIZE)(code_cache_end[old_idx] - code_cache_start[old_idx]));
		current_code_cache_ptr[old_idx] = code_cache_start[old_idx];
		origin_process_code_cache_ptr[old_idx] = origin_process_code_cache_start[old_idx];
	}
	BOOL isInCC(ORIGIN_ADDR origin_addr)
	{
		return ((origin_addr>=origin_process_code_cache_start[0])&&(origin_addr<=origin_process_code_cache_end[1]));
	}
	
	BOOL isInOldCC(ORIGIN_ADDR origin_addr)
	{
		UINT8 old_idx = curr_idx==0 ? 1 : 0;
		return (origin_addr>=origin_process_code_cache_start[old_idx]) && (origin_addr<origin_process_code_cache_end[old_idx]);
	}

	BOOL isInCurrentCC(ORIGIN_ADDR origin_addr)
	{
		return (origin_addr>=origin_process_code_cache_start[curr_idx]) && (origin_addr<origin_process_code_cache_end[curr_idx]);
	}
	
	void disassemble(const char prev_info[], CODE_CACHE_ADDR start, CODE_CACHE_ADDR end)
	{
		ASSERT(end<=current_code_cache_ptr[curr_idx]);
		_OffsetType origin_start = start-code_cache_start[curr_idx]+origin_process_code_cache_start[curr_idx];
		//_DecodeResult res;
		_DecodedInst  *disassembled = new _DecodedInst[end-start];
		UINT32 decodedInstsCount = 0;
		distorm_decode(origin_start, (const unsigned char*)start, end-start, Decode64Bits, disassembled, end-start, &decodedInstsCount);
		for(UINT32 i=0; i<decodedInstsCount; i++){
			PRINT("%s%12lx (%02d) %-24s  %s%s%s\n", prev_info, disassembled[i].offset, disassembled[i].size, disassembled[i].instructionHex.p,\
				(char*)disassembled[i].mnemonic.p, disassembled[i].operands.length != 0 ? " " : "", (char*)disassembled[i].operands.p);
		}
		delete [] disassembled;
	}
};

class CodeCacheManagement
{
private:
	vector<CodeCache*> cc_vec;
public:
	CodeCacheManagement(){;}
	void insert(CodeCache *cc)
	{
		cc_vec.push_back(cc);
	}
	void flush()
	{
		for(vector<CodeCache*>::iterator iter = cc_vec.begin(); iter!=cc_vec.end(); iter++){
			(*iter)->flush();
		}
	}
	BOOL is_in_old_cc(ORIGIN_ADDR origin_inst_addr)
	{
		for(vector<CodeCache*>::iterator iter = cc_vec.begin(); iter!=cc_vec.end(); iter++){
			if((*iter)->isInOldCC(origin_inst_addr))
				return true;
		}
		return false;
	}
	BOOL is_in_curr_cc(ORIGIN_ADDR origin_inst_addr)
	{
		for(vector<CodeCache*>::iterator iter = cc_vec.begin(); iter!=cc_vec.end(); iter++){
			if((*iter)->isInCurrentCC(origin_inst_addr))
				return true;
		}
		return false;
	}
	BOOL is_in_cc(ORIGIN_ADDR origin_inst_addr)
	{
		for(vector<CodeCache*>::iterator iter = cc_vec.begin(); iter!=cc_vec.end(); iter++){
			if((*iter)->isInCC(origin_inst_addr))
				return true;
		}
		return false;
	}
};

#endif
