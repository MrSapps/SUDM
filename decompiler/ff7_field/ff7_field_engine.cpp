#include "ff7_field_engine.h"
#include "ff7_field_disassembler.h"
#include "ff7_field_codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include "make_unique.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

/*
OpCodes which need implementing (already done in DAT dumper)

"BLINK"
"XYI"
"CMOVE"
"MOVA"
"TURA"
"ANIMW"
"FMOVE"
"ANIME2"
"ANIM_1"
"CANIM1" ?
"CANM_1"
"TURN"
"DIRA"
"GETDIR"
"GETAXY"
"JUMP"
"AXYZI"
"LADER"
"OFST"
"TALKR"
"ANIMB"
"TURNW"
*/

std::unique_ptr<Disassembler> FF7::FF7FieldEngine::getDisassembler(InstVec &insts, const std::vector<unsigned char>& rawScriptData)
{
    auto ret = std::make_unique<FF7Disassembler>(mFormatter, this, insts, rawScriptData);
    mScaleFactor = ret->ScaleFactor();
    return std::move(ret);
}

std::unique_ptr<Disassembler> FF7::FF7FieldEngine::getDisassembler(InstVec &insts)
{
    auto ret = std::make_unique<FF7Disassembler>(mFormatter, this, insts);
    mScaleFactor = ret->ScaleFactor();
    return std::move(ret);
}

std::unique_ptr<CodeGenerator> FF7::FF7FieldEngine::getCodeGenerator(const InstVec& insts, std::ostream &output)
{
    // dessert: the broken version
    //return std::make_unique<FF7CodeGenerator>(this, insts, output);

    // dessert: the not-as-nice-but-at-least-it-works version
    return std::make_unique<FF7SimpleCodeGenerator>(this, insts, output, mFormatter);
}

void FF7::FF7FieldEngine::postCFG(InstVec& /*insts*/, Graph /*g*/)
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

void FF7::FF7CondJumpInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

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
        ValuePtr v = new UnqotedStringValue(funcName + "(" + std::to_string(_params[0]->getUnsigned()) + ")");
        stack.push(v);
        return;
    }

    std::string op;
    uint32 type = _params[4]->getUnsigned();
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());

    switch (type)
    {
    case 0:
        op = "==";
        break;

    case 1:
        op = "~=";
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
    {
        op = "hasbit("+ source + ", bit(" + destination + "))";
        ValuePtr v = new UnqotedStringValue(op);
        stack.push(v);
    }
        return;

    case 0xA:
    {
        op = "not hasbit(" + source + ", bit(" + destination + "))";
        ValuePtr v = new UnqotedStringValue(op);
        stack.push(v);
    }
        return;

    default:
        throw UnknownConditionalOperatorException(_address, type);
    }


    ValuePtr v = new BinaryOpValue(new VarValue(source), new VarValue(destination), op);
    stack.push(v); 
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


void FF7::FF7UncondJumpInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator* cg)
{
    switch (_opcode)
    {
    case eOpcodes::JMPB:
    case eOpcodes::JMPBL:
        // HACK: Hard loop will hang the game, insert a wait to yield control
        cg->addOutputLine("-- Hack, yield control for possible inf loop");
        cg->addOutputLine("script:wait(0)");
        break;
    }
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
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
    const auto& scriptName = cg->mFormatter.FunctionName(entity.Name(), entity.FunctionByIndex(_params[2]->getUnsigned()));
    auto priority = _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("script:request( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % priority).str());
}

void FF7::FF7ControlFlowInstruction::processREQSW(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
    const auto& scriptName = cg->mFormatter.FunctionName(entity.Name(), entity.FunctionByIndex(_params[2]->getUnsigned()));
    auto priority = _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("script:request_start_sync( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % priority).str());
}

void FF7::FF7ControlFlowInstruction::processREQEW(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    try
    {
        FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
        const auto& entity = engine.EntityByIndex(_params[0]->getSigned());
        const auto& scriptName = cg->mFormatter.FunctionName(entity.Name(), entity.FunctionByIndex(_params[2]->getUnsigned()));
        auto priority = _params[1]->getUnsigned();
        codeGen->addOutputLine((boost::format("script:request_end_sync( Script.ENTITY, \"%1%\", \"%2%\", %3% )") % entity.Name() % scriptName % priority).str());
    }
    catch (const InternalDecompilerError&)
    {
        codeGen->addOutputLine((boost::format("-- ERROR call to non existing function index %1%") % _params[2]->getUnsigned()).str());
    }

}

void FF7::FF7ControlFlowInstruction::processRETTO(CodeGenerator* codeGen)
{
    auto entityIndex = _params[0]->getUnsigned();
    auto priority = _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("-- return_to( script_id_in_current_entity=%2%, priority=%1% )") % entityIndex % priority).str());
}

