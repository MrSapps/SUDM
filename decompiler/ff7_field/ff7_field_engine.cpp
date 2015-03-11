#include "ff7_field_engine.h"
#include "ff7_field_disassembler.h"
#include "ff7_field_codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include "make_unique.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

std::unique_ptr<Disassembler> FF7::FF7FieldEngine::getDisassembler(InstVec &insts, const std::vector<unsigned char>& rawScriptData)
{
    return std::make_unique<FF7Disassembler>(mFormatter, this, insts, rawScriptData);
}

std::unique_ptr<Disassembler> FF7::FF7FieldEngine::getDisassembler(InstVec &insts)
{
    return std::make_unique<FF7Disassembler>(mFormatter, this, insts);
}

std::unique_ptr<CodeGenerator> FF7::FF7FieldEngine::getCodeGenerator(const InstVec& insts, std::ostream &output)
{
    return std::make_unique<FF7CodeGenerator>(this, insts, output);
}

void FF7::FF7FieldEngine::postCFG(InstVec& insts, Graph g)
{
    /*
    // In FF7 some scripts ends with an infinite loop to "keep it alive"
    // in QGears this isn't required so we can remove them
    RemoveTrailingInfiniteLoops(insts, g);

    // This could generate bad code, but it always seems to follow that pattern that if the last
    // instruction is an uncond jump back into the script then it simply nests all of those blocks
    // in an infinite loop
    MarkInfiniteLoopGroups(insts, g);

    // Scripts end with a "return" this isn't required so strip them out
    RemoveExtraneousReturnStatements(insts, g);
    */
}

std::map<std::string, int> FF7::FF7FieldEngine::GetEntities() const
{
    std::map<std::string, int> r;
    for (auto& f : _functions)
    {
        const Function& func = f.second;
        FF7::FunctionMetaData meta(func._metadata);
        auto it = r.find(meta.EntityName());
        if (it != std::end(r))
        {
            // Try to find a function in this entity has that has a char id
            // don't overwrite a valid char id with a "blank" one
            if (it->second == -1)
            {
                it->second = meta.CharacterId();
            }
        }
        else
        {
            r[meta.EntityName()] = meta.CharacterId();
        }
    }
    return r;
}

void FF7::FF7FieldEngine::AddEntityFunction(const std::string& entityName, size_t entityIndex, const std::string& funcName, size_t funcIndex)
{
    auto it = mEntityIndexMap.find(entityIndex);
    if (it != std::end(mEntityIndexMap))
    {
        (*it).second.AddFunction(funcName, funcIndex);
    }
    else
    {
        Entity e(entityName);
        e.AddFunction(funcName, funcIndex);
        mEntityIndexMap.insert(std::make_pair(entityIndex, e));
    }
}

void FF7::FF7FieldEngine::RemoveExtraneousReturnStatements(InstVec& insts, Graph g)
{
    for (auto& f : _functions)
    {
        Function& func = f.second;
        for (auto it = insts.begin(); it != insts.end(); it++)
        {
            // Is it the last instruction in the function, and is it a return statement?
            if ((*it)->_address == func.mEndAddr)
            {
                if ((*it)->_opcode == eOpcodes::RET)
                {
                    // Set new end address to be before the NOP
                    func.mEndAddr = (*(it - 1))->_address;
                    func.mNumInstructions--;

                    Instruction* nop = new FF7NoOutputInstruction();
                    nop->_opcode = eOpcodes::NOP;
                    nop->_address = (*it)->_address;
                    (*it).reset(nop);
                    break;
                }
            }
        }
    }
}

void FF7::FF7FieldEngine::RemoveTrailingInfiniteLoops(InstVec& insts, Graph g)
{
    for (auto& f : _functions)
    {
        Function& func = f.second;
        for (auto it = insts.begin(); it != insts.end(); it++)
        {
            // Is it the last instruction in the function, a jump, and a jumping to itself?
            if ((*it)->_address == func.mEndAddr)
            {
                if ((*it)->isJump() && (*it)->getDestAddress() == (*it)->_address)
                {
                    // Set new end address to be before the NOP
                    func.mEndAddr = (*(it - 1))->_address;
                    func.mNumInstructions--;

                    Instruction* nop = new FF7NoOutputInstruction();
                    nop->_opcode = eOpcodes::NOP;
                    nop->_address = (*it)->_address;
                    (*it).reset(nop);
                    break;
                }
            }
        }
    }
}

void FF7::FF7FieldEngine::MarkInfiniteLoopGroups(InstVec& insts, Graph g)
{
    for (auto& f : _functions)
    {
        Function& func = f.second;
        for (auto it = insts.begin(); it != insts.end(); it++)
        {
            if ((*it)->_address == func.mEndAddr)
            {
                // Note: This is a "best effort heuristic", so quite a few loops will still end up as goto's.
                // this could potentially generate invalid code too
                if ((*it)->isUncondJump() )
                {
                    // Then assume its an infinite do { } while(true) loop that wraps part of the script
                    VertexRange vr = boost::vertices(g);
                    for (VertexIterator v = vr.first; v != vr.second; ++v)
                    {
                        GroupPtr gr = GET(*v);
                        if ((*gr->_start)->_address == func.mEndAddr)
                        {
                            // Then assume its an infinite do { } while(true) loop that wraps part of the script
                            gr->_type = kDoWhileCondGroupType;
                        }
                    } 
                }
                break;
            }
        }
    }
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
        return "var_" + std::to_string(bank) + "_" + std::to_string(addr);
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
        throw UnknownBankException();
    }

}

