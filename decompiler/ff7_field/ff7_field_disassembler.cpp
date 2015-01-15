#include "ff7_field_disassembler.h"
#include "ff7_field_engine.h"
#include "decompiler_engine.h"
#include <boost/format.hpp>
#include "lzs.h"
#include "make_unique.h"

FF7::FF7Disassembler::FF7Disassembler(FF7Engine *engine, InstVec &insts)
    : SimpleDisassembler(insts), mEngine(engine)
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

std::unique_ptr<Function> FF7::FF7Disassembler::StartFunction(size_t entityNumber, size_t scriptIndex)
{
    auto func = std::make_unique<Function>();
    func->_retVal = false;
    func->_args = 0;
    func->_name = "script_" + std::string(mHeader.mFieldEntityNames[entityNumber].data()) + "_" + std::to_string(entityNumber) + "_" + std::to_string(scriptIndex);
    func->_startIt = _insts.begin(); // TODO: Will become invalid, and this is wrong
    return func;
}

void FF7::FF7Disassembler::doDisassemble() throw(std::exception)
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

    // Read the script header
    mHeader.Read(*mStream);

    // Loop through the scripts for each entity
    for (size_t entityNumber = 0; entityNumber < mHeader.mEntityScripts.size(); entityNumber++)
    {
        std::set<uint16> parsedScripts;
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
                scriptEntryPoint += kSectionPointersSize;
                mStream->Seek(scriptEntryPoint);
         
                this->_addressBase = mStream->Position();
                this->_address = this->_addressBase;
               
                if (scriptIndex > 0)
                {
                    // Read each block of opcodes up to a return
                    auto func = StartFunction(entityNumber, scriptIndex);
                    while (mStream->Position() != nextScriptEntryPoint + kSectionPointersSize)
                    {
                        // Keep going till we have all of the script
                        // TODO: Wrong as should only be a single return in these scripts?
                        ReadOpCodes(nextScriptEntryPoint + kSectionPointersSize);
                    }
                    func->_endIt = _insts.end() - 1;
                    mEngine->_functions[scriptEntryPoint] = *func;
                }
                else
                {
                    // Read the init script
                    const size_t endPos = nextScriptEntryPoint + kSectionPointersSize;
                    auto initFunc = StartFunction(entityNumber, scriptIndex);
                    initFunc->_name += "_init";
                    ReadOpCodes(endPos);
                    initFunc->_endIt = _insts.end() - 1;
                    mEngine->_functions[scriptEntryPoint] = *initFunc;

                    size_t streamPos = mStream->Position();
                    if (streamPos != endPos)
                    {
                        // Main entry point is the current pos
                        scriptEntryPoint = static_cast<uint16>(streamPos);
                        auto mainFunc = StartFunction(entityNumber, scriptIndex);
                        mainFunc->_name += "_main";

                        // Read the main script
                        ReadOpCodes(endPos);
                        mainFunc->_endIt = _insts.end() - 1;
                        mEngine->_functions[scriptEntryPoint] = *mainFunc;

                        streamPos = mStream->Position();
                        if (streamPos != endPos)
                        {
                            // We only ever expect 2 return statements in script 0
                            // the first gives us the "init" script, the second gives
                            // us the "main" script, anymore is an error
                            abort();
                        }
                    }
                }
            }
        }
    } 
}