void FF7::FF7ControlFlowInstruction::processWAIT(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("script:wait( %1% )") % (_params[0]->getUnsigned() / 30.0f)).str());
}

void FF7::FF7ModuleInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::DSKCG:
        WriteTodo(codeGen, md.EntityName(), "DSKCG");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::ARROW:
        codeGen->addOutputLine((boost::format("game:pointer_enable(%1%)") % (_params[0]->getUnsigned() ? "true" : "false")).str());
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::PNAME:
        WriteTodo(codeGen, md.EntityName(), "PNAME");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::GMSPD:
        WriteTodo(codeGen, md.EntityName(), "GMSPD");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::SMSPD:
        WriteTodo(codeGen, md.EntityName(), "SMSPD");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::FLMAT:
        codeGen->addOutputLine("game:fill_materia()");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::FLITM:
        WriteTodo(codeGen, md.EntityName(), "FLITM");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::BTLCK:
        codeGen->addOutputLine((boost::format("game:battle_enable(%1%)") % (_params[0]->getUnsigned() ? "true" : "false")).str());
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::MVLCK:
        codeGen->addOutputLine((boost::format("game:movie_enable(%1%)") % (_params[0]->getUnsigned() ? "true" : "false")).str());
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::SPCNM:
        WriteTodo(codeGen, md.EntityName(), "SPCNM");
        break;

    case (eOpcodes::SPECIAL << 8) | eSpecialOpcodes::RSGLB:
        codeGen->addOutputLine("game:global_reset()");
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
        processMAPJUMP(codeGen, func);
        break;

    case eOpcodes::LSTMP:
        WriteTodo(codeGen, md.EntityName(), "LSTMP");
        break;

    case eOpcodes::BATTLE:
        processBATTLE(codeGen);
        break;

    case eOpcodes::BTLON:
        processBTLON(codeGen);
        break;

    case eOpcodes::BTLMD:
        WriteTodo(codeGen, md.EntityName(), "BTLMD");
        break;

    case eOpcodes::MPJPO:
        // Gateway function will do nothing if this is set to true
        codeGen->addOutputLine(std::string("FFVII.Data.DisableGateways=") + (_params[0]->getUnsigned() ? "true" : "false"));
        break;

    case eOpcodes::PMJMP:
        // Prepare to change map, don't need to output anything for this
        codeGen->addOutputLine("-- Prepare map change");
        break;

    case eOpcodes::PMJMP2:
        // Prepare to change map, don't need to output anything for this, seems to be the same thing as PMJMP
        codeGen->addOutputLine("-- Prepare map change 2");
        break;

    case eOpcodes::GAMEOVER:
        WriteTodo(codeGen, md.EntityName(), "GAMEOVER");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7ModuleInstruction::processBATTLE(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& battleId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:battle_run( %1% )") % battleId).str());
}

void FF7::FF7ModuleInstruction::processBTLON(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("field:random_encounter_on( %1% )") % FF7CodeGeneratorHelpers::FormatInvertedBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7ModuleInstruction::processMAPJUMP(CodeGenerator* codeGen, Function& func)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto targetMapId = _params[0]->getUnsigned();

    FunctionMetaData md(func._metadata);
    const std::string sourceSpawnPointName = cg->mFormatter.SpawnPointName(targetMapId, md.EntityName(), func._name, _address);

    cg->mFormatter.AddSpawnPoint(targetMapId,
        md.EntityName(), 
        func._name,
        _address,
        _params[1]->getSigned(), // X
        _params[2]->getSigned(), // Y
        _params[3]->getSigned(), // Walk mesh triangle ID
        _params[4]->getSigned());

    const std::string targetMapName = cg->mFormatter.MapName(targetMapId);
    codeGen->addOutputLine("load_field_map_request(\"" + targetMapName + "\", \"" + sourceSpawnPointName + "\")");
}

