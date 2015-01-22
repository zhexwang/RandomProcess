#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include "instruction.h"
using namespace std;

class BasicBlock
{
private:
	vector<Instruction *> instruction_list;
	vector<BasicBlock *> prev_bb_list;
	BasicBlock *fallthrough_bb;
	BasicBlock *target_bb;
public:
	BasicBlock();
	~BasicBlock();
};


#endif
