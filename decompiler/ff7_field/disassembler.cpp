#include "disassembler.h"
#include "engine.h"
#include <boost/format.hpp>

FF7::FF7Disassembler::FF7Disassembler(FF7Engine *engine, InstVec &insts)
    : SimpleDisassembler(insts)
{

}

FF7::FF7Disassembler::~FF7Disassembler() 
{

}

void FF7::FF7Disassembler::doDisassemble() throw(std::exception)
{
    // Firstly decompress the whole file


    this->_address = this->_addressBase;
    while (this->_f.pos() != (int)this->_f.size())
    {
        uint32 full_opcode = 0;
        uint8 opcode = this->_f.readByte();
        std::string opcodePrefix;
        switch (opcode)
        {
            OPCODE(0x14, "IFUB", FF7CondJumpInstruction, 0, "BBBBB");
            OPCODE(0x12, "JMPB", FF7UncondJumpInstruction, 0, "B");
            OPCODE(0x0, "RET", FF7KernelCallInstruction, 0, "");
            OPCODE(0xE5, "STPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xEA, "MPPAL2", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0x24, "WAIT", FF7KernelCallInstruction, 0, "W");
            OPCODE(0xE7, "CPPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xE6, "LDPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0x87, "MINUS", FF7StoreInstruction, 0, "BBB");
            OPCODE(0x85, "PLUS", FF7StoreInstruction, 0, "BBB");
            OPCODE(0x80, "SETBYTE", FF7StoreInstruction, 0, "BBB");
        default:
            throw UnknownOpcodeException(this->_address, opcode);
        }
        INC_ADDR;
    }
}