void FF7::FF7MathInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

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
        processBITON(codeGen);
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
        auto s = FF7::FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, srcBank, srcAddrOrValue);
        const uint32 dstBank = _params[1]->getUnsigned();
        const uint32 dstAddr = _params[3]->getUnsigned();
        auto d = FF7::FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, dstBank, dstAddr);

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
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("--if (%1% > 255); %1% = 255; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedPLUS2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("--if (%1% > 32767); %1% = 32767; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedMINUS(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("--if (%1% < 0); %1% = 0; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedMINUS2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - %2%") % lhs % rhs).str());
    codeGen->addOutputLine((boost::format("--if (%1% < 0); %1% = 0; end") % lhs).str());
}

void FF7::FF7MathInstruction::processSaturatedINC(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + 1") % destination).str());
    codeGen->addOutputLine((boost::format("--if (%1% > 255); %1% = 255; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedINC2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and negative wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% + 1") % destination).str());
    codeGen->addOutputLine((boost::format("--if (%1% > 32767); %1% = 32767; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedDEC(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - 1") % destination).str());
    codeGen->addOutputLine((boost::format("--if (%1% < 0); %1% = 0; end") % destination).str());
}

void FF7::FF7MathInstruction::processSaturatedDEC2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and positive wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% - 1") % destination).str());
    codeGen->addOutputLine((boost::format("--if (%1% < 0); %1% = 0; end") % destination).str());
}

void FF7::FF7MathInstruction::processRDMSD(CodeGenerator* codeGen)
{
    // TODO: we don't have os.time...
    // TODO: RNG emulation?
    codeGen->addOutputLine("math.randomseed( os.time() )");
}

void FF7::FF7MathInstruction::processSETBYTE_SETWORD(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: respect destination bank sizes (16-bit writes only affect low byte)
    codeGen->addOutputLine((boost::format("%1% = %2%") % destination % source).str());
}

void FF7::FF7MathInstruction::processBITON(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& bitIndex = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: respect destination bank sizes (16-bit writes only affect low byte)
    // TODO: Lua didn't get bitwise ops until a later version than we are using. these need to be handled C++-side
    codeGen->addOutputLine((boost::format("-- %1% = %1% | (0x01 << %2%) -- alt: %1% = bit32.bor( %1%, bit32.lshift( 1, %2% ) )") % destination % bitIndex).str());
}

void FF7::FF7MathInstruction::processPLUSx_MINUSx(CodeGenerator* codeGen, const std::string& op)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& lhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& rhs = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    // TODO: repect destination bank sizes and wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% %2% %3%") % lhs % op % rhs).str());
}

void FF7::FF7MathInstruction::processINCx_DECx(CodeGenerator* codeGen, const std::string& op)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes and wraparound
    codeGen->addOutputLine((boost::format("%1% = %1% %2% 1") % destination % op).str());
}

void FF7::FF7MathInstruction::processRANDOM(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to value
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned());
    // TODO: repect destination bank sizes (16-bit writes only affect low byte)
    // TODO: RNG emulation?
    codeGen->addOutputLine((boost::format("%1% = math.random( 0, 255 )") % destination).str());
}

// ===========================================================================================================================================================

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
        processMESSAGE(codeGen, eng.ScriptName());
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
        processMENU2(codeGen);
        break;

    case eOpcodes::WINDOW:
        processWINDOW(codeGen);
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
        processWCLSE(codeGen);
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

void FF7::FF7WindowInstruction::processWINDOW(CodeGenerator* codeGen)
{
    // Init a new window - wont display till MESSAGE is used
    auto windowId = _params[0]->getUnsigned();
    auto x = _params[1]->getUnsigned();
    auto y = _params[2]->getUnsigned();
    auto width = _params[3]->getUnsigned();
    auto height = _params[4]->getUnsigned();
    codeGen->addOutputLine((boost::format("dialog:dialog_open( \"%1%\", %2%, %3%, %4%, %5%)") % windowId % x % y % width % height).str());
}

void FF7::FF7WindowInstruction::processMESSAGE(CodeGenerator* codeGen, const std::string& scriptName)
{
    // Displays a dialog in the WINDOW that has previously been initialized to display this dialog.
    auto windowId = _params[0]->getUnsigned();
    auto dialogId = _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("dialog:dialog_set_text( \"%1%\", \"%2%_%3%\" )") % windowId % scriptName % dialogId).str());
}

void FF7::FF7WindowInstruction::processWCLSE(CodeGenerator* codeGen)
{
    // Close a dialog
    auto windowId = _params[0]->getUnsigned();
    codeGen->addOutputLine((boost::format("dialog:dialog_close( \"%1%\" )") % windowId).str());
}