void FF7::FF7Disassembler::ReadOpCodes(size_t endPos)
{
    while (mStream->Position() < endPos)
    {
        uint8 opcode = mStream->ReadU8();
        uint32 full_opcode = 0;
        std::string opcodePrefix;
        switch (opcode)
        {
            OPCODE(eOpcodes::IFUB, "IFUB", FF7CondJumpInstruction, 0, "NBBBB");
            OPCODE(eOpcodes::IFUBL, "IFUBL", FF7CondJumpInstruction, 0, "NBBBw");
            OPCODE(eOpcodes::IFSW, "IFSW", FF7CondJumpInstruction, 0, "NwsBB");
            OPCODE(eOpcodes::IFSWL, "IFSWL", FF7CondJumpInstruction, 0, "NwsBw");
            OPCODE(eOpcodes::IFUW, "IFUW", FF7CondJumpInstruction, 0, "NwwBB");
            OPCODE(eOpcodes::IFUWL, "IFUWL", FF7CondJumpInstruction, 0, "NwwBw");


            OPCODE(eOpcodes::JMPF, "JMPF", FF7UncondJumpInstruction, 0, "B");
            OPCODE(eOpcodes::JMPB, "JMPB", FF7UncondJumpInstruction, 0, "B");
           
            OPCODE(eOpcodes::RET, "RET", FF7KernelCallInstruction, 0, "");
            /*
            OPCODE_BASE(eOpcodes::KAWAI)
                opcode = this->mStream->ReadU8(); // Annoyingly has the length here
                opcode = this->mStream->ReadU8();
                switch (opcode) {
                OPCODE(0x6e, "6eUnknown", FF7KernelCallInstruction, 0, "");
            END_SUBOPCODE
            */
            START_SUBOPCODE(eOpcodes::SPECIAL)
                OPCODE(0xF5, "ARROW", FF7KernelCallInstruction, 0, "B");
                //OPCODE(0x24, "24Unknown", FF7KernelCallInstruction, 0, "");
            END_SUBOPCODE
            OPCODE(eOpcodes::OFST, "OFST", FF7KernelCallInstruction, 0, "BBBBsssw");
            OPCODE(eOpcodes::MUL, "MUL", FF7KernelCallInstruction, 0, "BBB");
            OPCODE(0xAF, "ANIM!1", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xDE, "TURNW", FF7KernelCallInstruction, 0, "");
            OPCODE(0xBA, "ANIM!2", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0xAE, "ANIME2", FF7KernelCallInstruction, 0, "BB");
            OPCODE(eOpcodes::BGSCR, "BGSCR", FF7KernelCallInstruction, 0, "BBss");
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
            OPCODE(0x33, "UC", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x2C, "BGPDH", FF7KernelCallInstruction, 0, "BBw");
            OPCODE(0xE4, "BGCLR", FF7KernelCallInstruction, 0, "BB");
            OPCODE(eOpcodes::BGON, "BGON", FF7KernelCallInstruction, 0, "NBB");
            OPCODE(0xE1, "BGOFF", FF7KernelCallInstruction, 0, "NBB");
            OPCODE(0xBB, "CANIM2", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0xAD, "CMOVE", FF7KernelCallInstruction, 0, "Bss");
            OPCODE(0x25, "NFADE", FF7KernelCallInstruction, 0, "BBBBBBBB");
            OPCODE(0x2E, "WCLS", FF7KernelCallInstruction, 0, "B");
            OPCODE(0xEC, "LDPLS", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(eOpcodes::RANDOM, "RANDOM", FF7StoreInstruction, 0, "BB");
            OPCODE(eOpcodes::MOD, "MOD", FF7StoreInstruction, 0, "NBB");
            OPCODE(0x08, "JOIN", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x6C, "FADEW", FF7KernelCallInstruction, 0, "");
            OPCODE(0x0b, "SPTYE", FF7KernelCallInstruction, 0, "BBBBB");
            OPCODE(0x0a, "GTPYE", FF7KernelCallInstruction, 0, "BBBBB");
            OPCODE(eOpcodes::SETWORD, "SETWORD", FF7StoreInstruction, 0, "NBs");
            OPCODE(0x02, "REQSW", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x03, "REQEW", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x04, "PREQ", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x05, "PRQSW", FF7KernelCallInstruction, 0, "BB");
            OPCODE(0x42, "MPRA2", FF7KernelCallInstruction, 0, "BBBw");
            OPCODE(0xef, "ADPAL2", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0xe9, "ADPAL", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0xEA, "MPPAL2", FF7KernelCallInstruction, 0, "BBBBBBBBB");
            OPCODE(0x6b, "FADE", FF7KernelCallInstruction, 0, "BBBBBBBB");
            OPCODE(0x09, "SPLIT", FF7KernelCallInstruction, 0, "BBBssBssBB");
            OPCODE(eOpcodes::INC, "INC", FF7StoreInstruction, 0, "BB");
            OPCODE(eOpcodes::DEC, "DEC", FF7StoreInstruction, 0, "BB");
            OPCODE(0xeb, "STPLS", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0x35, "PTURA", FF7KernelCallInstruction, 0, "BBB");
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
            OPCODE(0x0e, "DSKCG", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x3E, "MHMMX", FF7KernelCallInstruction, 0, "");
            OPCODE(0xA4, "VISI", FF7KernelCallInstruction, 0, "B");
            OPCODE(0x01, "REQ", ReturnInstruction, 0, "BB");
            OPCODE(0xE5, "STPAL", FF7KernelCallInstruction, 0, "BBBB");
            OPCODE(0x24, "WAIT", FF7KernelCallInstruction, 0, "w");
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
        if (opcode == 0x0)
        {
            return;
        }
    }
}