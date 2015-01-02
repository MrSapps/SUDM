#include "engine.h"
#include "disassembler.h"
#include "codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

unsigned int Nib1(unsigned int v)
{
    return (v & 0xF);
}

unsigned int Nib2(unsigned int v)
{
    return (v >> 4) & 0xF;
}

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
    switch (_opcode)
    {
    case 0x80: // set byte
        codeGen->addOutputLine("var_"
            + std::to_string(_params[0]->getUnsigned())
            + "_" +
            std::to_string(_params[1]->getUnsigned())
            + "="
            + std::to_string(_params[2]->getUnsigned()) + ";");
        break;

    case 0x87:
        codeGen->addOutputLine(
            "var_" +
            _params[0]->getString() + "_" +
            _params[1]->getString() + " -= " +
            _params[2]->getString() );
        break;

    case 0x85:
        codeGen->addOutputLine(
        "var_" +
            _params[0]->getString() + "_" +
            _params[1]->getString() + " += " +
            _params[2]->getString() );
        break;

    default:
        codeGen->addOutputLine("unknown store");
        break;
    }
}

void FF7::FF7StackInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7CondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    std::string op;
    switch (_params[3]->getUnsigned())
    {
    case 0:
        op = "==";
        break;
         
    case 1:
        op = "!=";
        break;

    case 2:
        op = ">";
        break;

    case 3:
        op = "<";
        break;

    case 4:
        op = ">=";
        break;

    case 5:
        op = "<=";
        break;

    case 6:
        op = "&";
        break;

    case 7:
        op = "^";
        break;

    case 8:
        op = "|";
        break;

    case 9:
        op = "BitOn";
        break;

    case 0xA:
        op = "BitOff";
        break;

    default:
        throw std::runtime_error("unknown op");
    }

    ValuePtr v = new BinaryOpValue(
        new VarValue("var_" +
        std::to_string(_params[0]->getUnsigned()) + "_" + std::to_string(_params[1]->getUnsigned())),
        new VarValue(_params[2]->getString()),
        op);

    stack.push(v->negate());
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
    if (_opcode == 0x10)
    {
        // Forward jump
        return _address + _params[0]->getUnsigned()+1;
    }
    // Backwards jump
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
    std::string strName;
    switch (_opcode)
    {
    case 0x0:
        strName = "return;";
        break;

    case 0x24:
        strName = "Wait(" + std::to_string(_params[0]->getUnsigned()) + ");";
        break;

    case 0xE5:
        strName = "SetPalette(" + 
            std::to_string(_params[0]->getUnsigned()) + "," +
            std::to_string(_params[1]->getUnsigned()) + "," +
            std::to_string(_params[2]->getUnsigned()) + "," +
            std::to_string(_params[3]->getUnsigned()) +  ");";
        break;

    case 0xEA:
        strName = "MulitplyPallete();";
        break;

    case 0xE7:
        strName = "CopyPallete();";
        break;

    case 0xE6:
        strName = "LoadPallete();";
        break;

    default:
        strName = "UnknownKernelFunction_" + std::to_string(_opcode);
        break;
    }
    codeGen->addOutputLine(strName);
}

void FF7::FF7NoOutputInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}
