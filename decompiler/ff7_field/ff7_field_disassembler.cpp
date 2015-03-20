#include "ff7_field_disassembler.h"
#include "ff7_field_engine.h"
#include "decompiler_engine.h"
#include <boost/format.hpp>
#include "lzs.h"
#include "make_unique.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

FF7::FF7Disassembler::FF7Disassembler(SUDM::IScriptFormatter& formatter, FF7FieldEngine* engine, InstVec& insts, const std::vector<unsigned char>& rawScriptData)
    : SimpleDisassembler(insts),
    mEngine(engine),
    mFormatter(formatter)
{
    mbFromRaw = true;
    kSectionPointersSize = 0; // If loading a raw section then we don't have a "sections header" to skip
    auto dataCopy = rawScriptData;
    mStream = std::make_unique<BinaryReader>(std::move(dataCopy));
}

FF7::FF7Disassembler::FF7Disassembler(SUDM::IScriptFormatter& formatter, FF7FieldEngine *engine, InstVec &insts)
    : SimpleDisassembler(insts),
    mEngine(engine),
    mFormatter(formatter)
{

}

FF7::FF7Disassembler::~FF7Disassembler()
{

}

void FF7::FF7Disassembler::open(const char *filename)
{
    // Read all of the file, decompress it, then stuff it into a stream
    mStream = std::make_unique<BinaryReader>(Lzs::Decompress(BinaryReader::ReadAll(filename)));
}

uint32 FF7::FF7Disassembler::GetEndOfScriptOffset(uint16 curEntryPoint, size_t entityIndex, size_t scriptIndex)
{
    uint16 nextEntryPoint = curEntryPoint;
    do
    {
        if (scriptIndex + 1 >= 32)
        {
            if (entityIndex + 1 >= mHeader.mEntityScripts.size())
            {
                // This is the very last script, so use the end of its data which is the offset to strings
                return mHeader.mOffsetToStrings;
            }
            else
            {
                // Wrap around to the next entity
                entityIndex++;
                scriptIndex = 0;
            }
        }
        else
        {
            // Get the next script in the same entity
            scriptIndex++;
        }
        nextEntryPoint = mHeader.mEntityScripts[entityIndex][scriptIndex];
    } while (nextEntryPoint == curEntryPoint);
    return nextEntryPoint;
}

std::unique_ptr<Function> FF7::FF7Disassembler::StartFunction(size_t scriptIndex)
{
    auto func = std::make_unique<Function>();
    func->_retVal = false;
    func->_args = 0;
    func->_name = "script_" + std::to_string(scriptIndex);
    func->mStartAddr = _address;
    return func;
}

struct ScriptInfo
{
    uint16 mEntryPoint;
    uint32 mNextEntryPoint;
    size_t mIndex;
};

void FF7::FF7Disassembler::doDisassemble() throw(std::exception)
{
    if (!mbFromRaw)
    {
        // First read the file section pointers
        for (int i = 0; i < kNumSections; i++)
        {
            mSections[i] = mStream->ReadU32();
        }

        // Now fix up from PSX RAM pointers to simple file offsets
        const uint32 basePtr = mSections[0];
        for (int i = 0; i < kNumSections; i++)
        {
            mSections[i] = (mSections[i] - basePtr) + kSectionPointersSize;
        }

        // Now seek to the script section
        mStream->Seek(mSections[eScript]);
    }

    // Read the script header
    mHeader.Read(*mStream);

    // Loop through the scripts for each entity
    for (size_t entityNumber = 0; entityNumber < mHeader.mEntityScripts.size(); entityNumber++)
    {
        const std::string entityName = mFormatter.EntityName(mHeader.mFieldEntityNames[entityNumber].data());

        // Only parse each script one
        std::set<uint16> parsedScripts;
        std::vector<ScriptInfo> scriptInfo;

        // Collect the scripts to parse
        for (size_t scriptIndex = 0; scriptIndex < mHeader.mEntityScripts[entityNumber].size(); scriptIndex++)
        {
            uint16 scriptEntryPoint = mHeader.mEntityScripts[entityNumber][scriptIndex];
            if (parsedScripts.find(scriptEntryPoint) != std::end(parsedScripts))
            {
                // If we've already parsed this scripts entry point, then don't do it again as it means
                // two scripts have the same entry which only seems to be true for "empty" scripts
                continue;
            }
            parsedScripts.insert(scriptEntryPoint);

            const uint32 nextScriptEntryPoint = GetEndOfScriptOffset(scriptEntryPoint, entityNumber, scriptIndex);
            const uint32 scriptSize = nextScriptEntryPoint - scriptEntryPoint;
            if (scriptSize > 0)
            {
                ScriptInfo info = { scriptEntryPoint, nextScriptEntryPoint, scriptIndex };
                scriptInfo.push_back(info);
            }
        }

        for (auto it = scriptInfo.begin(); it != scriptInfo.end(); it++)
        {
            const bool isStart = it == scriptInfo.begin();
            const bool isEnd = it == (--scriptInfo.end());
            DisassembleIndivdualScript(entityName, entityNumber, it->mIndex, it->mEntryPoint, it->mNextEntryPoint, isStart, isEnd);
        }
    }
}