void FF7::FF7WindowInstruction::processMPNAM(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- field:map_name( %1% )") % _params[0]->getUnsigned()).str());
}

void FF7::FF7WindowInstruction::processMENU2(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- field:menu_lock( %1% )") % FF7CodeGeneratorHelpers::FormatBool(_params[0]->getUnsigned())).str());
}



// ===========================================================================================================================================================

void FF7::FF7PartyInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

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
        processSTITM(codeGen);
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
        processPRTYE(codeGen);
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

    case eOpcodes::CHGLD:
        WriteTodo(codeGen, md.EntityName(), "CHGLD");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7PartyInstruction::processSTITM(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    const auto& itemId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& amount = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("FFVII.add_item( %1%, %2% )") % itemId % amount).str());
}

void FF7::FF7PartyInstruction::processPRTYE(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* gc = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    auto characterId1 = gc->mFormatter.CharName(_params[0]->getUnsigned());
    characterId1 = (characterId1 == "") ? "nil" : ("\"" + characterId1 + "\"");
    auto characterId2 = gc->mFormatter.CharName(_params[1]->getUnsigned());
    characterId2 = (characterId2 == "") ? "nil" : ("\"" + characterId2 + "\"");
    auto characterId3 = gc->mFormatter.CharName(_params[2]->getUnsigned());
    characterId3 = (characterId3 == "") ? "nil" : ("\"" + characterId3 + "\"");
    codeGen->addOutputLine((boost::format("FFVII.set_party( %1%, %2%, %3% )") % characterId1 % characterId2 % characterId3).str());
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
        processDFANM(codeGen, md.EntityName(), md.CharacterId());
        break;

    case eOpcodes::ANIME1:
        processANIME1(codeGen, md.EntityName(), md.CharacterId());
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
        WriteTodo(codeGen, md.EntityName(), "CMOVE");
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
        processTURNGEN(codeGen, md.EntityName());
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
        processANIM_2(codeGen, md.EntityName(), md.CharacterId());
        break;

    case eOpcodes::CANIM2:
        processCANIM2(codeGen, md.EntityName(), md.CharacterId());
        break;

    case eOpcodes::CANM_2:
        processCANM_2(codeGen, md.EntityName(), md.CharacterId());
        break;

    case eOpcodes::ASPED:
        WriteTodo(codeGen, md.EntityName(), "ASPED");
        break;

    case eOpcodes::CC:
        processCC(codeGen, eng);
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
    {
        /* TODO: Implement me
        FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

        const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
        const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
        const auto& z = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
        const auto& speed = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());

        codeGen->addOutputLine((boost::format("%1%:offset_to_position( %2%, %3%, %4%, %5% )") % 
            md.EntityName() %
            x %
            y %
            z %
            (_params[2]->getUnsigned() ? "Entity.SMOOTH" : "Entity.LINEAR") %
            (_params[6]->getUnsigned() / 30.0f)
            ).str());

       
        export_script->Log(entity_list[i] + ":offset_to_position( " + 
            ParseGetVariable(GetU8(script + 1) >> 4, 
            (s16)GetU16LE(script + 4), 0, downscaler) + ", " + 
            ParseGetVariable(GetU8(script + 1) & 0x0F, 
            (s16)GetU16LE(script + 6), false, downscaler) + ", " + 
            ParseGetVariable(GetU8(script + 2) >> 4, 
            (s16)GetU16LE(script + 8), false, downscaler) + ", " + 
            ((type == 2) ? "Entity.SMOOTH" : "Entity.LINEAR") + ", " +
            ParseGetVariable(GetU8(script + 2) & 0x0F, GetU16LE(script + 10), 0, 30.0f) + " )\n");
            */
        WriteTodo(codeGen, md.EntityName(), "OFST");
      
    }
        break;

    case eOpcodes::OFSTW:
        codeGen->addOutputLine(md.EntityName() + ":offset_sync()");
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

void FF7::FF7ModelInstruction::processDFANM(CodeGenerator* codeGen, const std::string& entity, int charId)
{
    // ID will be fixed-up downstream
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    auto animationId = _params[0]->getUnsigned();
    // TODO: check for zero
    auto speed = 1.0f / _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("self.%1%:set_default_animation( \"%2%\" ) -- speed %3%") % entity % cg->mFormatter.AnimationName(charId, animationId) % speed).str());

    codeGen->addOutputLine((boost::format("self.%1%:play_animation( \"%2%\" )") % entity % cg->mFormatter.AnimationName(charId, animationId)).str());
}

