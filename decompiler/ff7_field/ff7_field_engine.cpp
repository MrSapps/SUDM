#include "ff7_field_engine.h"
#include "ff7_field_disassembler.h"
#include "ff7_field_codegen.h" 

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

void FF7::FF7Engine::getVariants(std::vector<std::string>&) const
{

}

void FF7::FF7LoadInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}

static std::string GetVarName(uint32 bank, uint32 addr)
{
    if (bank == 0)
    {
        // Just a number
        return std::to_string(addr);
    }
    else if (bank == 1 || bank == 2 || bank == 3 || bank == 13 || bank == 15)
    {
        // TODO: Get the textual name of the var such as tifaLovePoints
        return std::to_string(bank) + "_" + std::to_string(addr);
    }
    else if (bank == 5)
    {
        return "temp5_" + std::to_string(addr);
    }
    else if (bank == 6)
    {
        return "temp6_" + std::to_string(addr);
    }
    else
    {
        throw InternalDecompilerError();
    }

}

void FF7::FF7StoreInstruction::processInst(ValueStack&, Engine*, CodeGenerator *codeGen)
{
    switch (_opcode)
    {
    case eOpcodes::SETWORD:
    case eOpcodes::SETBYTE:
        {
            const uint32 srcBank = _params[1]->getUnsigned();
            const uint32 srcAddrOrValue = _params[3]->getUnsigned();
            auto s = GetVarName(srcBank, srcAddrOrValue);

            const uint32 dstBank = _params[0]->getUnsigned();
            const uint32 dstAddr = _params[2]->getUnsigned();
            auto d = GetVarName(dstBank, dstAddr);

            codeGen->addOutputLine(d + " = " + s + ";");
        }
        break;

    case eOpcodes::MOD:
        {
            const uint32 srcBank = _params[0]->getUnsigned();
            const uint32 srcAddrOrValue = _params[2]->getUnsigned(); 
            auto s = GetVarName(srcBank, srcAddrOrValue);

            const uint32 dstBank = _params[1]->getUnsigned(); 
            const uint32 dstAddr = _params[3]->getUnsigned();
            auto d = GetVarName(dstBank, dstAddr);

            codeGen->addOutputLine(s + " = " + s + " % " + d + ";");
        }
        break;

    case eOpcodes::INC:
        {
            const uint32 srcBank = _params[0]->getUnsigned();
            const uint32 srcAddrOrValue = _params[1]->getUnsigned();
            auto s = GetVarName(srcBank, srcAddrOrValue);
            codeGen->addOutputLine(s + "++;");
        }
        break;

    case eOpcodes::DEC:
        {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[1]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);
        codeGen->addOutputLine(s + "--;");
        }
        break;

    case eOpcodes::RANDOM:
        {
            const uint32 dstBank = _params[0]->getUnsigned();
            const uint32 dstAddr = _params[1]->getUnsigned();
            auto d = GetVarName(dstBank, dstAddr);
            codeGen->addOutputLine(d + " = rand(); ");
        }
        break;

    case eOpcodes::MINUS:
    case eOpcodes::PLUS:
    {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[2]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);

        const uint32 dstBank = _params[1]->getUnsigned();
        const uint32 dstAddr = _params[3]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddr);


        codeGen->addOutputLine(
            s +
            (_opcode == eOpcodes::MINUS ? " -= " : " += ") +
            d);
    }
        break;

    default:
        codeGen->addOutputLine("unknown store");
        break;
    }
}

void FF7::FF7CondJumpInstruction::processInst(ValueStack &stack, Engine*, CodeGenerator*)
{
    std::string op;
    uint32 type = _params[4]->getUnsigned();
    switch (type)
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
        throw UnknownConditionalOperatorException(_address, type);
    }

    const uint32 srcBank = _params[0]->getUnsigned();
    const uint32 srcAddrOrValue = _params[2]->getUnsigned();
    auto s = GetVarName(srcBank, srcAddrOrValue);

    const uint32 dstBank = _params[1]->getUnsigned();
    const uint32 dstAddrOrValue = _params[3]->getUnsigned();
    auto d = GetVarName(dstBank, dstAddrOrValue);


    ValuePtr v = new BinaryOpValue(
        new VarValue(s),
        new VarValue(d), op);

    stack.push(v->negate());
}

uint32 FF7::FF7CondJumpInstruction::getDestAddress() const
{
    uint32 paramsSize = 0;
    switch (_opcode)
    {
    case eOpcodes::IFUB:
        paramsSize = 5; 
        break;

    case eOpcodes::IFUBL:
        paramsSize = 6;
        break;

    case eOpcodes::IFSW:
        paramsSize = 7;
        break;

    case eOpcodes::IFSWL:
        paramsSize = 8;
        break;

    case eOpcodes::IFUW:
        paramsSize = 7;
        break;

    case eOpcodes::IFUWL:
        paramsSize = 8;
        break;

    default:
        throw UnknownOpcodeException(_address, _opcode);
    }

    return _address + _params[5]->getUnsigned() + paramsSize;
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
        codeGen->addOutputLine("return;");
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
    {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[2]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);

        const uint32 dstBank = _params[1]->getUnsigned();
        const uint32 dstAddrOrValue = _params[3]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddrOrValue);

        std::string line = "backgroundOff(" + d + ", " + s + ");";
        codeGen->addOutputLine(line);
    }
        break;

    case eOpcodes::BGON:
    {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[2]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);

        const uint32 dstBank = _params[1]->getUnsigned();
        const uint32 dstAddrOrValue = _params[3]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddrOrValue);
        
        std::string line = "backgroundOn(" + d + ", " + s + ");";
        codeGen->addOutputLine(line);
    }
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

    case eOpcodes::BTLMD:
        codeGen->writeFunctionCall("BTLMD", "n", _params);
        break;

    case eOpcodes::MUSIC:
        codeGen->writeFunctionCall("MUSIC", "n", _params);
        break;

    case eOpcodes::MPNAM:
        codeGen->writeFunctionCall("setMapName", "n", _params);
        break;

    case eOpcodes::GETAI:
    {

        const uint32 dstBank = _params[0]->getUnsigned();
        const uint32 dstAddrOrValue = _params[2]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddrOrValue);

        std::string line = "getEntityTriangleId(" + d + ", " + std::to_string(_params[1]->getUnsigned()) + ");";
        codeGen->addOutputLine(line);
    }
        break;

    default:
        codeGen->addOutputLine("UnknownKernelFunction_" + std::to_string(_opcode) + "();");
        break;
    }
}

void FF7::FF7NoOutputInstruction::processInst(ValueStack&, Engine*, CodeGenerator*)
{

}