static int FindId(uint32 startAddr, uint32 endAddr, const InstVec& insts)
{
    for (const InstPtr& instruction : insts)
    {
        if (instruction->_address >= startAddr && instruction->_address <= endAddr)
        {
            if (instruction->_opcode == FF7::eOpcodes::opCodeCHAR)
            {
                return instruction->_params[0]->getSigned();
            }
        }
    }
    return -1;
}

void FF7::FF7Disassembler::AddFunc(std::string entityName, size_t entityIndex, size_t scriptIndex, uint32 nextScriptEntryPoint, const bool isStart, bool isEnd, bool toReturnOnly, std::string funcName)
{

    const auto kScriptEntryPoint = mStream->Position();

    // Read each block of opcodes up to a return
    const size_t oldNumInstructions = _insts.size();

    auto func = StartFunction(scriptIndex);
    if (toReturnOnly)
    {
        // Read opcodes to the end or bail at the first return
        ReadOpCodesToPositionOrReturn(nextScriptEntryPoint + kSectionPointersSize);
        auto streamPos = mStream->Position();
        const size_t endPos = nextScriptEntryPoint + kSectionPointersSize;
        if (streamPos != endPos)
        {
            // Can't be the end if there is more data
            isEnd = false;
        }
    }
    else
    {
        while (mStream->Position() != nextScriptEntryPoint + kSectionPointersSize)
        {
            // Keep going till we have all of the script, i.e if we bail at a return then call
            // again till we have everything
            ReadOpCodesToPositionOrReturn(nextScriptEntryPoint + kSectionPointersSize);
        }
    }

    std::string metaData;
    if (isStart && isEnd)
    {
        metaData = "start_end_";
    }
    else if (isStart)
    {
        metaData = "start_";
    }
    else if (isEnd)
    {
        metaData = "end_";
    }


    const size_t newNumInstructions = _insts.size();
    func->mNumInstructions = newNumInstructions - oldNumInstructions;
    func->mEndAddr = _insts.back()->_address;
    if (!funcName.empty())
    {
        func->_name = funcName;
    }

    const int id = FindId(func->mStartAddr, func->mEndAddr, _insts);
    metaData += std::to_string(id) + "_" + entityName;
    func->_metadata = metaData;

    mEngine->_functions[kScriptEntryPoint] = *func;
    mEngine->AddEntityFunction(entityName, entityIndex, func->_name, scriptIndex);

}

void FF7::FF7Disassembler::DisassembleIndivdualScript(std::string entityName,
    size_t entityIndex,
    size_t scriptIndex,
    int16 scriptEntryPoint,
    uint32 nextScriptEntryPoint,
    bool isStart,
    bool isEnd)
{

    scriptEntryPoint += kSectionPointersSize;

    mStream->Seek(scriptEntryPoint);

    _addressBase = mStream->Position();
    _address = _addressBase;

    // "Normal" script
    if (scriptIndex > 0)
    {
        AddFunc(entityName, entityIndex, scriptIndex, nextScriptEntryPoint, isStart, isEnd, false, "");
    }
    else
    {
        const size_t endPos = nextScriptEntryPoint + kSectionPointersSize;

        // Read the init script, which means stop at the first return
        AddFunc(entityName, entityIndex, scriptIndex, nextScriptEntryPoint, isStart, isEnd, true, "init");

        // Not at the end of this script? Then the remaining data is the "main" script
        auto streamPos = mStream->Position();
        if (streamPos != endPos)
        {
            // The "main" script we should also only have 1 return statement
            AddFunc(entityName, entityIndex, scriptIndex, nextScriptEntryPoint, false, isEnd, true, "main");
            streamPos = mStream->Position();
            if (streamPos != endPos)
            {
                // We only ever expect 2 return statements in script 0
                // the first gives us the "init" script, the second gives
                // us the "main" script, anymore is an error
                throw TooManyReturnStatementsException();
            }
        }

    }
}

struct TInstructRecord
{
    unsigned char mOpCodeSize;
    unsigned int mOpCode;
    const char* mMnemonic;
    const char* mArgumentFormat;
    std::function<InstPtr()> mFactoryFunc;
};