void FF7::FF7StoreInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator *codeGen)
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

            codeGen->addOutputLine(d + " = " + s + codeGen->TargetLang().LineTerminator());
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

            codeGen->addOutputLine(s + " = " + s + " % " + d + codeGen->TargetLang().LineTerminator());
        }
        break;

    case eOpcodes::INC:
        {
            const uint32 srcBank = _params[0]->getUnsigned();
            const uint32 srcAddrOrValue = _params[1]->getUnsigned();
            auto s = GetVarName(srcBank, srcAddrOrValue);
            codeGen->addOutputLine(s + " = " + s + " + 1" + codeGen->TargetLang().LineTerminator());
        }
        break;

    case eOpcodes::DEC:
        {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[1]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);
        codeGen->addOutputLine(s + " = " + s + " - 1" + codeGen->TargetLang().LineTerminator());
        }
        break;

    case eOpcodes::RANDOM: // 8-bit random value 
        {
            const uint32 dstBank = _params[0]->getUnsigned();
            const uint32 dstAddr = _params[1]->getUnsigned();
            auto d = GetVarName(dstBank, dstAddr);
            codeGen->addOutputLine(d + " = math.random(0, 255)" + codeGen->TargetLang().LineTerminator());
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

        if (_opcode == eOpcodes::MINUS)
        {
            codeGen->addOutputLine(s + " = " + s + " - " + d + codeGen->TargetLang().LineTerminator());
        }
        else
        {
            codeGen->addOutputLine(s + " = " + s + " + " + d + codeGen->TargetLang().LineTerminator());
        }
    }
        break;

    case eOpcodes::BITON:
        codeGen->addOutputLine("-- TODO BITON");
        break;

    default:
        codeGen->addOutputLine("-- TODO unknown store " + std::to_string(_opcode));
    }
}

void FF7::FF7CondJumpInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator*)
{
    std::string funcName;
    if (_opcode == eOpcodes::IFKEYON)
    {
        funcName = "KeyOn";
    }
    else if (_opcode == eOpcodes::IFKEYOFF)
    {
        funcName = "KeyOff";
    }
    else if (_opcode == eOpcodes::IFKEY)
    {
        funcName = "Key";
    }
    else if (_opcode == eOpcodes::IFMEMBQ)
    {
        funcName = "IFMEMBQ";
    }
    else if (_opcode == eOpcodes::IFPRTYQ)
    {
        funcName = "IFPRTYQ";
    }

    if (!funcName.empty())
    {
        ValuePtr v = new StringValue("if (" + funcName + " (" + std::to_string(_params[0]->getUnsigned()) + "))) then");
        stack.push(v);
        return;
    }

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
    uint32 jumpParamIndex = 5;
    switch (_opcode)
    {
    case eOpcodes::IFUB:
        paramsSize = 5; 
        break;

    case eOpcodes::IFUBL:
        // dessert: i think the jumps are relative to the start of the jump offset. otherwise they jump into to the middle of the instruction
        //paramsSize = 6;
        paramsSize = 5;
        break;

    case eOpcodes::IFSW:
        paramsSize = 7;
        break;

    case eOpcodes::IFSWL:
        paramsSize = 7;
        break;

    case eOpcodes::IFUW:
        paramsSize = 7;
        break;

    case eOpcodes::IFUWL:
        // dessert: as above
        //paramsSize = 8;
        paramsSize = 7;
        break;

    case eOpcodes::IFKEYON:
    case eOpcodes::IFKEYOFF:
    case eOpcodes::IFKEY:
        paramsSize = 3;
        jumpParamIndex = 1;
        break;

    case eOpcodes::IFPRTYQ:
    case eOpcodes::IFMEMBQ:
        paramsSize = 2;
        jumpParamIndex = 1;
        break;

    default:
        throw UnknownJumpTypeException(_address, _opcode);
    }

    return _address + _params[jumpParamIndex]->getUnsigned() + paramsSize;
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
    if (static_cast<eOpcodes>(_opcode) == eOpcodes::JMPF || static_cast<eOpcodes>(_opcode) == eOpcodes::JMPFL)
    {
        // Short or long forward jump 
        return _address + _params[0]->getUnsigned() + 1;
    }

    // Backwards jump,  eOpcodes::JMPB/L
	return _address - _params[0]->getUnsigned();
}

std::ostream& FF7::FF7UncondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    output << " (Jump target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}


void FF7::FF7UncondJumpInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}

