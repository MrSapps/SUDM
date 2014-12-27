#pragma once

#include "simple_disassembler.h"
#include "common/endianness.h"

namespace FF7 
{
    class FF7Engine;
    class FF7Disassembler : public SimpleDisassembler 
    {
    public:
        FF7Disassembler(FF7Engine* engine, InstVec& insts);
        ~FF7Disassembler();
	    void doDisassemble() throw(std::exception);
    };

} 