void FF7::FF7ModelInstruction::processANIME1(CodeGenerator* codeGen, const std::string& entity, int charId)
{
    // ID will be fixed-up downstream
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    auto animationId = _params[0]->getUnsigned();
    // TODO: check for zero
    auto speed = 1.0f / _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("self.%1%:play_animation( \"%2%\" ) -- speed %3%") % entity % cg->mFormatter.AnimationName(charId, animationId) % speed).str());
    codeGen->addOutputLine((boost::format("self.%1%:animation_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processVISI(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_visible( %2% )") % entity % FF7CodeGeneratorHelpers::FormatBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7ModelInstruction::processXYZI(CodeGenerator* codeGen, const std::string& entity)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const float scale = 128.0f * cg->ScaleFactor();
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[4]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[5]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& z = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[6]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& triangleId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[7]->getUnsigned());
    codeGen->addOutputLine((boost::format("self.%1%:set_position( %2%, %3%, %4% ) -- triangle ID %5%") % entity % x % y % z % triangleId).str());
}

void FF7::FF7ModelInstruction::processMOVE(CodeGenerator* codeGen, const std::string& entity)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const float scale = 128.0f * cg->ScaleFactor();
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getSigned(), FF7CodeGeneratorHelpers::ValueType::Float, scale);
    codeGen->addOutputLine((boost::format("self.%1%:move_to_position( %2%, %3% )") % entity % x % y).str());
    codeGen->addOutputLine((boost::format("self.%1%:move_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processMSPED(CodeGenerator* codeGen, const std::string& entity)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const float scale = 128.0f * cg->ScaleFactor();
    const auto& speed = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[2]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 256.0f * scale / 30.0f);
    codeGen->addOutputLine((boost::format("self.%1%:set_move_auto_speed( %2% )") % entity % speed).str());
}

void FF7::FF7ModelInstruction::processDIR(CodeGenerator* codeGen, const std::string& entity)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    const auto& degrees = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[1]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 256.0f / 360.0f);
    codeGen->addOutputLine((boost::format("self.%1%:set_rotation( %2% )") % entity % degrees).str());
}

