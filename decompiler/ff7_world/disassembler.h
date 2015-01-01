#pragma once

#include "simple_disassembler.h"

namespace FF7 
{
    class FF7WorldEngine;
    class FF7WorldDisassembler : public SimpleDisassembler
    {
    public:
        FF7WorldDisassembler(FF7WorldEngine* engine, InstVec& insts, int scriptNumber);
        ~FF7WorldDisassembler();
	    void doDisassemble() throw(std::exception) override;
    private:
        int mScriptNumber = 0;
    };

} 
