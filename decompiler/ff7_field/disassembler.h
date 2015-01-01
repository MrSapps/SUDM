#pragma once

#include "simple_disassembler.h"

namespace FF7 
{
    class FF7Engine;
    class FF7Disassembler : public SimpleDisassembler 
    {
    public:
        FF7Disassembler(FF7Engine* engine, InstVec& insts);
        ~FF7Disassembler();
	    virtual void doDisassemble() throw(std::exception) override;
    };

} 
