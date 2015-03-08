#include "ff7_field_disassembler.h"
#include "ff7_field_engine.h"
#include "decompiler_engine.h"
#include <boost/format.hpp>
#include "lzs.h"
#include "make_unique.h"

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
            DisassembleIndivdualScript(entityName, it->mIndex, it->mEntryPoint, it->mNextEntryPoint, isStart, isEnd);
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

void FF7::FF7Disassembler::AddFunc(std::string entityName, size_t scriptIndex, uint32 nextScriptEntryPoint, const bool isStart, bool isEnd, bool toReturnOnly, std::string funcName)
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

}

void FF7::FF7Disassembler::DisassembleIndivdualScript(std::string entityName,
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
        AddFunc(entityName, scriptIndex, nextScriptEntryPoint, isStart, isEnd, false, "");
    }
    else
    {
        const size_t endPos = nextScriptEntryPoint + kSectionPointersSize;

        // Read the init script, which means stop at the first return
        AddFunc(entityName, scriptIndex, nextScriptEntryPoint, isStart, isEnd, true, "init");
        
        // Not at the end of this script? Then the remaining data is the "main" script
        auto streamPos = mStream->Position();
        if (streamPos != endPos)
        {
            // The "main" script we should also only have 1 return statement
            AddFunc(entityName, scriptIndex, nextScriptEntryPoint, false, isEnd, true, "main");
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

void FF7::FF7Disassembler::ReadOpCodesToPositionOrReturn(size_t endPos)
{
    while (mStream->Position() < endPos)
    {
        uint8 opcode = mStream->ReadU8();
        uint32 full_opcode = 0;
        std::string opcodePrefix;
        switch (opcode)
        {
            OPCODE(eOpcodes::RET, "RET", FF7KernelCallInstruction, 0, "");
            OPCODE(eOpcodes::REQ, "REQ", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::REQSW, "REQSW", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::REQEW, "REQEW", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::PREQ, "PREQ", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::PREQSW, "PREQSW", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::PREQEW, "PREQEW", FF7KernelCallInstruction, 0, "BU");
            OPCODE(eOpcodes::RETTO, "RETTO", FF7KernelCallInstruction, 0, "U");
            OPCODE(eOpcodes::JOIN, "JOIN", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::SPLIT, "SPLIT", FF7KernelCallInstruction, 0, "NNNssBssBB");
            OPCODE(eOpcodes::SPTYE, "SPTYE", FF7KernelCallInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::GTPYE, "GTPYE", FF7KernelCallInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::DSKCG, "DSKCG", FF7KernelCallInstruction, 0, "B");
            START_SUBOPCODE(eOpcodes::SPECIAL)
                OPCODE(eSpecialOpcodes::ARROW, "ARROW", FF7KernelCallInstruction, 0, "B");
                OPCODE(eSpecialOpcodes::PNAME, "PNAME", FF7KernelCallInstruction, 0, "B");
                OPCODE(eSpecialOpcodes::GMSPD, "GMSPD", FF7KernelCallInstruction, 0, "B");
                OPCODE(eSpecialOpcodes::SMSPD, "SMSPD", FF7KernelCallInstruction, 0, "BB");
                OPCODE(eSpecialOpcodes::FLMAT, "FLMAT", FF7KernelCallInstruction, 0, "");
                OPCODE(eSpecialOpcodes::FLITM, "FLITM", FF7KernelCallInstruction, 0, "");
                OPCODE(eSpecialOpcodes::BTLCK, "BTLCK", FF7KernelCallInstruction, 0, "B");
                OPCODE(eSpecialOpcodes::MVLCK, "MVLCK", FF7KernelCallInstruction, 0, "B");
                OPCODE(eSpecialOpcodes::SPCNM, "SPCNM", FF7KernelCallInstruction, 0, "BB");
                OPCODE(eSpecialOpcodes::RSGLB, "RSGLB", FF7KernelCallInstruction, 0, "");
                OPCODE(eSpecialOpcodes::CLITM, "CLITM", FF7KernelCallInstruction, 0, "");
            END_SUBOPCODE

            OPCODE(eOpcodes::IFPRTYQ, "IFPRTYQ", FF7CondJumpInstruction, 0, "BB");
            OPCODE(eOpcodes::IFMEMBQ, "IFMEMBQ", FF7CondJumpInstruction, 0, "BB");

            OPCODE(eOpcodes::JMPF, "JMPF", FF7UncondJumpInstruction, 0, "B");
            OPCODE(eOpcodes::JMPFL, "JMPFL", FF7UncondJumpInstruction, 0, "s");
            OPCODE(eOpcodes::JMPB, "JMPB", FF7UncondJumpInstruction, 0, "B");
            OPCODE(eOpcodes::JMPBL, "JMPBL", FF7UncondJumpInstruction, 0, "s");
            OPCODE(eOpcodes::IFUB, "IFUB", FF7CondJumpInstruction, 0, "NBBBB");
            OPCODE(eOpcodes::IFUBL, "IFUBL", FF7CondJumpInstruction, 0, "NBBBw");
            OPCODE(eOpcodes::IFSW, "IFSW", FF7CondJumpInstruction, 0, "NwsBB");
            OPCODE(eOpcodes::IFSWL, "IFSWL", FF7CondJumpInstruction, 0, "NwsBw");
            OPCODE(eOpcodes::IFUW, "IFUW", FF7CondJumpInstruction, 0, "NwwBB");
            OPCODE(eOpcodes::IFUWL, "IFUWL", FF7CondJumpInstruction, 0, "NwwBw");
            OPCODE(eOpcodes::MINIGAME, "MINIGAME", FF7KernelCallInstruction, 0, "wsssBB");
            OPCODE(eOpcodes::TUTOR, "TUTOR", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::BTMD2, "BTMD2", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(eOpcodes::BTRLD, "BTRLD", FF7KernelCallInstruction, 0, "BB");
            OPCODE(eOpcodes::WAIT, "WAIT", FF7KernelCallInstruction, 0, "w");
            OPCODE(eOpcodes::NFADE, "NFADE", FF7KernelCallInstruction, 0, "BBBBBBBB");
            OPCODE(eOpcodes::BLINK, "BLINK", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::BGMOVIE, "BGMOVIE", FF7KernelCallInstruction, 0, "B");

            /*
            OPCODE_BASE(eOpcodes::KAWAI)
            opcode = this->mStream->ReadU8(); // Annoyingly has the length here
            opcode = this->mStream->ReadU8();
            switch (opcode) {
            OPCODE(0x6e, "6eUnknown", FF7KernelCallInstruction, 0, "");
            END_SUBOPCODE
            */

            OPCODE(eOpcodes::KAWIW, "KAWIW", FF7KernelCallInstruction, 0, "");
            OPCODE(eOpcodes::PMOVA, "PMOVA", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::SLIP, "SLIP", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::BGPDH, "BGPDH", FF7KernelCallInstruction, 0, "NBw");
            OPCODE(eOpcodes::BGSCR, "BGSCR", FF7KernelCallInstruction, 0, "NBss");
            OPCODE(eOpcodes::WCLS, "WCLS", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::WSIZW, "WSIZW", FF7KernelCallInstruction, 0, "Bwwww");
            OPCODE(eOpcodes::IFKEY, "IFKEY", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::IFKEYON, "IFKEYON", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::IFKEYOFF, "IFKEYOFF", FF7CondJumpInstruction, 0, "wB");
            OPCODE(eOpcodes::UC, "UC", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::PDIRA, "PDIRA", FF7KernelCallInstruction, 0, "B");
            OPCODE(eOpcodes::PTURA, "PTURA", FF7KernelCallInstruction, 0, "BBB");
            OPCODE(eOpcodes::WSPCL, "WSPCL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(eOpcodes::WNUMB, "WNUMB", FF7KernelCallInstruction, 0, "NBwB");
            OPCODE(eOpcodes::STTIM, "STTIM", FF7KernelCallInstruction, 0, "NNBBB");
            OPCODE(eOpcodes::GOLDU, "GOLDU", FF7KernelCallInstruction, 0, "Nd");
            OPCODE(eOpcodes::GOLDD, "GOLDD", FF7KernelCallInstruction, 0, "Nd");
            OPCODE(eOpcodes::CHGLD, "CHGLD", FF7KernelCallInstruction, 0, "NBB");


            OPCODE(eOpcodes::NOP, "NOP", FF7NoOutputInstruction, 0, "");
            OPCODE(eOpcodes::OFST, "OFST", FF7KernelCallInstruction, 0, "BBBBsssw");
            OPCODE(eOpcodes::MUL, "MUL", FF7KernelCallInstruction, 0, "BBB");
            OPCODE(0xAF, "ANIM!1", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xDE, "TURNW", FF7KernelCallInstruction, 0, "");
            OPCODE(0xBA, "ANIM!2", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xAE, "ANIME2", FF7KernelCallInstruction, 0, "BB");
            OPCODE(eOpcodes::TURNGEN, "TURNGEN", FF7KernelCallInstruction, 0, "BBBBB");
            OPCODE(0xE8, "RTPAL", FF7KernelCallInstruction, 0, "BBBBBB"); // ?
            OPCODE(0xAB, "TURA", FF7KernelCallInstruction, 0, "Bs"); // one s shorter than the docs?
            OPCODE(0xAC, "ANIMW", FF7KernelCallInstruction, 0, "");
            OPCODE(0xB1, "CANM!1", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xBC, "CANM!2", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0x60, "MAPJUMP", FF7KernelCallInstruction, 0, "swwwB");
            OPCODE(0xF2, "AKAO", FF7KernelCallInstruction, 0, "BBBBBwwww");
            OPCODE(0x5E, "SHAKE", FF7KernelCallInstruction, 0, "BBBBBBB");
            OPCODE(0x40, "MESSAGE", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x50, "WINDOW", FF7KernelCallInstruction, 0, "Bwwww");
            OPCODE(0xf1, "SOUND", FF7KernelCallInstruction, 0, "BwB");
            OPCODE(0x4a, "MENU2", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xE4, "BGCLR", FF7KernelCallInstruction, 0, "BB");
            OPCODE(eOpcodes::BGON, "BGON", FF7KernelCallInstruction, 0, "NBB");
            OPCODE(0xE1, "BGOFF", FF7KernelCallInstruction, 0, "NBB");
            OPCODE(0xBB, "CANIM2", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xAD, "CMOVE", FF7KernelCallInstruction, 0, "Bss");
            OPCODE(0xEC, "LDPLS", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(eOpcodes::RANDOM, "RANDOM", FF7StoreInstruction, 0, "BB");
            OPCODE(eOpcodes::MOD, "MOD", FF7StoreInstruction, 0, "NBB");
            OPCODE(0x6C, "FADEW", FF7KernelCallInstruction, 0, "");
            OPCODE(eOpcodes::SETWORD, "SETWORD", FF7StoreInstruction, 0, "NBs");
            OPCODE(0x42, "MPRA2", FF7KernelCallInstruction, 0, "BBBw");
            OPCODE(0xef, "ADPAL2", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0xe9, "ADPAL", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0xEA, "MPPAL2", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0x6b, "FADE", FF7KernelCallInstruction, 0, "BBBBBBBB");
            OPCODE(eOpcodes::INC, "INC", FF7StoreInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC, "DEC", FF7StoreInstruction, 0, "BB");
            OPCODE(0xeb, "STPLS", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0x7D, "DEC2!", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xD8, "PMJMP", FF7KernelCallInstruction, 0, "w");
            OPCODE(0xFF, "GAMEOVER", FF7KernelCallInstruction, 0, "");
            OPCODE(0xB3, "DIR", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xB2, "MSPED", FF7KernelCallInstruction, 0, "Bw");
            OPCODE(0xA2, "DFANM", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x7E, "TLKON", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xA8, "MOVE", FF7KernelCallInstruction, 0, "Bss");
            OPCODE(0xA3, "ANIME1", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x79, "MINUS2!", FF7KernelCallInstruction, 0, "BBw");
            OPCODE(0x3E, "MHMMX", FF7KernelCallInstruction, 0, "");
            OPCODE(0xA4, "VISI", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xE5, "STPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xE7, "CPPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xE6, "LDPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(eOpcodes::MINUS, "MINUS", FF7StoreInstruction, 0, "NBB");
            OPCODE(eOpcodes::PLUS, "PLUS", FF7StoreInstruction, 0, "NBB");
            OPCODE(0x80, "SETBYTE", FF7StoreInstruction, 0, "NBB");
            OPCODE(0x72, "BTLMD", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xF0, "MUSIC", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x43, "MPNAM", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xB9, "GETAI", FF7KernelCallInstruction, 0, "BBB");
            OPCODE(0xA1, "CHAR", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xA0, "PC", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x5A, "CKITM", FF7KernelCallInstruction, 0, "BwB");
            OPCODE(0xA5, "XYZI", FF7KernelCallInstruction, 0, "BBsssw");
            OPCODE(0xC7, "SOLID", FF7KernelCallInstruction, 0, "B");
        default:
            throw UnknownOpcodeException(this->_address, opcode);
        }
        INC_ADDR;
        if (opcode == eOpcodes::RET)
        {
            return;
        }
    }
}