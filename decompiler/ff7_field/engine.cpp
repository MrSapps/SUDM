#include "engine.h"
#include "disassembler.h"
#include "codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

Disassembler* FF7::FF7Engine::getDisassembler(InstVec &insts)
{
    return new FF7Disassembler(this, insts);
}

CodeGenerator* FF7::FF7Engine::getCodeGenerator(std::ostream &output)
{
    return new FF7CodeGenerator(this, output);
}

void FF7::FF7Engine::postCFG(InstVec &insts, Graph g)
{
    VertexRange vr = boost::vertices(g);
    for (VertexIterator v = vr.first; v != vr.second; ++v) 
    {
        GroupPtr gr = GET(*v);

        // If this group is the last instruction and its an unconditional jump
        if ((*gr->_start)->_address == insts.back()->_address && insts.back()->isUncondJump())
        {
            // Then assume its an infinite do { } while(true) loop that wraps part of the script
            gr->_type = kDoWhileCondGroupType;
        }
    }
}

bool FF7::FF7Engine::detectMoreFuncs() const
{
	return false;
}

void FF7::FF7Engine::getVariants(std::vector<std::string> &variants) const
{

}

void FF7::FF7LoadInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7StoreInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7StackInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7CondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    stack.push(new VarValue("foo"));
}

uint32 FF7::FF7CondJumpInstruction::getDestAddress() const
{
    return _address + _params[4]->getUnsigned() + 5;
}

std::ostream& FF7::FF7CondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    output << " (False target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}



bool FF7::FF7UncondJumpInstruction::isFuncCall() const
{
	return _isCall;
}

bool FF7::FF7UncondJumpInstruction::isUncondJump() const
{
	return !_isCall;
}

uint32 FF7::FF7UncondJumpInstruction::getDestAddress() const
{
	return _address - _params[0]->getUnsigned();
}

std::ostream& FF7::FF7UncondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    output << " (Jump target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}


void FF7::FF7UncondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7KernelCallInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    std::string strName = "UnknownKernelFunction_" + std::to_string(_opcode);
    switch (_opcode)
    {
    case 0xE5:
        strName = "SetPalette";
        break;
    }
    codeGen->addOutputLine(strName + "();");
}

void FF7::FF7NoOutputInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}