void FF7::FF7ModelInstruction::processTURNGEN(CodeGenerator* codeGen, const std::string& entity)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    const auto& degrees = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[2]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 256.0f / 360.0f);

    std::string direction;
    switch (_params[3]->getUnsigned())
    {
    case 0:
        direction = "Entity.CLOCKWISE";
        break;
    case 1:
        direction = "Entity.ANTICLOCKWISE";
        break;
    case 2:
        direction = "Entity.CLOSEST";
        break;
    default:
        codeGen->addOutputLine((boost::format("-- log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: unknown rotation direction %3%\")") % entity % _address %_params[3]->getUnsigned()).str());
        direction = "Entity.CLOSEST";
        break;
    }

    auto steps = _params[4]->getUnsigned();

    std::string stepType;
    switch (_params[5]->getUnsigned())
    {
    case 1:
        stepType = "Entity.LINEAR";
        break;
    case 2:
        stepType = "Entity.SMOOTH";
        break;
    default:
        codeGen->addOutputLine((boost::format("-- log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: unknown step type %3%\")") % entity % _address %_params[5]->getUnsigned()).str());
        stepType = "Entity.SMOOTH";
        break;
    }

    const float scaledSteps = static_cast<float>(steps) / 30.0f;
    codeGen->addOutputLine((boost::format("self.%1%:turn_to_direction( %2%, %3%, %4%, %5% )") % entity % degrees % direction % stepType % scaledSteps).str());
    codeGen->addOutputLine((boost::format("self.%1%:turn_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processGETAI(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // TODO: check for assignment to literal
    const auto& variable = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    const auto& entity = engine.EntityByIndex(_params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("%1% = entity_manager:get_entity( \"%2%\" ):get_move_triangle_id()") % variable % entity.Name()).str());
}

void FF7::FF7ModelInstruction::processANIM_2(CodeGenerator* codeGen, const std::string& entity, int charId)
{
    // ID will be fixed-up downstream
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    auto animationId = _params[0]->getUnsigned();
    // TODO: check for zero
    auto speed = 1.0f / _params[1]->getUnsigned();
    codeGen->addOutputLine((boost::format("self.%1%:play_animation_stop( \"%2%\" ) -- speed %3%") % entity % cg->mFormatter.AnimationName(charId, animationId) % speed).str());
    codeGen->addOutputLine((boost::format("self.%1%:animation_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processCANIM2(CodeGenerator* codeGen, const std::string& entity, int charId)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    auto animationId = _params[0]->getUnsigned();
    auto startFrame = _params[1]->getUnsigned() / 30.0f;
    auto endFrame = _params[2]->getUnsigned() / 30.0f;
    // TODO: check for zero
    auto speed = 1.0f / _params[3]->getUnsigned();
    codeGen->addOutputLine((boost::format("self.%1%:play_animation( \"%2%\", %3%, %4% ) -- speed %5%") % entity % cg->mFormatter.AnimationName(charId, animationId) % startFrame % endFrame % speed).str());
    codeGen->addOutputLine((boost::format("self.%1%:animation_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processCANM_2(CodeGenerator* codeGen, const std::string& entity, int charId)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);

    // ID will be fixed-up downstream
    auto animationId = _params[0]->getUnsigned();
    auto startFrame = _params[1]->getUnsigned() / 30.0f;
    auto endFrame = _params[2]->getUnsigned() / 30.0f;
    // TODO: check for zero
    auto speed = 1.0f / _params[3]->getUnsigned();

    codeGen->addOutputLine((boost::format("self.%1%:play_animation_stop( \"%2%\", %3%, %4% ) -- speed %5%") % entity % cg->mFormatter.AnimationName(charId, animationId) % startFrame % endFrame % speed).str());
    codeGen->addOutputLine((boost::format("self.%1%:animation_sync()") % entity).str());
}

void FF7::FF7ModelInstruction::processCC(CodeGenerator* codeGen, const FF7FieldEngine& engine)
{
    const auto& entity = engine.EntityByIndex(_params[0]->getUnsigned());
    codeGen->addOutputLine((boost::format("entity_manager:set_player_entity( \"%1%\" )") % entity.Name()).str());
}

void FF7::FF7ModelInstruction::processSOLID(CodeGenerator* codeGen, const std::string& entity)
{
    codeGen->addOutputLine((boost::format("self.%1%:set_solid( %2% )") % entity % FF7CodeGeneratorHelpers::FormatInvertedBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7WalkmeshInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::SLIP:
        WriteTodo(codeGen, md.EntityName(), "SLIP");
        break;

    case eOpcodes::UC:
        processUC(codeGen);
        break;

    case eOpcodes::IDLCK:
        // Triangle id, on or off
        codeGen->addOutputLine(
            (boost::format("entity_manager:lock_walkmesh( %1%, %2% )")
            % _params[0]->getUnsigned()
            % FF7CodeGeneratorHelpers::FormatBool(_params[1]->getUnsigned())).str());
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

void FF7::FF7WalkmeshInstruction::processUC(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("entity_manager:player_lock( %1% )") % FF7CodeGeneratorHelpers::FormatBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7BackgroundInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

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

    case eOpcodes::RTPAL2:
        WriteTodo(codeGen, md.EntityName(), "RTPAL2");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7BackgroundInstruction::processBGON(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& layerId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_on( %1%, %2% )") % backgroundId % layerId).str());
}

void FF7::FF7BackgroundInstruction::processBGOFF(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& layerId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_off( %1%, %2% )") % backgroundId % layerId).str());
}

void FF7::FF7BackgroundInstruction::processBGCLR(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& backgroundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- field:background_clear( %1% )") % backgroundId).str());
}

void FF7::FF7BackgroundInstruction::processSTPAL(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    auto numEntries = _params[4]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- store palette %1% to position %2%, start CLUT index 0, %3% entries") % source % destination % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processLDPAL(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    auto numEntries = _params[4]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- load palette %2% from position %1%, start CLUT index 0, %3% entries") % source % destination % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processCPPAL(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    auto numEntries = _params[4]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- copy palette %1% to palette %2%, %3% entries") % source % destination % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processADPAL(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[6]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[7]->getUnsigned());
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[4]->getUnsigned(), _params[10]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[9]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[8]->getUnsigned());
    auto numEntries = _params[11]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- add RGB(%3%, %4%, %5%) to %6% entries of palette stored at position %1%, storing result in position %2%") % source % destination % r % g % b % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processMPPAL2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& source = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[6]->getUnsigned());
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[7]->getUnsigned());
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[4]->getUnsigned(), _params[10]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[9]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[8]->getUnsigned());
    auto numEntries = _params[11]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- multiply RGB(%3%, %4%, %5%) by %6% entries of palette stored at position %1%, storing result in position %2%") % source % destination % r % g % b % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processSTPLS(CodeGenerator* codeGen)
{
    auto source = _params[0]->getUnsigned();
    auto destination = _params[1]->getUnsigned();
    auto startClut = _params[2]->getUnsigned();
    auto numEntries = _params[3]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- store palette %1% to position %2%, start CLUT index %3%, %4% entries") % source % destination % startClut % numEntries).str());
}

void FF7::FF7BackgroundInstruction::processLDPLS(CodeGenerator* codeGen)
{
    auto source = _params[0]->getUnsigned();
    auto destination = _params[1]->getUnsigned();
    auto startClut = _params[2]->getUnsigned();
    auto numEntries = _params[3]->getUnsigned() + 1;
    codeGen->addOutputLine((boost::format("-- load palette %2% from position %1%, start CLUT index %3%, %4% entries") % source % destination % startClut % numEntries).str());
}

void FF7::FF7CameraInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::NFADE:
        processNFADE(codeGen);
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
        processSCR2D(codeGen);
        break;

    case eOpcodes::SCRCC:
        WriteTodo(codeGen, md.EntityName(), "SCRCC");
        break;

    case eOpcodes::SCR2DC:
        processSCR2DC(codeGen);
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
        processFADE(codeGen);
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

    case eOpcodes::SCRLA:
        WriteTodo(codeGen, md.EntityName(), "SCRLA");
        break;

    default:
        codeGen->addOutputLine(FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(md.EntityName(), _address, _opcode));
    }
}

void FF7::FF7CameraInstruction::processNFADE(CodeGenerator* codeGen)
{
    // TODO: not fully reversed
    auto rawType = _params[4]->getUnsigned();
    if (rawType == 0)
    {
        codeGen->addOutputLine("-- fade:clear()");
        return;
    }
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const std::string type = rawType == 12 ? "Fade.SUBTRACT" : "Fade.ADD";
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[5]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[6]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[7]->getUnsigned());
    const auto& unknown = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[8]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- fade:fade( %2%, %3%, %4%, %1%, %5% )") % type % r % g % b % unknown).str());
}

void FF7::FF7CameraInstruction::processSCR2D(CodeGenerator* codeGen)
{
    // kUpScaler

    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getSigned());
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getSigned());
    codeGen->addOutputLine((boost::format("background2d:scroll_to_position( %1% * 3, %2% * 3, Background2D.NONE, 0 )") % x % y).str());
}

void FF7::FF7CameraInstruction::processSCR2DC(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& x = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[4]->getSigned());
    const auto& y = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[5]->getSigned());
    const auto& speed = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[6]->getUnsigned(), FF7CodeGeneratorHelpers::ValueType::Float, 30.0f);
    codeGen->addOutputLine((boost::format("background2d:scroll_to_position( %1% * 3, %2% * 3, Background2D.SMOOTH, %3% )") % x % y % speed).str());
}

void FF7::FF7CameraInstruction::processFADE(CodeGenerator* codeGen)
{
    // TODO: not fully reversed
    auto rawType = _params[8]->getUnsigned();
    std::string type;
    switch (rawType)
    {
    case 1:
    case 2:
    case 7:
    case 8:
        type = "Fade.SUBTRACT";
        break;
    case 4:
        codeGen->addOutputLine("-- fade:black()");
        return;
    default:
        type = "Fade.ADD";
        break;
    }

    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& r = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[4]->getUnsigned());
    const auto& g = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[5]->getUnsigned());
    const auto& b = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[6]->getUnsigned());
    // TODO: needs to be divided by 30.0f?
    auto speed = _params[7]->getUnsigned();
    auto start = _params[9]->getUnsigned();
    codeGen->addOutputLine((boost::format("-- fade:fade( %2%, %3%, %4%, %1%, %5%, %6% )") % type % r % g % b % speed % start).str());
}