static void WriteTodo(CodeGenerator *codeGen, std::string entityName, std::string opCode)
{
    codeGen->addOutputLine("--Todo(\"" + opCode + "\")");
}

void FF7::FF7KernelCallInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::RET:
        codeGen->addOutputLine("return 0"); // Seems all our functions must return zero
        break;

    case eOpcodes::WAIT:
        WriteTodo(codeGen, md.EntityName(), "script:wait");
        break;

    case eOpcodes::STPAL:
        WriteTodo(codeGen, md.EntityName(), "setPalette");
        break;

    case eOpcodes::MPPAL2:
        WriteTodo(codeGen, md.EntityName(), "mulitplyPallete");
        break;

    case eOpcodes::CPPAL:
        WriteTodo(codeGen, md.EntityName(), "copyPallete");
        break;

    case eOpcodes::LDPAL:
        WriteTodo(codeGen, md.EntityName(), "loadPallete");
        break;

    case eOpcodes::REQEW:
    {
        const auto priority = _params[1]->getUnsigned();
        const auto& entity = eng.EntityByIndex(_params[0]->getSigned());
        const auto& scriptName = entity.FunctionByIndex(_params[2]->getUnsigned());

        WriteTodo(codeGen, md.EntityName(), "callScriptBlocking " + entity.Name() + " " + scriptName + " " + std::to_string(priority));
    }
        break;

    case eOpcodes::BGCLR:
        WriteTodo(codeGen, md.EntityName(), "backgroundClear");
        break;

    case eOpcodes::BGOFF:
    {
        const uint32 srcBank = _params[0]->getUnsigned();
        const uint32 srcAddrOrValue = _params[2]->getUnsigned();
        auto s = GetVarName(srcBank, srcAddrOrValue);

        const uint32 dstBank = _params[1]->getUnsigned();
        const uint32 dstAddrOrValue = _params[3]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddrOrValue);

        std::string line = "backgroundOff(" + d + ", " + s + ")";
        WriteTodo(codeGen, md.EntityName(), line);
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
        
        std::string line = "backgroundOn(" + d + ", " + s + ")";
        WriteTodo(codeGen, md.EntityName(), line);
    }
        break;

    case eOpcodes::opCodeCHAR:
        codeGen->addOutputLine("self." + md.EntityName() + " = entity_manager:get_entity(\"" + md.EntityName() + "\")");
        break;

    case eOpcodes::PC:
        codeGen->addOutputLine("set_entity_to_character(\"" + md.EntityName() + "\", \"" + md.EntityName() + "\")");
        break;

    case eOpcodes::XYZI:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_position", "nnnnn", _params);
        break;

    case eOpcodes::SOLID:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_solid", "b", _params);
        break;

    case eOpcodes::TLKON:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_talkable", "b", _params);
        break;

    case eOpcodes::VISI:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_visible", "b", _params);
        break;

    case eOpcodes::MSPED:
        // TODO: QGears had av_b of mds1_2 set to 3.75, but how?
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_move_auto_speed", "nn", _params);
        break;

    case eOpcodes::MOVE:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":move_to_position", "nnn", _params);
        break;

    case eOpcodes::ANIME1:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":play_animation", "nn", _params);
        codeGen->writeFunctionCall("self." + md.EntityName() + ":animation_sync", "", _params);
        break;

    case eOpcodes::DFANM:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_default_animation", "nn", _params);
        codeGen->writeFunctionCall("self." + md.EntityName() + ":play_animation", "nn", _params);
        break;

    case eOpcodes::DIR:
        codeGen->writeFunctionCall("self." + md.EntityName() + ":set_rotation", "nn", _params);
        break;

    case eOpcodes::STPLS:
        WriteTodo(codeGen, md.EntityName(), "STPLS");
        break;

    case eOpcodes::ADPAL:
        WriteTodo(codeGen, md.EntityName(), "ADPAL");
        break;

    case eOpcodes::LDPLS:
        WriteTodo(codeGen, md.EntityName(), "LDPLS");
        break;

    case eOpcodes::BTLMD:
        WriteTodo(codeGen, md.EntityName(), "BTLMD");
        break;

    case eOpcodes::MUSIC:
        WriteTodo(codeGen, md.EntityName(), "MUSIC");
        break;

    case eOpcodes::MPNAM:
        WriteTodo(codeGen, md.EntityName(), "MPNAM");
        break;

    case eOpcodes::GETAI:
    {

        const uint32 dstBank = _params[0]->getUnsigned();
        const uint32 dstAddrOrValue = _params[2]->getUnsigned();
        auto d = GetVarName(dstBank, dstAddrOrValue);

        std::string line = "getEntityTriangleId(" + d + ", " + std::to_string(_params[1]->getUnsigned()) + ")";
        WriteTodo(codeGen, md.EntityName(), line);
    }
        break;

    default:
        WriteTodo(codeGen, md.EntityName(), "UnknownKernelFunction_" + std::to_string(_opcode));
        break;
    }
}

void FF7::FF7NoOutputInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}
