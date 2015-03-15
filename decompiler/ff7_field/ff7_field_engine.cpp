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

                    Instruction* nop = new FF7NoOperationInstruction();
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

                    Instruction* nop = new FF7NoOperationInstruction();
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
                if ((*it)->isUncondJump())
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

    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());

    ValuePtr v = new BinaryOpValue(new VarValue(source), new VarValue(destination), op);

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

void FF7::FF7ControlFlowInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::RET:
        codeGen->addOutputLine("return 0"); // Seems all our functions must return zero
        break;

    case eOpcodes::REQ:
        processREQ(codeGen, eng);
        break;

    case eOpcodes::REQSW:
        processREQSW(codeGen, eng);
        break;

    case eOpcodes::REQEW:
        processREQEW(codeGen, eng);
        break;

    case eOpcodes::PREQ:
        WriteTodo(codeGen, md.EntityName(), "PREQ");
        break;

    case eOpcodes::PRQSW:
        WriteTodo(codeGen, md.EntityName(), "PRQSW");
        break;

    case eOpcodes::PRQEW:
        WriteTodo(codeGen, md.EntityName(), "PRQEW");
        break;

    case eOpcodes::RETTO:
        processRETTO(codeGen);
        break;
        
    case eOpcodes::WAIT:
        processWAIT(codeGen);
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7ControlFlowInstruction::processREQ(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
    const auto& scriptName = entity.FunctionByIndex(_params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("script:request( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % _params[1]->getUnsigned()).str());
}

void FF7::FF7ControlFlowInstruction::processREQSW(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
    const auto& scriptName = entity.FunctionByIndex(_params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("script:request_start_sync( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % _params[1]->getUnsigned()).str());
}

void FF7::FF7ControlFlowInstruction::processREQEW(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
    const auto& scriptName = entity.FunctionByIndex(_params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("script:request_end_sync( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % _params[1]->getUnsigned()).str());
}

void FF7::FF7ControlFlowInstruction::processRETTO(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- return_to( script_id_in_current_entity=%2%, priority=%1% )") % _params[0]->getUnsigned() % _params[1]->getUnsigned()).str());
}

void FF7::FF7ControlFlowInstruction::processWAIT(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("script:wait( %1% )") % (_params[0]->getUnsigned() / 30.0f)).str());
}

void FF7::FF7ModuleInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::DSKCG:
        WriteTodo(codeGen, md.EntityName(), "DSKCG");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::ARROW:
        WriteTodo(codeGen, md.EntityName(), "ARROW");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::PNAME:
        WriteTodo(codeGen, md.EntityName(), "PNAME");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::GMSPD:
        WriteTodo(codeGen, md.EntityName(), "GMSPD");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::FLMAT:
        WriteTodo(codeGen, md.EntityName(), "FLMAT");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::FLITM:
        WriteTodo(codeGen, md.EntityName(), "FLITM");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::BTLCK:
        WriteTodo(codeGen, md.EntityName(), "BTLCK");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::MVLCK:
        WriteTodo(codeGen, md.EntityName(), "MVLCK");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::SPCNM:
        WriteTodo(codeGen, md.EntityName(), "SPCNM");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::RSGLB:
        WriteTodo(codeGen, md.EntityName(), "RSGLB");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::CLITM:
        WriteTodo(codeGen, md.EntityName(), "CLITM");
        break;

    case eOpcodes::MINIGAME:
        WriteTodo(codeGen, md.EntityName(), "MINIGAME");
        break;

    case eOpcodes::BTMD2:
        WriteTodo(codeGen, md.EntityName(), "BTMD2");
        break;

    case eOpcodes::BTRLD:
        WriteTodo(codeGen, md.EntityName(), "BTRLD");
        break;

    case eOpcodes::BTLTB:
        WriteTodo(codeGen, md.EntityName(), "BTLTB");
        break;

    case eOpcodes::MAPJUMP:
        WriteTodo(codeGen, md.EntityName(), "MAPJUMP");
        break;

    case eOpcodes::LSTMP:
        WriteTodo(codeGen, md.EntityName(), "LSTMP");
        break;

    case eOpcodes::BATTLE:
        WriteTodo(codeGen, md.EntityName(), "BATTLE");
        break;

    case eOpcodes::BTLON:
        WriteTodo(codeGen, md.EntityName(), "BTLON");
        break;

    case eOpcodes::BTLMD:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, *this));
        break;

    case eOpcodes::MPJPO:
        WriteTodo(codeGen, md.EntityName(), "MPJPO");
        break;

    case eOpcodes::PMJMP:
        WriteTodo(codeGen, md.EntityName(), "PMJMP");
        break;

    case eOpcodes::PMJMP2:
        WriteTodo(codeGen, md.EntityName(), "PMJMP2");
        break;

    case eOpcodes::GAMEOVER:
        WriteTodo(codeGen, md.EntityName(), "GAMEOVER");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7MathInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::PLUS_:
        processSaturatedPLUS(codeGen);
        break;

    case eOpcodes::PLUS2_:
        processSaturatedPLUS2(codeGen);
        break;

    case eOpcodes::MINUS_:
        processSaturatedMINUS(codeGen);
        break;

    case eOpcodes::MINUS2_:
        processSaturatedMINUS2(codeGen);
        break;

    case eOpcodes::INC_:
        processSaturatedINC(codeGen);
        break;

    case eOpcodes::INC2_:
        processSaturatedINC2(codeGen);
        break;

    case eOpcodes::DEC_:
        processSaturatedDEC(codeGen);
        break;

    case eOpcodes::DEC2_:
        processSaturatedDEC2(codeGen);
        break;

    case eOpcodes::RDMSD:
        processRDMSD(codeGen);
        break;

    case eOpcodes::SETBYTE:
    case eOpcodes::SETWORD:
        processSETBYTE_SETWORD(codeGen);
        break;

    case eOpcodes::BITON:
        WriteTodo(codeGen, md.EntityName(), "BITON");
        break;

    case eOpcodes::BITOFF:
        WriteTodo(codeGen, md.EntityName(), "BITOFF");
        break;

    case eOpcodes::BITXOR:
        WriteTodo(codeGen, md.EntityName(), "BITXOR");
        break;

    case eOpcodes::PLUS:
    case eOpcodes::PLUS2:
        processPLUSx_MINUSx(codeGen, "+");
        break;

    case eOpcodes::MINUS:
    case eOpcodes::MINUS2:
        processPLUSx_MINUSx(codeGen, "-");
        break;

    case eOpcodes::MUL:
        WriteTodo(codeGen, md.EntityName(), "MUL");
        break;

    case eOpcodes::MUL2:
        WriteTodo(codeGen, md.EntityName(), "MUL2");
        break;

    case eOpcodes::DIV:
        WriteTodo(codeGen, md.EntityName(), "DIV");
        break;

    case eOpcodes::DIV2:
        WriteTodo(codeGen, md.EntityName(), "DIV2");
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

    case eOpcodes::MOD2:
        WriteTodo(codeGen, md.EntityName(), "MOD2");
        break;

    case eOpcodes::AND:
        WriteTodo(codeGen, md.EntityName(), "AND");
        break;

    case eOpcodes::AND2:
        WriteTodo(codeGen, md.EntityName(), "AND2");
        break;

    case eOpcodes::OR:
        WriteTodo(codeGen, md.EntityName(), "OR");
        break;

    case eOpcodes::OR2:
        WriteTodo(codeGen, md.EntityName(), "OR2");
        break;

    case eOpcodes::XOR:
        WriteTodo(codeGen, md.EntityName(), "XOR");
        break;

    case eOpcodes::XOR2:
        WriteTodo(codeGen, md.EntityName(), "XOR2");
        break;

    case eOpcodes::INC:
    case eOpcodes::INC2:
        processINCx_DECx(codeGen, "+");
        break;

    case eOpcodes::DEC:
    case eOpcodes::DEC2:
        processINCx_DECx(codeGen, "-");
        break;  

    case eOpcodes::RANDOM:
        processRANDOM(codeGen);
        break;

    case eOpcodes::LBYTE:
        WriteTodo(codeGen, md.EntityName(), "LBYTE");
        break;

    case eOpcodes::HBYTE:
        WriteTodo(codeGen, md.EntityName(), "HBYTE");
        break;

    case eOpcodes::TWOBYTE:
        WriteTodo(codeGen, md.EntityName(), "2BYTE");
        break;

    case eOpcodes::SIN:
        WriteTodo(codeGen, md.EntityName(), "SIN");
        break;

    case eOpcodes::COS:
        WriteTodo(codeGen, md.EntityName(), "COS");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7MathInstruction::processSaturatedPLUS(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("if (%1% > 255); %1% = 255; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedPLUS2(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("if (%1% > 32767); %1% = 32767; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedMINUS(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("if (%1% < 0); %1% = 0; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedMINUS2(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("if (%1% < 0); %1% = 0; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedINC(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + 1") % destination).str());
    codeGen->addOutputLine((boost::format("if (%1% > 255); %1% = 255; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedINC2(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + 1") % destination).str());
    codeGen->addOutputLine((boost::format("if (%1% > 32767); %1% = 32767; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedDEC(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - 1") % destination).str());
    codeGen->addOutputLine((boost::format("if (%1% < 0); %1% = 0; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedDEC2(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - 1")).str());
    codeGen->addOutputLine((boost::format("if (%1% < 0); %1% = 0; end") % destination).str());
}

void FF7::FF7MathInstruction::processRDMSD(CodeGenerator* codeGen)
{
    // TODO: verify we actually have os.time
    // TODO: RNG emulation?
    codeGen->addOutputLine("math.randomseed( os.time() )");
}

void FF7::FF7MathInstruction::processSETBYTE_SETWORD(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: respect destination bank sizes (16-bit writes only affect low byte)
    codeGen->addOutputLine((boost::format("%1% = %2%") % destination % source).str());
}

void FF7::FF7MathInstruction::processPLUSx_MINUSx(CodeGenerator* codeGen, const std::string& op)
{
    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% %2% %3%") % lhs % op % rhs).str());
}

void FF7::FF7MathInstruction::processINCx_DECx(CodeGenerator* codeGen, const std::string& op)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% %2% 1") % destination % op).str());
}

void FF7::FF7MathInstruction::processRANDOM(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes (16-bit writes only affect low byte)
    // TODO: RNG emulation?
    codeGen->addOutputLine((boost::format("%1% = math.random( 0, 255 )") % destination).str());
}

void FF7::FF7WindowInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::TUTOR:
        WriteTodo(codeGen, md.EntityName(), "TUTOR");
        break;

    case eOpcodes::WCLS:
        WriteTodo(codeGen, md.EntityName(), "WCLS");
        break;

    case eOpcodes::WSIZW:
        WriteTodo(codeGen, md.EntityName(), "WSIZW");
        break;

    case eOpcodes::WSPCL:
        WriteTodo(codeGen, md.EntityName(), "WSPCL");
        break;

    case eOpcodes::WNUMB:
        WriteTodo(codeGen, md.EntityName(), "WNUMB");
        break;

    case eOpcodes::STTIM:
        WriteTodo(codeGen, md.EntityName(), "STTIM");
        break;

    case eOpcodes::MESSAGE:
        WriteTodo(codeGen, md.EntityName(), "MESSAGE");
        break;

    case eOpcodes::MPARA:
        WriteTodo(codeGen, md.EntityName(), "MPARA");
        break;

    case eOpcodes::MPRA2:
        WriteTodo(codeGen, md.EntityName(), "MPRA2");
        break;

    case eOpcodes::MPNAM:
        processMPNAM(codeGen);
        break;

    case eOpcodes::ASK:
        WriteTodo(codeGen, md.EntityName(), "ASK");
        break;

    case eOpcodes::MENU:
        WriteTodo(codeGen, md.EntityName(), "MENU");
        break;

    case eOpcodes::MENU2:
        WriteTodo(codeGen, md.EntityName(), "MENU2");
        break;

    case eOpcodes::WINDOW:
        WriteTodo(codeGen, md.EntityName(), "WINDOW");
        break;

    case eOpcodes::WMOVE:
        WriteTodo(codeGen, md.EntityName(), "WMOVE");
        break;

    case eOpcodes::WMODE:
        WriteTodo(codeGen, md.EntityName(), "WMODE");
        break;

    case eOpcodes::WREST:
        WriteTodo(codeGen, md.EntityName(), "WREST");
        break;

    case eOpcodes::WCLSE:
        WriteTodo(codeGen, md.EntityName(), "WCLSE");
        break;

    case eOpcodes::WROW:
        WriteTodo(codeGen, md.EntityName(), "WROW");
        break;

    case eOpcodes::GWCOL:
        WriteTodo(codeGen, md.EntityName(), "GWCOL");
        break;

    case eOpcodes::SWCOL:
        WriteTodo(codeGen, md.EntityName(), "SWCOL");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7WindowInstruction::processMPNAM(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- field:map_name( %1% )") % _params[0]->getUnsigned()).str());
}

void FF7::FF7PartyInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::SPTYE:
        WriteTodo(codeGen, md.EntityName(), "SPTYE");
        break;

    case eOpcodes::GTPYE:
        WriteTodo(codeGen, md.EntityName(), "GTPYE");
        break;

    case eOpcodes::GOLDU:
        WriteTodo(codeGen, md.EntityName(), "GOLDU");
        break;

    case eOpcodes::GOLDD:
        WriteTodo(codeGen, md.EntityName(), "GOLDD");
        break;

    case eOpcodes::HMPMAX1:
        WriteTodo(codeGen, md.EntityName(), "HMPMAX1");
        break;

    case eOpcodes::HMPMAX2:
        WriteTodo(codeGen, md.EntityName(), "HMPMAX2");
        break;

    case eOpcodes::MHMMX:
        WriteTodo(codeGen, md.EntityName(), "MHMMX");
        break;

    case eOpcodes::HMPMAX3:
        WriteTodo(codeGen, md.EntityName(), "HMPMAX3");
        break;

    case eOpcodes::MPU:
        WriteTodo(codeGen, md.EntityName(), "MPU");
        break;

    case eOpcodes::MPD:
        WriteTodo(codeGen, md.EntityName(), "MPD");
        break;

    case eOpcodes::HPU:
        WriteTodo(codeGen, md.EntityName(), "HPU");
        break;

    case eOpcodes::HPD:
        WriteTodo(codeGen, md.EntityName(), "HPD");
        break;

    case eOpcodes::STITM:
        WriteTodo(codeGen, md.EntityName(), "STITM");
        break;

    case eOpcodes::DLITM:
        WriteTodo(codeGen, md.EntityName(), "DLITM");
        break;

    case eOpcodes::CKITM:
        WriteTodo(codeGen, md.EntityName(), "CKITM");
        break;

    case eOpcodes::SMTRA:
        WriteTodo(codeGen, md.EntityName(), "SMTRA");
        break;

    case eOpcodes::DMTRA:
        WriteTodo(codeGen, md.EntityName(), "DMTRA");
        break;

    case eOpcodes::CMTRA:
        WriteTodo(codeGen, md.EntityName(), "CMTRA");
        break;

    case eOpcodes::GETPC:
        WriteTodo(codeGen, md.EntityName(), "GETPC");
        break;

    case eOpcodes::PRTYP:
        WriteTodo(codeGen, md.EntityName(), "PRTYP");
        break;

    case eOpcodes::PRTYM:
        WriteTodo(codeGen, md.EntityName(), "PRTYM");
        break;

    case eOpcodes::PRTYE:
        WriteTodo(codeGen, md.EntityName(), "PRTYE");
        break;

    case eOpcodes::MMBUD:
        WriteTodo(codeGen, md.EntityName(), "MMBUD");
        break;

    case eOpcodes::MMBLK:
        WriteTodo(codeGen, md.EntityName(), "MMBLK");
        break;

    case eOpcodes::MMBUK:
        WriteTodo(codeGen, md.EntityName(), "MMBUK");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7ModelInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::JOIN:
        WriteTodo(codeGen, md.EntityName(), "JOIN");
        break;

    case eOpcodes::SPLIT:
        WriteTodo(codeGen, md.EntityName(), "SPLIT");
        break;

    case eOpcodes::BLINK:
        WriteTodo(codeGen, md.EntityName(), "BLINK");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::EYETX:
        WriteTodo(codeGen, md.EntityName(), "EYETX");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::TRNSP:
        WriteTodo(codeGen, md.EntityName(), "TRNSP");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::AMBNT:
        WriteTodo(codeGen, md.EntityName(), "AMBNT");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown03:
        WriteTodo(codeGen, md.EntityName(), "Unknown03");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown04:
        WriteTodo(codeGen, md.EntityName(), "Unknown04");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown05:
        WriteTodo(codeGen, md.EntityName(), "Unknown05");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::LIGHT:
        WriteTodo(codeGen, md.EntityName(), "LIGHT");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown07:
        WriteTodo(codeGen, md.EntityName(), "Unknown07");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown08:
        WriteTodo(codeGen, md.EntityName(), "Unknown08");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown09:
        WriteTodo(codeGen, md.EntityName(), "Unknown09");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::SBOBJ:
        WriteTodo(codeGen, md.EntityName(), "SBOBJ");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown0B:
        WriteTodo(codeGen, md.EntityName(), "Unknown0B");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::Unknown0C:
        WriteTodo(codeGen, md.EntityName(), "Unknown0C");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::SHINE:
        WriteTodo(codeGen, md.EntityName(), "SHINE");
        break;

    case (eOpcodes::KAWAI << 8) | eKawaiOpcodes::RESET:
        WriteTodo(codeGen, md.EntityName(), "RESET");
        break;

    case eOpcodes::KAWIW:
        WriteTodo(codeGen, md.EntityName(), "KAWIW");
        break;

    case eOpcodes::PMOVA:
        WriteTodo(codeGen, md.EntityName(), "PMOVA");
        break;

    case eOpcodes::PDIRA:
        WriteTodo(codeGen, md.EntityName(), "PDIRA");
        break;

    case eOpcodes::PTURA:
        WriteTodo(codeGen, md.EntityName(), "PTURA");
        break;

    case eOpcodes::PGTDR:
        WriteTodo(codeGen, md.EntityName(), "PGTDR");
        break;

    case eOpcodes::PXYZI:
        WriteTodo(codeGen, md.EntityName(), "PXYZI");
        break;

    case eOpcodes::TLKON:
        processTLKON(codeGen, md.EntityName());
        break;

    case eOpcodes::PC:
        processPC(codeGen, md.EntityName());
        break;

    case eOpcodes::opCodeCHAR:
        processCHAR(codeGen, md.EntityName());
        break;

    case eOpcodes::DFANM:
        processDFANM(codeGen, md.EntityName());
        break;

    case eOpcodes::ANIME1:
        processANIME1(codeGen, md.EntityName());
        break;

    case eOpcodes::VISI:
        processVISI(codeGen, md.EntityName());
        break;

    case eOpcodes::XYZI:
        processXYZI(codeGen, md.EntityName());
        break;

    case eOpcodes::XYI:
        WriteTodo(codeGen, md.EntityName(), "XYI");
        break;

    case eOpcodes::XYZ:
        WriteTodo(codeGen, md.EntityName(), "XYZ");
        break;

    case eOpcodes::MOVE:
        processMOVE(codeGen, md.EntityName());
        break;

    case eOpcodes::CMOVE:
        WriteTodo(codeGen, md.EntityName(), "XYZ");
        break;

    case eOpcodes::MOVA:
        WriteTodo(codeGen, md.EntityName(), "MOVA");
        break;

    case eOpcodes::TURA:
        WriteTodo(codeGen, md.EntityName(), "TURA");
        break;

    case eOpcodes::ANIMW:
        WriteTodo(codeGen, md.EntityName(), "ANIMW");
        break;

    case eOpcodes::FMOVE:
        WriteTodo(codeGen, md.EntityName(), "FMOVE");
        break;

    case eOpcodes::ANIME2:
        WriteTodo(codeGen, md.EntityName(), "ANIME2");
        break;

    case eOpcodes::ANIM_1:
        WriteTodo(codeGen, md.EntityName(), "ANIM_1");
        break;

    case eOpcodes::CANIM1:
        WriteTodo(codeGen, md.EntityName(), "CANIM1");
        break;

    case eOpcodes::CANM_1:
        WriteTodo(codeGen, md.EntityName(), "CANM_1");
        break;

    case eOpcodes::MSPED:
        processMSPED(codeGen, md.EntityName());
        break;

    case eOpcodes::DIR:
        processDIR(codeGen, md.EntityName());
        break;

    case eOpcodes::TURNGEN:
        WriteTodo(codeGen, md.EntityName(), "TURNGEN");
        break;

    case eOpcodes::TURN:
        WriteTodo(codeGen, md.EntityName(), "TURN");
        break;

    case eOpcodes::DIRA:
        WriteTodo(codeGen, md.EntityName(), "DIRA");
        break;

    case eOpcodes::GETDIR:
        WriteTodo(codeGen, md.EntityName(), "GETDIR");
        break;

    case eOpcodes::GETAXY:
        WriteTodo(codeGen, md.EntityName(), "GETAXY");
        break;

    case eOpcodes::GETAI:
        processGETAI(codeGen, eng);
        break;

    case eOpcodes::ANIM_2:
        WriteTodo(codeGen, md.EntityName(), "ANIM_2");
        break;

    case eOpcodes::CANIM2:
        WriteTodo(codeGen, md.EntityName(), "CANIM2");
        break;

    case eOpcodes::CANM_2:
        WriteTodo(codeGen, md.EntityName(), "CANM_2");
        break;

    case eOpcodes::ASPED:
        WriteTodo(codeGen, md.EntityName(), "ASPED");
        break;

    case eOpcodes::CC:
        WriteTodo(codeGen, md.EntityName(), "CC");
        break;

    case eOpcodes::JUMP:
        WriteTodo(codeGen, md.EntityName(), "JUMP");
        break;

    case eOpcodes::AXYZI:
        WriteTodo(codeGen, md.EntityName(), "AXYZI");
        break;

    case eOpcodes::LADER:
        WriteTodo(codeGen, md.EntityName(), "LADER");
        break;

    case eOpcodes::OFST:
        WriteTodo(codeGen, md.EntityName(), "OFST");
        break;

    case eOpcodes::OFSTW:
        WriteTodo(codeGen, md.EntityName(), "OFSTW");
        break;

    case eOpcodes::TALKR:
        WriteTodo(codeGen, md.EntityName(), "TALKR");
        break;

    case eOpcodes::SLIDR:
        WriteTodo(codeGen, md.EntityName(), "SLIDR");
        break;

    case eOpcodes::SOLID:
        processSOLID(codeGen, md.EntityName());
        break;

    case eOpcodes::TLKR2:
        WriteTodo(codeGen, md.EntityName(), "TLKR2");
        break;

    case eOpcodes::SLDR2:
        WriteTodo(codeGen, md.EntityName(), "SLDR2");
        break;

    case eOpcodes::CCANM:
        WriteTodo(codeGen, md.EntityName(), "CCANM");
        break;

    case eOpcodes::FCFIX:
        WriteTodo(codeGen, md.EntityName(), "FCFIX");
        break;

    case eOpcodes::ANIMB:
        WriteTodo(codeGen, md.EntityName(), "ANIMB");
        break;

    case eOpcodes::TURNW:
        WriteTodo(codeGen, md.EntityName(), "TURNW");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7ModelInstruction::processTLKON(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_talkable( %2% )") % entity % FF7CodeGeneratorHelpers::FormatInvertedBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7ModelInstruction::processPC(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("set_entity_to_character( \"%1%\", \"%1%\" )") % entity).str());
}

void FF7::FF7ModelInstruction::processCHAR(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1% = entity_manager:get_entity( \"%1%\" )") % entity).str());
}

void FF7::FF7ModelInstruction::processDFANM(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_default_animation( %2% ) -- speed %3%") % entity % _params[0]->getUnsigned() % (1.0f / _params[1]->getUnsigned())).str());
    codeGen->addOutputLine((boost::format("self.%1%:play_animation( %2% )") % entity % _params[0]->getUnsigned()).str());
}

void FF7::FF7ModelInstruction::processANIME1(CodeGenerator* codeGen, const std::string& entity)
{
    // ID will be fixed-up downstream
    codeGen->addOutputLine((boost::format("self.%1%:play_animation( %2% ) -- speed %3%") % entity % _params[0]->getUnsigned() % (1.0f / _params[1]->getUnsigned())).str());
    codeGen->addOutputLine((boost::format("self.%1%:animation_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processVISI(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_visible( %2% )") % entity % FF7CodeGeneratorHelpers::FormatBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7ModelInstruction::processXYZI(CodeGenerator* codeGen, const std::string& entity)
{
    // TODO: variable scale
    float scale = 128.0f;
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[4]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[5]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& z = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[2]->getUnsigned(), _params[6]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& triangleId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[3]->getUnsigned(), _params[7]->getUnsigned());
    codeGen->addOutputLine((boost::format("self.%1%:set_position( %2%, %3%, %4% ) -- triangle ID %5%") % entity % x % y % z % triangleId).str());
}

void FF7::FF7ModelInstruction::processMOVE(CodeGenerator* codeGen, const std::string& entity)
{
    // TODO: variable scale
    float scale = 128.0f;
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    codeGen->addOutputLine((boost::format("self.%1%:move_to_position( %2%, %3% )") % entity % x % y).str());
    codeGen->addOutputLine((boost::format("self.%1%:move_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processGETAI(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    const auto& entity = engine.EntityByIndex(_params[2]->getUnsigned());
    // TODO: check for assignment to literal
    const auto& variable = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("local entity = entity_manager:get_entity( \"%1%\" )") % entity.Name()).str());
    codeGen->addOutputLine((boost::format("%1% = entity:get_move_triangle_id()") % variable).str());
}

void FF7::FF7ModelInstruction::processMSPED(CodeGenerator* codeGen, const std::string& entity)
{
    // TODO: variable scale
    float scale = 128.0f;
    const auto& speed = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[2]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 256.0f * scale / 30.0f);
    codeGen->addOutputLine((boost::format("self.%1%:set_move_auto_speed( %2% )") % entity % speed).str());
}

void FF7::FF7ModelInstruction::processDIR(CodeGenerator* codeGen, const std::string& entity)
{
    const auto& direction = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[1]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 256.0f / 360.0f);
    codeGen->addOutputLine((boost::format("self.%1%:set_rotation( %2% )") % entity % direction).str());
}

void FF7::FF7ModelInstruction::processSOLID(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_solid( %2% )") % entity % FF7CodeGeneratorHelpers::FormatInvertedBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7WalkmeshInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::SLIP:
        WriteTodo(codeGen, md.EntityName(), "SLIP");
        break;

    case eOpcodes::UC:
        WriteTodo(codeGen, md.EntityName(), "UC");
        break;

    case eOpcodes::IDLCK:
        WriteTodo(codeGen, md.EntityName(), "IDLCK");
        break;

    case eOpcodes::LINE:
        WriteTodo(codeGen, md.EntityName(), "LINE");
        break;

    case eOpcodes::LINON:
        WriteTodo(codeGen, md.EntityName(), "LINON");
        break;

    case eOpcodes::SLINE:
        WriteTodo(codeGen, md.EntityName(), "SLINE");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7BackgroundInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::BGPDH:
        WriteTodo(codeGen, md.EntityName(), "BGPDH");
        break;

    case eOpcodes::BGSCR:
        WriteTodo(codeGen, md.EntityName(), "BGSCR");
        break;

    case eOpcodes::MPPAL:
        WriteTodo(codeGen, md.EntityName(), "MPPAL");
        break;

    case eOpcodes::BGON:
        processBGON(codeGen);
        break;

    case eOpcodes::BGOFF:
        processBGOFF(codeGen);
        break;

    case eOpcodes::BGROL:
        WriteTodo(codeGen, md.EntityName(), "BGROL");
        break;

    case eOpcodes::BGROL2:
        WriteTodo(codeGen, md.EntityName(), "BGROL2");
        break;

    case eOpcodes::BGCLR:
        processBGCLR(codeGen);
        break;

    case eOpcodes::STPAL:
        processSTPAL(codeGen);
        break;

    case eOpcodes::LDPAL:
        processLDPAL(codeGen);
        break;

    case eOpcodes::CPPAL:
        processCPPAL(codeGen);
        break;

    case eOpcodes::RTPAL:
        WriteTodo(codeGen, md.EntityName(), "RTPAL");
        break;

    case eOpcodes::ADPAL:
        processADPAL(codeGen);
        break;

    case eOpcodes::MPPAL2:
        processMPPAL2(codeGen);
        break;

    case eOpcodes::STPLS:
        processSTPLS(codeGen);
        break;

    case eOpcodes::LDPLS:
        processLDPLS(codeGen);
        break;

    case eOpcodes::CPPAL2:
        WriteTodo(codeGen, md.EntityName(), "CPPAL2");
        break;

    case eOpcodes::ADPAL2:
        WriteTodo(codeGen, md.EntityName(), "ADPAL2");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7BackgroundInstruction::processBGON(CodeGenerator* codeGen)
{
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& layerId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_on( %1%, %2% )") % backgroundId % layerId).str());
}

void FF7::FF7BackgroundInstruction::processBGOFF(CodeGenerator* codeGen)
{
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& layerId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_off( %1%, %2% )") % backgroundId % layerId).str());
}

void FF7::FF7BackgroundInstruction::processBGCLR(CodeGenerator* codeGen)
{
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_clear( %1% )") % backgroundId).str());
}

void FF7::FF7BackgroundInstruction::processSTPAL(CodeGenerator* codeGen)
{
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- store palette %1% to position %2%, start CLUT index 0, %3% entries") % source % destination % (_params[4]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processLDPAL(CodeGenerator* codeGen)
{
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- load palette %2% from position %1%, start CLUT index 0, %3% entries") % source % destination % (_params[4]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processCPPAL(CodeGenerator* codeGen)
{
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- copy palette %1% to palette %2%, %3% entries") % source % destination % (_params[4]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processADPAL(CodeGenerator* codeGen)
{
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[6]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[7]->getUnsigned());
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[4]->getUnsigned(), _params[10]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[3]->getUnsigned(), _params[9]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[2]->getUnsigned(), _params[8]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- add RGB(%3%, %4%, %5%) to %6% entries of palette stored at position %1%, storing result in position %2%") % source % destination % r % g % b % (_params[11]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processMPPAL2(CodeGenerator* codeGen)
{
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[0]->getUnsigned(), _params[6]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[1]->getUnsigned(), _params[7]->getUnsigned());
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[4]->getUnsigned(), _params[10]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[3]->getUnsigned(), _params[9]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(_params[2]->getUnsigned(), _params[8]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- multiply RGB(%3%, %4%, %5%) by %6% entries of palette stored at position %1%, storing result in position %2%") % source % destination % r % g % b % (_params[11]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processSTPLS(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- store palette %1% to position %2%, start CLUT index %3%, %4% entries") % _params[0]->getUnsigned() % _params[1]->getUnsigned() % _params[2]->getUnsigned() % (_params[3]->getUnsigned() + 1)).str());
}

void FF7::FF7BackgroundInstruction::processLDPLS(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- load palette %2% from position %1%, start CLUT index %3%, %4% entries") % _params[0]->getUnsigned() % _params[1]->getUnsigned() % _params[2]->getUnsigned() % (_params[3]->getUnsigned() + 1)).str());
}

void FF7::FF7CameraInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::NFADE:
        WriteTodo(codeGen, md.EntityName(), "NFADE");
        break;

    case eOpcodes::SHAKE:
        WriteTodo(codeGen, md.EntityName(), "SHAKE");
        break;

    case eOpcodes::SCRLO:
        WriteTodo(codeGen, md.EntityName(), "SCRLO");
        break;

    case eOpcodes::SCRLC:
        WriteTodo(codeGen, md.EntityName(), "SCRLC");
        break;

    case eOpcodes::SCR2D:
        WriteTodo(codeGen, md.EntityName(), "SCR2D");
        break;

    case eOpcodes::SCRCC:
        WriteTodo(codeGen, md.EntityName(), "SCRCC");
        break;

    case eOpcodes::SCR2DC:
        WriteTodo(codeGen, md.EntityName(), "SCR2DC");
        break;

    case eOpcodes::SCRLW:
        WriteTodo(codeGen, md.EntityName(), "SCRLW");
        break;

    case eOpcodes::SCR2DL:
        WriteTodo(codeGen, md.EntityName(), "SCR2DL");
        break;

    case eOpcodes::VWOFT:
        WriteTodo(codeGen, md.EntityName(), "VWOFT");
        break;

    case eOpcodes::FADE:
        WriteTodo(codeGen, md.EntityName(), "FADE");
        break;

    case eOpcodes::FADEW:
        WriteTodo(codeGen, md.EntityName(), "FADEW");
        break;

    case eOpcodes::SCRLP:
        WriteTodo(codeGen, md.EntityName(), "SCRLP");
        break;

    case eOpcodes::MVCAM:
        WriteTodo(codeGen, md.EntityName(), "MVCAM");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7AudioVideoInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::BGMOVIE:
        WriteTodo(codeGen, md.EntityName(), "BGMOVIE");
        break;

    case eOpcodes::AKAO2:
        WriteTodo(codeGen, md.EntityName(), "AKAO2");
        break;

    case eOpcodes::MUSIC:
        processMUSIC(codeGen);
        break;

    case eOpcodes::SOUND:
        WriteTodo(codeGen, md.EntityName(), "SOUND");
        break;

    case eOpcodes::AKAO:
        WriteTodo(codeGen, md.EntityName(), "AKAO");
        break;

    case eOpcodes::MUSVT:
        WriteTodo(codeGen, md.EntityName(), "MUSVT");
        break;

    case eOpcodes::MUSVM:
        WriteTodo(codeGen, md.EntityName(), "MUSVM");
        break;

    case eOpcodes::MULCK:
        WriteTodo(codeGen, md.EntityName(), "MULCK");
        break;

    case eOpcodes::BMUSC:
        WriteTodo(codeGen, md.EntityName(), "BMUSC");
        break;

    case eOpcodes::CHMPH:
        WriteTodo(codeGen, md.EntityName(), "CHMPH");
        break;

    case eOpcodes::PMVIE:
        WriteTodo(codeGen, md.EntityName(), "PMVIE");
        break;

    case eOpcodes::MOVIE:
        WriteTodo(codeGen, md.EntityName(), "MOVIE");
        break;

    case eOpcodes::MVIEF:
        WriteTodo(codeGen, md.EntityName(), "MVIEF");
        break;

    case eOpcodes::FMUSC:
        WriteTodo(codeGen, md.EntityName(), "FMUSC");
        break;

    case eOpcodes::CMUSC:
        WriteTodo(codeGen, md.EntityName(), "CMUSC");
        break;

    case eOpcodes::CHMST:
        WriteTodo(codeGen, md.EntityName(), "CHMST");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7AudioVideoInstruction::processMUSIC(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- music:execute_akao( 0x10, pointer_to_field_AKAO_%1% )") % _params[0]->getUnsigned()).str());
}

void FF7::FF7UncategorizedInstruction::processInst(Function& func, ValueStack&, Engine* engine, CodeGenerator *codeGen)
{
    FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::MPDSP:
        WriteTodo(codeGen, md.EntityName(), "MPDSP");
        break;

    case eOpcodes::SETX:
        WriteTodo(codeGen, md.EntityName(), "SETX");
        break;

    case eOpcodes::GETX:
        WriteTodo(codeGen, md.EntityName(), "GETX");
        break;

    case eOpcodes::SEARCHX:
        WriteTodo(codeGen, md.EntityName(), "SEARCHX");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7NoOperationInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}