void FF7::FF7AudioVideoInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

    FunctionMetaData md(func._metadata);

    switch (_opcode)
    {
    case eOpcodes::BGMOVIE:
        WriteTodo(codeGen, md.EntityName(), "BGMOVIE");
        break;

    case eOpcodes::AKAO2:
        processAKAO2(codeGen);
        break;

    case eOpcodes::MUSIC:
        processMUSIC(codeGen);
        break;

    case eOpcodes::SOUND:
        processSOUND(codeGen);
        break;

    case eOpcodes::AKAO:
        processAKAO(codeGen);
        break;

    case eOpcodes::MUSVT:
        WriteTodo(codeGen, md.EntityName(), "MUSVT");
        break;

    case eOpcodes::MUSVM:
        WriteTodo(codeGen, md.EntityName(), "MUSVM");
        break;

    case eOpcodes::MULCK:
        processMULCK(codeGen);
        break;

    case eOpcodes::BMUSC:
        WriteTodo(codeGen, md.EntityName(), "BMUSC");
        break;

    case eOpcodes::CHMPH:
        WriteTodo(codeGen, md.EntityName(), "CHMPH");
        break;

    case eOpcodes::PMVIE:
        processPMVIE(codeGen);
        break;

    case eOpcodes::MOVIE:
        processMOVIE(codeGen);
        break;

    case eOpcodes::MVIEF:
        processMVIEF(codeGen);
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

void FF7::FF7AudioVideoInstruction::processAKAO2(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& param1 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[7]->getUnsigned());
    const auto& param2 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[8]->getUnsigned());
    const auto& param3 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[9]->getUnsigned());
    const auto& param4 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[10]->getUnsigned());
    const auto& param5 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[5]->getUnsigned(), _params[11]->getUnsigned());
    auto op = _params[6]->getUnsigned();
    codeGen->addOutputLine((boost::format("-- music:execute_akao( 0x%6$02x, %1%, %2%, %3%, %4%, %5% )") % param1 % param2 % param3 % param4 % param5 % op).str());
}

