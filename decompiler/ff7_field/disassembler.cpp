#include "disassembler.h"
#include "engine.h"
#include <boost/format.hpp>
#include "lzs.h"

FF7::FF7Disassembler::FF7Disassembler(FF7Engine *engine, InstVec &insts)
    : SimpleDisassembler(insts)
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


uint32 FF7::FF7Disassembler::GetEndOfScriptOffset(size_t entityIndex, size_t scriptIndex)
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

    return mHeader.mEntityScripts[entityIndex][scriptIndex];

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

    int count = 0;

    // Loop through the scripts for each entity
    for (size_t entityNumber = 0; entityNumber < mHeader.mEntityScripts.size(); entityNumber++)
    {
        for (size_t scriptIndex = 0; scriptIndex < mHeader.mEntityScripts[entityNumber].size(); scriptIndex++)
        {
            uint16 scriptEntryPoint = mHeader.mEntityScripts[entityNumber][scriptIndex];
            const uint32 nextScriptEntryPoint = GetEndOfScriptOffset(entityNumber, scriptIndex);
            const uint32 scriptSize = nextScriptEntryPoint - scriptEntryPoint;
            if (scriptSize > 0)
            {
                count++;
                if (count <= 8)
                {
                  //  continue;
                }

       
                scriptEntryPoint += kSectionPointersSize;
                mStream->Seek(scriptEntryPoint);
                std::cout << std::endl << std::endl;
                std::cout << std::string(mHeader.mFieldEntityNames[entityNumber].data()).c_str() << std::endl;
                std::cout << "script_" << scriptIndex << " size is " << scriptSize << " entry " << scriptEntryPoint << std::endl;

                this->_addressBase = mStream->Position();
                this->_address = this->_addressBase;
                //for (;;)
                while (mStream->Position() < nextScriptEntryPoint + kSectionPointersSize)
                {
                    uint8 opcode = mStream->ReadU8();

                    uint32 full_opcode = 0;
                    std::string opcodePrefix;
                    switch (opcode)
                    {
                        OPCODE(0x10, "JMPF", FF7UncondJumpInstruction, 0, "B");
                        OPCODE(0x12, "JMPB", FF7UncondJumpInstruction, 0, "B");
                        OPCODE(0x16, "IFSW", FF7CondJumpInstruction, 0, "BwsBB");
                        OPCODE(0x14, "IFUB", FF7CondJumpInstruction, 0, "BBBBB");
                        
                        /*
                        OPCODE_BASE(0x28) // KAWAI
                        opcode = this->mStream->ReadU8(); // Annoyingly has the length here
                        opcode = this->mStream->ReadU8();
                            switch (opcode) {
                                OPCODE(0x6e, "6eUnknown", FF7KernelCallInstruction, 0, "");
                        END_SUBOPCODE

                           
                        START_SUBOPCODE(0x0f) // SPECIAL
                            OPCODE(0xF5, "ARROW", FF7KernelCallInstruction, 0, "B");
                            OPCODE(0x24, "24Unknown", FF7KernelCallInstruction, 0, "");
                        END_SUBOPCODE
                        */

                        OPCODE(0x2C, "BGPDH", FF7KernelCallInstruction, 0, "BBw");
                        OPCODE(0xE4, "BGCLR", FF7KernelCallInstruction, 0, "BB");
                        OPCODE(0xE0, "BGON", FF7KernelCallInstruction, 0, "BBB");
                        OPCODE(0xE1, "BGOFF", FF7KernelCallInstruction, 0, "BBB");
                        OPCODE(0xBB, "CANIM2", FF7KernelCallInstruction, 0, "BBBB");
                        OPCODE(0xAD, "CMOVE", FF7KernelCallInstruction, 0, "Bss");
                        OPCODE(0x25, "NFADE", FF7KernelCallInstruction, 0, "BBBBBBBB");

                        OPCODE(0x2E, "WCLS", FF7KernelCallInstruction, 0, "B");
                        OPCODE(0xEC, "LDPLS", FF7KernelCallInstruction, 0, "BBBB");
                        OPCODE(0x99, "RANDOM", FF7KernelCallInstruction, 0, "BB");
                        OPCODE(0x8D, "MOD", FF7KernelCallInstruction, 0, "BBB");
                        OPCODE(0x08, "JOIN", FF7KernelCallInstruction, 0, "B");
                        OPCODE(0x6C, "FADEW", FF7KernelCallInstruction, 0, "");
                        OPCODE(0x0b, "SPTYE", FF7KernelCallInstruction, 0, "BBBBB");
                        OPCODE(0x0a, "GTPYE", FF7KernelCallInstruction, 0, "BBBBB");

                        OPCODE(0x81, "SETWORD", FF7KernelCallInstruction, 0, "BBs");
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

                      //  OPCODE(0x2a, "PMOVA", FF7KernelCallInstruction, 0, "B");

                        OPCODE(0x95, "INC", FF7KernelCallInstruction, 0, "BB");
                        OPCODE(0x97, "DEC", FF7KernelCallInstruction, 0, "BB");
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
                        OPCODE(0x01, "REQ", FF7KernelCallInstruction, 0, "BB");
                        OPCODE(0x00, "RET", FF7KernelCallInstruction, 0, "");
                        OPCODE(0xE5, "STPAL", FF7KernelCallInstruction, 0, "BBBB");
                    
                        OPCODE(0x24, "WAIT", FF7KernelCallInstruction, 0, "w");
                        OPCODE(0xE7, "CPPAL", FF7KernelCallInstruction, 0, "BBBB");
                        OPCODE(0xE6, "LDPAL", FF7KernelCallInstruction, 0, "BBBB");
                        OPCODE(0x87, "MINUS", FF7StoreInstruction, 0, "BBB");
                        OPCODE(0x85, "PLUS", FF7StoreInstruction, 0, "BBB");
                        OPCODE(0x80, "SETBYTE", FF7StoreInstruction, 0, "BBB");
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
                   // if (opcode == 0x0) break;
                }
               // return;
            }

        }
    } 
}