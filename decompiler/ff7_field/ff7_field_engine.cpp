#include "ff7_field_engine.h"
#include "ff7_field_disassembler.h"
#include "ff7_field_codegen.h" 

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

void FF7::FF7Engine::getVariants(std::vector<std::string>&) const
{

}

void FF7::FF7LoadInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}

void FF7::FF7StoreInstruction::processInst(ValueStack&, Engine*, CodeGenerator *codeGen)
{
    switch (_opcode)
    {
    case eOpcodes::SETBYTE: // set byte
        codeGen->addOutputLine("var_"
            + std::to_string(_params[0]->getUnsigned())
            + "_" +
            std::to_string(_params[1]->getUnsigned())
            + "="
            + std::to_string(_params[2]->getUnsigned()) + ";");
        break;

    case eOpcodes::MINUS:
        codeGen->addOutputLine(
            "var_" +
            _params[0]->getString() + "_" +
            _params[1]->getString() + " -= " +
            _params[2]->getString() );
        break;

    case eOpcodes::PLUS:
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

void FF7::FF7StackInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}

void FF7::FF7CondJumpInstruction::processInst(ValueStack &stack, Engine*, CodeGenerator*)
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
    if (static_cast<eOpcodes>(_opcode) == eOpcodes::JMPF)
    {
        // Forward jump
        return _address + _params[0]->getUnsigned()+1;
    }
   
    // Backwards jump,  eOpcodes::JMPBL
	return _address - _params[0]->getUnsigned();
}

std::ostream& FF7::FF7UncondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    output << " (Jump target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}


void FF7::FF7UncondJumpInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}

void FF7::FF7KernelCallInstruction::processInst(ValueStack&, Engine*, CodeGenerator *codeGen)
{
    switch (_opcode)
    {
    case eOpcodes::RET:
        codeGen->writeFunctionCall("return", "", _params);
        break;

    case eOpcodes::WAIT:
        codeGen->writeFunctionCall("wait", "f", _params);
        break;

    case eOpcodes::STPAL:
        codeGen->writeFunctionCall("setPalette", "nnnn", _params);
        break;

    case eOpcodes::MPPAL2:
        codeGen->writeFunctionCall("mulitplyPallete", "", _params);
        break;

    case eOpcodes::CPPAL:
        codeGen->writeFunctionCall("copyPallete", "", _params);
        break;

    case eOpcodes::LDPAL:
        codeGen->writeFunctionCall("loadPallete", "", _params);
        break;

    case eOpcodes::REQEW:
        codeGen->writeFunctionCall("callScriptBlocking", "", _params);
        break;

    case eOpcodes::BGCLR:
        codeGen->writeFunctionCall("backgroundClear", "nn", _params);
        break;

    case eOpcodes::BGOFF:
        codeGen->writeFunctionCall("backgroundOff", "", _params);
        break;

    case eOpcodes::BGON:
        codeGen->writeFunctionCall("backgroundOn", "", _params);
        break;

    case eOpcodes::MOD:
        codeGen->writeFunctionCall("Mod", "nn", _params); // TODO: Shouldn't be a kernel call
        break;

    case eOpcodes::INC:
        codeGen->writeFunctionCall("Inc", "nn", _params); // TODO: Shouldn't be a kernel call
        break;

    case eOpcodes::DEC:
        codeGen->writeFunctionCall("Dec", "nn", _params); // TODO: Shouldn't be a kernel call
        break;

    case eOpcodes::RANDOM:
        codeGen->writeFunctionCall("Rand", "nn", _params);
        break;

    case eOpcodes::opCodeCHAR:
        codeGen->writeFunctionCall("Char", "n", _params);
        break;

    case eOpcodes::PC:
        codeGen->writeFunctionCall("setPlayableChar", "n", _params);
        break;

    case eOpcodes::XYZI:
        codeGen->writeFunctionCall("placeObject", "nnnnn", _params);
        break;

    case eOpcodes::SOLID:
        codeGen->writeFunctionCall("setSolid", "n", _params);
        break;

    case eOpcodes::TALKON:
        codeGen->writeFunctionCall("setTalkable", "n", _params);
        break;

    case eOpcodes::VISI:
        codeGen->writeFunctionCall("setVisible", "n", _params);
        break;

    case eOpcodes::MSPED:
        codeGen->writeFunctionCall("setMoveSpeed", "nn", _params);
        break;

    case eOpcodes::MOVE:
        codeGen->writeFunctionCall("move", "nnn", _params);
        break;

    case eOpcodes::SETWORD:
        codeGen->writeFunctionCall("setWord", "nnn", _params);
        break;

    case eOpcodes::ANIME1:
        codeGen->writeFunctionCall("playBlockingAnimation", "nn", _params);
        break;

    case eOpcodes::DFANM:
        codeGen->writeFunctionCall("playAnimationLoop", "nn", _params);
        break;

    case eOpcodes::DIR:
        codeGen->writeFunctionCall("turnToEntity", "nn", _params);
        break;

    case eOpcodes::STPLS:
        codeGen->writeFunctionCall("STPLS", "", _params);
        break;

    case eOpcodes::ADPAL:
        codeGen->writeFunctionCall("ADPAL", "", _params);
        break;

    case eOpcodes::LDPLS:
        codeGen->writeFunctionCall("LDPLS", "", _params);
        break;

    default:
        codeGen->addOutputLine("UnknownKernelFunction_" + std::to_string(_opcode) + "();");
        break;
    }
}

void FF7::FF7NoOutputInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}