void FF7::FF7AudioVideoInstruction::processMUSIC(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- music:execute_akao( 0x10, pointer_to_field_AKAO_%1% )") % _params[0]->getUnsigned()).str());
}

void FF7::FF7AudioVideoInstruction::processSOUND(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& soundId = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[2]->getUnsigned());
    const auto& panning = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[3]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- music:execute_akao( 0x20, %1%, %2% )") % soundId % panning).str());
}

void FF7::FF7AudioVideoInstruction::processAKAO(CodeGenerator* codeGen)
{
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& param1 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[0]->getUnsigned(), _params[7]->getUnsigned());
    const auto& param2 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[8]->getUnsigned());
    const auto& param3 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[2]->getUnsigned(), _params[9]->getUnsigned());
    const auto& param4 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[3]->getUnsigned(), _params[10]->getUnsigned());
    const auto& param5 = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[5]->getUnsigned(), _params[11]->getUnsigned());
    auto op = _params[6]->getUnsigned();
    codeGen->addOutputLine((boost::format("-- music:execute_akao( 0x%6$02x, %1%, %2%, %3%, %4%, %5% )") % param1 % param2 % param3 % param4 % param5 % op).str());
}

void FF7::FF7AudioVideoInstruction::processMULCK(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- music:lock( %1% )") % FF7CodeGeneratorHelpers::FormatBool(_params[0]->getUnsigned())).str());
}

void FF7::FF7AudioVideoInstruction::processPMVIE(CodeGenerator* codeGen)
{
    codeGen->addOutputLine((boost::format("-- field:movie_set( %1% )") % _params[0]->getUnsigned()).str());
}

void FF7::FF7AudioVideoInstruction::processMOVIE(CodeGenerator* codeGen)
{
    codeGen->addOutputLine("-- field:play_movie()");
}

void FF7::FF7AudioVideoInstruction::processMVIEF(CodeGenerator* codeGen)
{
    // TODO: check for assignment to value
    FF7SimpleCodeGenerator* cg = static_cast<FF7SimpleCodeGenerator*>(codeGen);
    const auto& destination = FF7CodeGeneratorHelpers::FormatValueOrVariable(cg->mFormatter, _params[1]->getUnsigned(), _params[2]->getUnsigned());
    codeGen->addOutputLine((boost::format("-- %1% = field:get_movie_frame()") % destination).str());
}

void FF7::FF7UncategorizedInstruction::processInst(Function& func, ValueStack&, Engine* /*engine*/, CodeGenerator *codeGen)
{
    //FF7::FF7FieldEngine& eng = static_cast<FF7::FF7FieldEngine&>(*engine);

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
