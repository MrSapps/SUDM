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
    START_OPCODES;
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


    END_OPCODES;
}