const TInstructRecord kOpcodes[] =
{
    // Flow
    { 1, FF7::eOpcodes::RET, "RET", "", FF7::FF7ControlFlowInstruction::Create },
    { 1, FF7::eOpcodes::REQ, "REQ", "BU", FF7::FF7ControlFlowInstruction::Create },
    { 1, FF7::eOpcodes::REQSW, "REQSW", "BU", FF7::FF7ControlFlowInstruction::Create },
    { 1, FF7::eOpcodes::REQEW, "REQEW", "BU", FF7::FF7ControlFlowInstruction::Create },
    /*
    { 1, FF7::eOpcodes::PREQ, "PREQ", "BU", FF7::FF7ControlFlowInstruction },
    { 1, FF7::eOpcodes::PRQSW, "PRQSW", "BU", FF7::FF7ControlFlowInstruction },
    { 1, FF7::eOpcodes::PRQEW, "PRQEW", "BU", FF7::FF7ControlFlowInstruction },
    { 1, FF7::eOpcodes::RETTO, "RETTO", "U", FF7::FF7ControlFlowInstruction },
    { 1, FF7::eOpcodes::JMPF, "JMPF", "B", FF7::FF7UncondJumpInstruction },
    { 1, FF7::eOpcodes::JMPFL, "JMPFL", "w", FF7::FF7UncondJumpInstruction },
    { 1, FF7::eOpcodes::JMPB, "JMPB", "B", FF7::FF7UncondJumpInstruction },
    { 1, FF7::eOpcodes::JMPBL, "JMPBL", FF7::FF7UncondJumpInstruction, 0, "w" },
    { 1, FF7::eOpcodes::IFUB, "IFUB", FF7::FF7CondJumpInstruction, 0, "NBBBB" },
    { 1, FF7::eOpcodes::IFUBL, "IFUBL", FF7::FF7CondJumpInstruction, 0, "NBBBw" },
    { 1, FF7::eOpcodes::IFSW, "IFSW", FF7::FF7CondJumpInstruction, 0, "NwwBB" },
    { 1, FF7::eOpcodes::IFSWL, "IFSWL", FF7::FF7CondJumpInstruction, 0, "NwwBw" },
    { 1, FF7::eOpcodes::IFUW, "IFUW", FF7CondJumpInstruction, 0, "NwwBB" },
    { 1, FF7::eOpcodes::IFUWL, "IFUWL", FF7::FF7CondJumpInstruction, 0, "NwwBw" },
    { 1, FF7::eOpcodes::WAIT, "WAIT", FF7::FF7ControlFlowInstruction, 0, "w" },
    { 1, FF7::eOpcodes::IFKEY, "IFKEY", FF7::FF7CondJumpInstruction, 0, "wB" },
    { 1, FF7::eOpcodes::IFKEYON, "IFKEYON", FF7::FF7CondJumpInstruction, 0, "wB" },
    { 1, FF7::eOpcodes::IFKEYOFF, "IFKEYOFF", FF7::FF7CondJumpInstruction, 0, "wB" },*/
    { 1, FF7::eOpcodes::NOP, "NOP", "", FF7::FF7NoOperationInstruction::Create }/*,
                                                                                { 1, FF7::eOpcodes::IFPRTYQ, "IFPRTYQ", FF7::FF7CondJumpInstruction, 0, "BB" },
                                                                                { 1, FF7::eOpcodes::IFMEMBQ, "IFMEMBQ", FF7::FF7CondJumpInstruction, 0, "BB" },*/
};

std::vector<unsigned char> FF7::FF7Disassembler::Assemble(const std::string& input)
{
    // Convert the array to a map that we can query on by mnemonic
    std::map<std::string, const TInstructRecord*> mnemonicToInstructionRecords;
    for (size_t i = 0; i < _countof(kOpcodes); i++)
    {
        mnemonicToInstructionRecords[kOpcodes[i].mMnemonic] = &kOpcodes[i];
    }

    // TODO: Label parsing

    std::vector<unsigned char> r;

    std::stringstream is(input);

    InstVec insts;
    unsigned int address = 0;

    std::string line;
    while (std::getline(is, line))
    {
        std::vector<std::string> parts;
        boost::split(parts, line, boost::is_any_of(" "), boost::token_compress_on);

        auto it = mnemonicToInstructionRecords.find(parts[0]);
        if (it == std::end(mnemonicToInstructionRecords))
        {
            // Unknown opcode
        }

        InstPtr inst = it->second->mFactoryFunc();

        inst->_address = address;
        address += it->second->mOpCodeSize;

        // TODO: Parse params & size
        parts.push_back("TEST");
        parts.erase(parts.begin());
        readParams(inst, it->second->mArgumentFormat, parts);

        insts.push_back(inst);
    }

    // TODO: Serialize instructions back to raw byte code

    return r;
}

void FF7::FF7Disassembler::ReadOpCodesToPositionOrReturn(size_t endPos)
{
    /* Need all opcodes in the array before this will work
    // Convert the array to a map that we can query on by opcode
    std::map<unsigned int, const TInstructRecord*> opcodeToInstructionRecords;
    for (size_t i = 0; i < _countof(kOpcodes); i++)
    {
        opcodeToInstructionRecords[kOpcodes[i].mOpCode] = &kOpcodes[i];
    }

    while (mStream->Position() < endPos)
    {
        // See if the next data is a 1 byte opcode
        uint16 opcode = mStream->ReadU8();
        auto it = opcodeToInstructionRecords.find(opcode);
        _address++;
        if (it == std::end(opcodeToInstructionRecords))
        {
            // No, is it a 2 byte opcode?
            opcode = (mStream->ReadU8() << 8) + opcode;
            _address++;
            it = opcodeToInstructionRecords.find(opcode);
            if (it == std::end(opcodeToInstructionRecords))
            {
                // There are no instructions bigger than 2 bytes, so fail
                throw UnknownSubOpcodeException(_address, opcode);
            }
        }

        InstPtr inst = it->second->mFactoryFunc();
        inst->_opcode = opcode;
        inst->_address = _address;
        inst->_stackChange = 0;
        inst->_name = it->second->mMnemonic;
        readParams(inst, it->second->mArgumentFormat);
        _insts.push_back(inst);
    }
    */

    while (mStream->Position() < endPos)
    {
        uint8 opcode = mStream->ReadU8();
        uint32 full_opcode = 0;
        std::string opcodePrefix;
        switch (opcode)
        {
            // Flow
            OPCODE(eOpcodes::RET, "RET", FF7ControlFlowInstruction, 0, "");
            OPCODE(eOpcodes::REQ, "REQ", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::REQSW, "REQSW", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::REQEW, "REQEW", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::PREQ, "PREQ", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::PRQSW, "PRQSW", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::PRQEW, "PRQEW", FF7ControlFlowInstruction, 0, "BU");
            OPCODE(eOpcodes::RETTO, "RETTO", FF7ControlFlowInstruction, 0, "U");
            OPCODE(eOpcodes::JMPF, "JMPF", FF7UncondJumpInstruction, 0, "B");
            OPCODE(eOpcodes::JMPFL, "JMPFL", FF7UncondJumpInstruction, 0, "w");
            OPCODE(eOpcodes::JMPB, "JMPB", FF7UncondJumpInstruction, 0, "B");
            OPCODE(eOpcodes::JMPBL, "JMPBL", FF7UncondJumpInstruction, 0, "w");
            OPCODE(eOpcodes::IFUB, "IFUB", FF7CondJumpInstruction, 0, "NBBBB");
            OPCODE(eOpcodes::IFUBL, "IFUBL", FF7CondJumpInstruction, 0, "NBBBw");
            OPCODE(eOpcodes::IFSW, "IFSW", FF7CondJumpInstruction, 0, "NwwBB");
            OPCODE(eOpcodes::IFSWL, "IFSWL", FF7CondJumpInstruction, 0, "NwwBw");
            OPCODE(eOpcodes::IFUW, "IFUW", FF7CondJumpInstruction, 0, "NwwBB");
            OPCODE(eOpcodes::IFUWL, "IFUWL", FF7CondJumpInstruction, 0, "NwwBw");
            OPCODE(eOpcodes::WAIT, "WAIT", FF7ControlFlowInstruction, 0, "w");
            OPCODE(eOpcodes::IFKEY, "IFKEY", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::IFKEYON, "IFKEYON", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::IFKEYOFF, "IFKEYOFF", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::NOP, "NOP", FF7NoOperationInstruction, 0, "");
            OPCODE(eOpcodes::IFPRTYQ, "IFPRTYQ", FF7CondJumpInstruction, 0, "BB");
            OPCODE(eOpcodes::IFMEMBQ, "IFMEMBQ", FF7CondJumpInstruction, 0, "BB");

            // Module
            OPCODE(eOpcodes::DSKCG, "DSKCG", FF7ModuleInstruction, 0, "B");
            START_SUBOPCODE(eOpcodes::SPECIAL)
                OPCODE(eSpecialOpcodes::ARROW, "ARROW", FF7ModuleInstruction, 0, "B");
            OPCODE(eSpecialOpcodes::PNAME, "PNAME", FF7ModuleInstruction, 0, "B");
            OPCODE(eSpecialOpcodes::GMSPD, "GMSPD", FF7ModuleInstruction, 0, "B");
            OPCODE(eSpecialOpcodes::SMSPD, "SMSPD", FF7ModuleInstruction, 0, "BB");
            OPCODE(eSpecialOpcodes::FLMAT, "FLMAT", FF7ModuleInstruction, 0, "");
            OPCODE(eSpecialOpcodes::FLITM, "FLITM", FF7ModuleInstruction, 0, "");
            OPCODE(eSpecialOpcodes::BTLCK, "BTLCK", FF7ModuleInstruction, 0, "B");
            OPCODE(eSpecialOpcodes::MVLCK, "MVLCK", FF7ModuleInstruction, 0, "B");
            OPCODE(eSpecialOpcodes::SPCNM, "SPCNM", FF7ModuleInstruction, 0, "BB");
            OPCODE(eSpecialOpcodes::RSGLB, "RSGLB", FF7ModuleInstruction, 0, "");
            OPCODE(eSpecialOpcodes::CLITM, "CLITM", FF7ModuleInstruction, 0, "");
            END_SUBOPCODE
                OPCODE(eOpcodes::MINIGAME, "MINIGAME", FF7ModuleInstruction, 0, "wsswBB");
            OPCODE(eOpcodes::BTMD2, "BTMD2", FF7ModuleInstruction, 0, "d");
            OPCODE(eOpcodes::BTRLD, "BTRLD", FF7ModuleInstruction, 0, "NB");
            OPCODE(eOpcodes::BTLTB, "BTLTB", FF7ModuleInstruction, 0, "B");
            OPCODE(eOpcodes::MAPJUMP, "MAPJUMP", FF7ModuleInstruction, 0, "wsswB");
            OPCODE(eOpcodes::LSTMP, "LSTMP", FF7ModuleInstruction, 0, "NB");
            OPCODE(eOpcodes::BATTLE, "BATTLE", FF7ModuleInstruction, 0, "Nw");
            OPCODE(eOpcodes::BTLON, "BTLON", FF7ModuleInstruction, 0, "B");
            OPCODE(eOpcodes::BTLMD, "BTLMD", FF7ModuleInstruction, 0, "w");
            OPCODE(eOpcodes::MPJPO, "MPJPO", FF7ModuleInstruction, 0, "B");
            OPCODE(eOpcodes::PMJMP, "PMJMP", FF7ModuleInstruction, 0, "w");
            OPCODE(eOpcodes::PMJMP2, "PMJMP2", FF7ModuleInstruction, 0, "");
            OPCODE(eOpcodes::GAMEOVER, "GAMEOVER", FF7ModuleInstruction, 0, "");

            // Math
            OPCODE(eOpcodes::PLUS_, "PLUS!", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::PLUS2_, "PLUS2!", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::MINUS_, "MINUS!", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::MINUS2_, "MINUS2!", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::INC_, "INC!", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::INC2_, "INC2!", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC_, "DEC!", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC2_, "DEC2!", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::RDMSD, "RDMSD", FF7MathInstruction, 0, "NB");
            OPCODE(eOpcodes::SETBYTE, "SETBYTE", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::SETWORD, "SETWORD", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::BITON, "BITON", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::BITOFF, "BITOFF", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::BITXOR, "BITXOR", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::PLUS, "PLUS", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::PLUS2, "PLUS2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::MINUS, "MINUS", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::MINUS2, "MINUS2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::MUL, "MUL", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::MUL2, "MUL2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::DIV, "DIV", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::DIV2, "DIV2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::MOD, "MOD", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::MOD2, "MOD2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::AND, "AND", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::AND2, "AND2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::OR, "OR", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::OR2, "OR2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::XOR, "XOR", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::XOR2, "XOR2", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::INC, "INC", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::INC2, "INC2", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC, "DEC", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC2, "DEC2", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::RANDOM, "RANDOM", FF7MathInstruction, 0, "BB");
            OPCODE(eOpcodes::LBYTE, "LBYTE", FF7MathInstruction, 0, "NBB");
            OPCODE(eOpcodes::HBYTE, "HBYTE", FF7MathInstruction, 0, "NBw");
            OPCODE(eOpcodes::TWOBYTE, "2BYTE", FF7MathInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::SIN, "SIN", FF7MathInstruction, 0, "NNwwwB");
            OPCODE(eOpcodes::COS, "COS", FF7MathInstruction, 0, "NNwwwB");

            // Window
            OPCODE(eOpcodes::TUTOR, "TUTOR", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::WCLS, "WCLS", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::WSIZW, "WSIZW", FF7WindowInstruction, 0, "Bwwww");
            OPCODE(eOpcodes::WSPCL, "WSPCL", FF7WindowInstruction, 0, "BBBB");
            OPCODE(eOpcodes::WNUMB, "WNUMB", FF7WindowInstruction, 0, "NBwwB"); // NBdB when N == 0
            OPCODE(eOpcodes::STTIM, "STTIM", FF7WindowInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::MESSAGE, "MESSAGE", FF7WindowInstruction, 0, "BB");
            OPCODE(eOpcodes::MPARA, "MPARA", FF7WindowInstruction, 0, "NBBB");
            OPCODE(eOpcodes::MPRA2, "MPRA2", FF7WindowInstruction, 0, "NBBw");
            OPCODE(eOpcodes::MPNAM, "MPNAM", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::ASK, "ASK", FF7WindowInstruction, 0, "NBBBBB");
            OPCODE(eOpcodes::MENU, "MENU", FF7WindowInstruction, 0, "NBB");
            OPCODE(eOpcodes::MENU2, "MENU2", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::WINDOW, "WINDOW", FF7WindowInstruction, 0, "Bwwww");
            OPCODE(eOpcodes::WMOVE, "WMOVE", FF7WindowInstruction, 0, "Bss");
            OPCODE(eOpcodes::WMODE, "WMODE", FF7WindowInstruction, 0, "BBB");
            OPCODE(eOpcodes::WREST, "WREST", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::WCLSE, "WCLSE", FF7WindowInstruction, 0, "B");
            OPCODE(eOpcodes::WROW, "WROW", FF7WindowInstruction, 0, "BB");
            OPCODE(eOpcodes::GWCOL, "GWCOL", FF7WindowInstruction, 0, "NNBBBB");
            OPCODE(eOpcodes::SWCOL, "SWCOL", FF7WindowInstruction, 0, "NNBBBB");

            // Party
            OPCODE(eOpcodes::SPTYE, "SPTYE", FF7PartyInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::GTPYE, "GTPYE", FF7PartyInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::GOLDU, "GOLDU", FF7PartyInstruction, 0, "Nww"); // Nd when N == 0
            OPCODE(eOpcodes::GOLDD, "GOLDD", FF7PartyInstruction, 0, "Nww"); // Nd when N == 0
            OPCODE(eOpcodes::CHGLD, "CHGLD", FF7PartyInstruction, 0, "NBB");
            OPCODE(eOpcodes::HMPMAX1, "HMPMAX1", FF7PartyInstruction, 0, "");
            OPCODE(eOpcodes::HMPMAX2, "HMPMAX2", FF7PartyInstruction, 0, "");
            OPCODE(eOpcodes::MHMMX, "MHMMX", FF7PartyInstruction, 0, "");
            OPCODE(eOpcodes::HMPMAX3, "HMPMAX3", FF7PartyInstruction, 0, "");
            OPCODE(eOpcodes::MPU, "MPU", FF7PartyInstruction, 0, "NBw");
            OPCODE(eOpcodes::MPD, "MPD", FF7PartyInstruction, 0, "NBw");
            OPCODE(eOpcodes::HPU, "HPU", FF7PartyInstruction, 0, "NBw");
            OPCODE(eOpcodes::HPD, "HPD", FF7PartyInstruction, 0, "NBw");
            OPCODE(eOpcodes::STITM, "STITM", FF7PartyInstruction, 0, "NwB");
            OPCODE(eOpcodes::DLITM, "DLITM", FF7PartyInstruction, 0, "NwB");
            OPCODE(eOpcodes::CKITM, "CKITM", FF7PartyInstruction, 0, "NwB");
            OPCODE(eOpcodes::SMTRA, "SMTRA", FF7PartyInstruction, 0, "NNBBBB");
            OPCODE(eOpcodes::DMTRA, "DMTRA", FF7PartyInstruction, 0, "NNBBBBB");
            OPCODE(eOpcodes::CMTRA, "CMTRA", FF7PartyInstruction, 0, "NNNBBBBBB");
            OPCODE(eOpcodes::GETPC, "GETPC", FF7PartyInstruction, 0, "NBB");
            OPCODE(eOpcodes::PRTYP, "PRTYP", FF7PartyInstruction, 0, "B");
            OPCODE(eOpcodes::PRTYM, "PRTYM", FF7PartyInstruction, 0, "B");
            OPCODE(eOpcodes::PRTYE, "PRTYE", FF7PartyInstruction, 0, "BBB");
            OPCODE(eOpcodes::MMBUD, "MMBUD", FF7PartyInstruction, 0, "BB");
            OPCODE(eOpcodes::MMBLK, "MMBLK", FF7PartyInstruction, 0, "B");
            OPCODE(eOpcodes::MMBUK, "MMBUK", FF7PartyInstruction, 0, "B");

            // Model
            OPCODE(eOpcodes::JOIN, "JOIN", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::SPLIT, "SPLIT", FF7ModelInstruction, 0, "NNNssBssBB");
            OPCODE(eOpcodes::BLINK, "BLINK", FF7ModelInstruction, 0, "B");
            OPCODE_BASE(eOpcodes::KAWAI)
            {
                int length = this->mStream->ReadU8();
                assert(length >= 3);
                std::ostringstream paramStream;
                for (int i = 3; i < length; ++i)
                {
                    paramStream << "B";
                }
                auto parameters = paramStream.str();

                opcode = this->mStream->ReadU8();
                switch (opcode)
                {
                    OPCODE(eKawaiOpcodes::EYETX, "EYETX", FF7ModelInstruction, 0, parameters.c_str()); // was BBBB
                    OPCODE(eKawaiOpcodes::TRNSP, "TRNSP", FF7ModelInstruction, 0, parameters.c_str()); // was B
                    OPCODE(eKawaiOpcodes::AMBNT, "AMBNT", FF7ModelInstruction, 0, parameters.c_str()); // was BBBBBBB
                    OPCODE(eKawaiOpcodes::Unknown03, "Unknown03", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown04, "Unknown04", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown05, "Unknown05", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::LIGHT, "LIGHT", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown07, "Unknown07", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown08, "Unknown08", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown09, "Unknown09", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::SBOBJ, "SBOBJ", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown0B, "Unknown0B", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::Unknown0C, "Unknown0C", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::SHINE, "SHINE", FF7ModelInstruction, 0, parameters.c_str());
                    OPCODE(eKawaiOpcodes::RESET, "RESET", FF7ModelInstruction, 0, parameters.c_str());
                default:
                    throw UnknownSubOpcodeException(this->_address, opcode);
                }
                INC_ADDR;
                INC_ADDR;
            }
            OPCODE_END
                OPCODE(eOpcodes::KAWIW, "KAWIW", FF7ModelInstruction, 0, "");
            OPCODE(eOpcodes::PMOVA, "PMOVA", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::PDIRA, "PDIRA", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::PTURA, "PTURA", FF7ModelInstruction, 0, "BBB");
            OPCODE(eOpcodes::PGTDR, "PGTDR", FF7ModelInstruction, 0, "NBB");
            OPCODE(eOpcodes::PXYZI, "PXYZI", FF7ModelInstruction, 0, "NNBBBBB");
            OPCODE(eOpcodes::TLKON, "TLKON", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::PC, "PC", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::opCodeCHAR, "CHAR", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::DFANM, "DFANM", FF7ModelInstruction, 0, "BB");
            OPCODE(eOpcodes::ANIME1, "ANIME1", FF7ModelInstruction, 0, "BB");
            OPCODE(eOpcodes::VISI, "VISI", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::XYZI, "XYZI", FF7ModelInstruction, 0, "NNsssw");
            OPCODE(eOpcodes::XYI, "XYI", FF7ModelInstruction, 0, "NNssw");
            OPCODE(eOpcodes::XYZ, "XYZ", FF7ModelInstruction, 0, "NNsss");
            OPCODE(eOpcodes::MOVE, "MOVE", FF7ModelInstruction, 0, "Nss");
            OPCODE(eOpcodes::CMOVE, "CMOVE", FF7ModelInstruction, 0, "Nss");
            OPCODE(eOpcodes::MOVA, "MOVA", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::TURA, "TURA", FF7ModelInstruction, 0, "BBB");
            OPCODE(eOpcodes::ANIMW, "ANIMW", FF7ModelInstruction, 0, "");
            OPCODE(eOpcodes::FMOVE, "FMOVE", FF7ModelInstruction, 0, "Nss");
            OPCODE(eOpcodes::ANIME2, "ANIME2", FF7ModelInstruction, 0, "BB");
            OPCODE(eOpcodes::ANIM_1, "ANIM!1", FF7ModelInstruction, 0, "BB");
            OPCODE(eOpcodes::CANIM1, "CANIM1", FF7ModelInstruction, 0, "BBBB");
            OPCODE(eOpcodes::CANM_1, "CANM!1", FF7ModelInstruction, 0, "BBBB");
            OPCODE(eOpcodes::MSPED, "MSPED", FF7ModelInstruction, 0, "Nw");
            OPCODE(eOpcodes::DIR, "DIR", FF7ModelInstruction, 0, "NB");
            OPCODE(eOpcodes::TURNGEN, "TURNGEN", FF7ModelInstruction, 0, "NBBBB");
            OPCODE(eOpcodes::TURN, "TURN", FF7ModelInstruction, 0, "NBBBB");
            OPCODE(eOpcodes::DIRA, "DIRA", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::GETDIR, "GETDIR", FF7ModelInstruction, 0, "NBB");
            OPCODE(eOpcodes::GETAXY, "GETAXY", FF7ModelInstruction, 0, "NBBB");
            OPCODE(eOpcodes::GETAI, "GETAI", FF7ModelInstruction, 0, "NBB");
            OPCODE(eOpcodes::ANIM_2, "ANIM!2", FF7ModelInstruction, 0, "BB");
            OPCODE(eOpcodes::CANIM2, "CANIM2", FF7ModelInstruction, 0, "BBBB");
            OPCODE(eOpcodes::CANM_2, "CANM!2", FF7ModelInstruction, 0, "BBBB");
            OPCODE(eOpcodes::ASPED, "ASPED", FF7ModelInstruction, 0, "Nw");
            OPCODE(eOpcodes::CC, "CC", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::JUMP, "JUMP", FF7ModelInstruction, 0, "NNssww");
            OPCODE(eOpcodes::AXYZI, "AXYZI", FF7ModelInstruction, 0, "NNBBBBB");
            OPCODE(eOpcodes::LADER, "LADER", FF7ModelInstruction, 0, "NNssswBBBB");
            OPCODE(eOpcodes::OFST, "OFST", FF7ModelInstruction, 0, "NNBsssw");
            OPCODE(eOpcodes::OFSTW, "OFSTW", FF7ModelInstruction, 0, "");
            OPCODE(eOpcodes::TALKR, "TALKR", FF7ModelInstruction, 0, "NB");
            OPCODE(eOpcodes::SLIDR, "SLIDR", FF7ModelInstruction, 0, "NB");
            OPCODE(eOpcodes::SOLID, "SOLID", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::TLKR2, "TLKR2", FF7ModelInstruction, 0, "Nw");
            OPCODE(eOpcodes::SLDR2, "SLDR2", FF7ModelInstruction, 0, "Nw");
            OPCODE(eOpcodes::CCANM, "CCANM", FF7ModelInstruction, 0, "BBB");
            OPCODE(eOpcodes::FCFIX, "FCFIX", FF7ModelInstruction, 0, "B");
            OPCODE(eOpcodes::ANIMB, "ANIMB", FF7ModelInstruction, 0, "");
            OPCODE(eOpcodes::TURNW, "TURNW", FF7ModelInstruction, 0, "");

            // Walkmesh
            OPCODE(eOpcodes::SLIP, "SLIP", FF7WalkmeshInstruction, 0, "B");
            OPCODE(eOpcodes::UC, "UC", FF7WalkmeshInstruction, 0, "B");
            OPCODE(eOpcodes::IDLCK, "IDLCK", FF7WalkmeshInstruction, 0, "wB");
            OPCODE(eOpcodes::LINE, "LINE", FF7WalkmeshInstruction, 0, "ssssss");
            OPCODE(eOpcodes::LINON, "LINON", FF7WalkmeshInstruction, 0, "B");
            OPCODE(eOpcodes::SLINE, "SLINE", FF7WalkmeshInstruction, 0, "NNNssssss");

            // Backgnd
            OPCODE(eOpcodes::BGPDH, "BGPDH", FF7BackgroundInstruction, 0, "NBs");
            OPCODE(eOpcodes::BGSCR, "BGSCR", FF7BackgroundInstruction, 0, "NBss");
            OPCODE(eOpcodes::MPPAL, "MPPAL", FF7BackgroundInstruction, 0, "NNNBBBBBBB");
            OPCODE(eOpcodes::BGON, "BGON", FF7BackgroundInstruction, 0, "NBB");
            OPCODE(eOpcodes::BGOFF, "BGOFF", FF7BackgroundInstruction, 0, "NBB");
            OPCODE(eOpcodes::BGROL, "BGROL", FF7BackgroundInstruction, 0, "NB");
            OPCODE(eOpcodes::BGROL2, "BGROL2", FF7BackgroundInstruction, 0, "NB");
            OPCODE(eOpcodes::BGCLR, "BGCLR", FF7BackgroundInstruction, 0, "NB");
            OPCODE(eOpcodes::STPAL, "STPAL", FF7BackgroundInstruction, 0, "NBBB");
            OPCODE(eOpcodes::LDPAL, "LDPAL", FF7BackgroundInstruction, 0, "NBBB");
            OPCODE(eOpcodes::CPPAL, "CPPAL", FF7BackgroundInstruction, 0, "NBBB");
            OPCODE(eOpcodes::RTPAL, "RTPAL", FF7BackgroundInstruction, 0, "NNBBBB");
            OPCODE(eOpcodes::ADPAL, "ADPAL", FF7BackgroundInstruction, 0, "NNNBBBBBB");
            OPCODE(eOpcodes::MPPAL2, "MPPAL2", FF7BackgroundInstruction, 0, "NNNBBBBBB");
            OPCODE(eOpcodes::STPLS, "STPLS", FF7BackgroundInstruction, 0, "BBBB");
            OPCODE(eOpcodes::LDPLS, "LDPLS", FF7BackgroundInstruction, 0, "BBBB");
            OPCODE(eOpcodes::CPPAL2, "CPPAL2", FF7BackgroundInstruction, 0, "BBBBBBB");
            OPCODE(eOpcodes::RTPAL2, "RTPAL2", FF7BackgroundInstruction, 0, "BBBBBBB");
            OPCODE(eOpcodes::ADPAL2, "ADPAL2", FF7BackgroundInstruction, 0, "BBBBBBBBBB");

            // Camera
            OPCODE(eOpcodes::NFADE, "NFADE", FF7CameraInstruction, 0, "NNBBBBBB");
            OPCODE(eOpcodes::SHAKE, "SHAKE", FF7CameraInstruction, 0, "BBBBBBB");
            OPCODE(eOpcodes::SCRLO, "SCRLO", FF7CameraInstruction, 0, "B");
            OPCODE(eOpcodes::SCRLC, "SCRLC", FF7CameraInstruction, 0, "BBBB");
            OPCODE(eOpcodes::SCRLA, "SCRLA", FF7CameraInstruction, 0, "NwBB");
            OPCODE(eOpcodes::SCR2D, "SCR2D", FF7CameraInstruction, 0, "Nss");
            OPCODE(eOpcodes::SCRCC, "SCRCC", FF7CameraInstruction, 0, "");
            OPCODE(eOpcodes::SCR2DC, "SCR2DC", FF7CameraInstruction, 0, "NNssw");
            OPCODE(eOpcodes::SCRLW, "SCRLW", FF7CameraInstruction, 0, "");
            OPCODE(eOpcodes::SCR2DL, "SCR2DL", FF7CameraInstruction, 0, "NNssw");
            OPCODE(eOpcodes::VWOFT, "VWOFT", FF7CameraInstruction, 0, "NssB");
            OPCODE(eOpcodes::FADE, "FADE", FF7CameraInstruction, 0, "NNBBBBBB");
            OPCODE(eOpcodes::FADEW, "FADEW", FF7CameraInstruction, 0, "");
            OPCODE(eOpcodes::SCRLP, "SCRLP", FF7CameraInstruction, 0, "NwBB");
            OPCODE(eOpcodes::MVCAM, "MVCAM", FF7CameraInstruction, 0, "B");

            // AV
            OPCODE(eOpcodes::BGMOVIE, "BGMOVIE", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::AKAO2, "AKAO2", FF7AudioVideoInstruction, 0, "NNNBwwwww");
            OPCODE(eOpcodes::MUSIC, "MUSIC", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::SOUND, "SOUND", FF7AudioVideoInstruction, 0, "NwB");
            OPCODE(eOpcodes::AKAO, "AKAO", FF7AudioVideoInstruction, 0, "NNNBBwwww");
            OPCODE(eOpcodes::MUSVT, "MUSVT", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::MUSVM, "MUSVM", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::MULCK, "MULCK", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::BMUSC, "BMUSC", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::CHMPH, "CHMPH", FF7AudioVideoInstruction, 0, "BBB");
            OPCODE(eOpcodes::PMVIE, "PMVIE", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::MOVIE, "MOVIE", FF7AudioVideoInstruction, 0, "");
            OPCODE(eOpcodes::MVIEF, "MVIEF", FF7AudioVideoInstruction, 0, "NB");
            OPCODE(eOpcodes::FMUSC, "FMUSC", FF7AudioVideoInstruction, 0, "B");
            OPCODE(eOpcodes::CMUSC, "CMUSC", FF7AudioVideoInstruction, 0, "BBBBBBB");
            OPCODE(eOpcodes::CHMST, "CHMST", FF7AudioVideoInstruction, 0, "NB");

            // Uncat
            OPCODE(eOpcodes::MPDSP, "MPDSP", FF7UncategorizedInstruction, 0, "B");
            OPCODE(eOpcodes::SETX, "SETX", FF7UncategorizedInstruction, 0, "BBBBBB");
            OPCODE(eOpcodes::GETX, "GETX", FF7UncategorizedInstruction, 0, "BBBBBB");
            OPCODE(eOpcodes::SEARCHX, "SEARCHX", FF7UncategorizedInstruction, 0, "BBBBBBBBBB");

        default:
            throw UnknownOpcodeException(this->_address, opcode);
        }
        INC_ADDR;
        if (full_opcode == eOpcodes::RET)
        {
            return;
        }
    }
}