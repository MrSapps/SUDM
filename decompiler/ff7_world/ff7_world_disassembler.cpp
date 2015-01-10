#include "ff7_world_disassembler.h"
#include "ff7_world_engine.h"
#include "decompiler_engine.h"
#include <boost/format.hpp>

FF7::FF7WorldDisassembler::FF7WorldDisassembler(FF7WorldEngine *engine, InstVec &insts, int scriptNumber)
    : SimpleDisassembler(insts), mScriptNumber(scriptNumber)
{

}

FF7::FF7WorldDisassembler::~FF7WorldDisassembler()
{

}

/*
00000202: reset stack (0)
00000203: push byte from bank 0 897 (2)
00000205: jump if false 25 (0)

00000207: reset stack (0)
00000208: push byte from bank 0 897 (2)
0000020a: push byte from bank 0 897 (2)
0000020c: push constant 1 (2)
0000020e: push sub (2)
0000020f: write bank (0)

00000210: reset stack (0)
00000211: push byte from bank 0 897 (2)
00000213: push constant 0 (2)
00000215: push equal (2)
00000216: jump if false 25 (0)

00000218: unknown 320 (0)
00000219: return (0)

if(!Read(897))
{
    Write(897-1)
    if (!(Read(897) == 0))
    {
        unknown320()
    }
}

*/
void FF7::FF7WorldDisassembler::doDisassemble() throw(std::exception)
{
    struct wmScriptData
    {
        uint16 mType = 0;
        uint16 mInstructionPointer = 0;
    };

    std::vector<wmScriptData> mData;
    mData.reserve(256);

    for (int i = 0; i < 256; i++)
    {
        wmScriptData tmp;
        tmp.mType = this->mStream->ReadU16();
        tmp.mInstructionPointer = this->mStream->ReadU16();
        mData.emplace_back(tmp);
    }


    int i = this->mScriptNumber;
  //  for (int i = 1; i < 256; i++)
    {
        wmScriptData data = mData[i];

        this->mStream->Seek((256 * sizeof(int)) + (data.mInstructionPointer*sizeof(uint16)));
        this->_address = this->mStream->Position();

        bool end = false;
        //    while (this->_f.pos() != (int)this->_f.size())
        for (;;)
        {
            uint32 full_opcode = 0;
            uint16 opcode = this->mStream->ReadU16();
            std::string opcodePrefix;
            if (end)
            {
                break;
            }
            if (opcode == 0x203)
            {
                end = true;
            }

            if (opcode >= 0x204 && opcode < 0x300)
            {
                this->_insts.push_back(new FF7WorldKernelCallInstruction());
                this->_insts.back()->_opcode = opcode;
                this->_insts.back()->_address = this->_address;
                this->_insts.back()->_stackChange = 0;
                this->_insts.back()->_name = std::string("function call");
                this->_insts.back()->_codeGenData = "";
                this->readParams(this->_insts.back(), "");
            }
            else
            {
                switch (opcode)
                {
                case 0x100:
                    this->_insts.push_back(new FF7WorldNoOutputInstruction());
                    this->_insts.back()->_opcode = 0x100;
                    this->_insts.back()->_address = this->_address;
                    this->_insts.back()->_stackChange = 0;
                    this->_insts.back()->_name = std::string("reset stack");
                    this->_insts.back()->_codeGenData = "";
                    this->readParams(this->_insts.back(), "");
                    break;

                    OPCODE(0x201, "jump if false", FF7WorldCondJumpInstruction, 0, "w"); 
                    OPCODE(0x203, "return", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x200, "jump", FF7WorldUncondJumpInstruction, 0, "w");

                    OPCODE(0x118, "push byte from bank 0", FF7WorldLoadBankInstruction, 2, "w");
                    OPCODE(0x110, "push constant", FF7WorldLoadInstruction, 2, "w");
                    OPCODE(0x119, "push byte from bank 1", FF7WorldLoadBankInstruction, 2, "w");
                    OPCODE(0x11c, "push word from bank 0", FF7WorldLoadBankInstruction, 2, "w");
                    OPCODE(0x114, "push bit from bank 0", FF7WorldLoadBankInstruction, 2, "w");
                    OPCODE(0x11b, "push special", FF7WorldLoadBankInstruction, 2, "w");
                    OPCODE(0x11f, "push special", FF7WorldLoadInstruction, 2, "w");
                    OPCODE(0x117, "push special", FF7WorldLoadBankInstruction, 2, "w");
           
                    OPCODE(0x0e0, "write bank", FF7WorldStoreInstruction, 0, "");
                    OPCODE(0x019, "push distance from active entity to entity by model id", FF7WorldKernelCallInstruction, 2, "");
                    OPCODE(0x018, "push distance from active entity to light", FF7WorldKernelCallInstruction, 2, "");

                    OPCODE(0x061, "push greater", FF7SubStackInstruction, 2, "");
                    OPCODE(0x0b0, "push logicand", FF7SubStackInstruction, 2, "");
                    OPCODE(0x062, "push lessequal", FF7SubStackInstruction, 2, "");
                    OPCODE(0x030, "push mul", FF7SubStackInstruction, 2, "");
                    OPCODE(0x051, "push shr", FF7SubStackInstruction, 2, "");
                    OPCODE(0x080, "push and", FF7SubStackInstruction, 2, "");
                    OPCODE(0x0a0, "push or", FF7SubStackInstruction, 2, "");
                    OPCODE(0x070, "push equal", BinaryEqualStackInstruction, 2, "");
                    OPCODE(0x041, "push sub", FF7SubStackInstruction, 2, "");
                    OPCODE(0x063, "push greaterequal", FF7SubStackInstruction, 2, "")
                    OPCODE(0x017, "push logicnot", FF7SubStackInstruction, 2, "");
                    OPCODE(0x060, "push less", FF7SubStackInstruction, 2, "");
                    OPCODE(0x0c0, "push logicor", FF7SubStackInstruction, 2, "");
                    OPCODE(0x040, "push add", FF7SubStackInstruction, 2, "");
                    OPCODE(0x015, "push neg", FF7SubStackInstruction, 2, "");

                    OPCODE(0x320, "unknown 320", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34a, "unknown 34a", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x334, "unknown 334", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x33a, "unknown 33a", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x31c, "unknown 31c", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32f, "unknown 32f", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x339, "unknown 339", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x30d, "unknown 30d", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x354, "unknown 354", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x332, "unknown 332", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x31f, "unknown 31f", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x329, "unknown 329", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32a, "unknown 32a", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34d, "unknown 3d4", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34e, "unknown 3de", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x30a, "unknown 30a", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x333, "unknown 333", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x331, "unknown 331", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x319, "unknown 319", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x353, "unknown 353", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x33e, "unknown 33e", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x321, "unknown 321", FF7WorldKernelCallInstruction, 0, "");

                    OPCODE(0x31d, "play sound effect", FF7WorldKernelCallInstruction, 2, "w");
                    OPCODE(0x336, "set active entity movespeed (honor walkmesh)", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x328, "set active entity direction", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x33d, "set field entry point2?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x351, "set music volume", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x352, "shake camera on / off", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34f, "set active entity y position", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x30b, "set active entity y offset", FF7WorldKernelCallInstruction, 0, "");                   
                    OPCODE(0x347, "move active entity to entity by model id ?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x30e, "active entity play animation", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x305, "set wait frames", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x306, "wait?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x307, "set control lock", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x303, "set active entity movespeed", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32c, "set window parameters", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32d, "wait for window ready", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x324, "set window dimensions", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x325, "set window message", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32e, "wait for message acknowledge", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x32b, "set battle lock", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x326, "set window prompt", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x327, "wait for prompt acknowledge ?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x318, "enter field scene", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x33b, "fade out?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x348, "fade in ?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x330, "set active entity", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x350, "set meteor texture on/off", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34b, "set chocobo type", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x34c, "set submarine color", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x349, "set world progress", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x300, "load model", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x302, "set player entity", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x30c, "enter vehicle?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x308, "set active entity mesh coordinates", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x309, "set active entity coordinates in mesh", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x304, "set active entity direction & facing", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x310, "set active point", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x311, "set point mesh coordinates", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x312, "set point coordinates in mesh", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x313, "set point terrain BGR", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x314, "set point dropoff parameters", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x315, "set point sky BGR", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x316, "set point BGR3?", FF7WorldKernelCallInstruction, 0, "");
                    OPCODE(0x317, "trigger battle", FF7WorldKernelCallInstruction, 0, "");
                default:
                    throw UnknownOpcodeException(this->_address, opcode);
                }
            }
            this->_address += 2;
        }

        for (auto& instruction : this->_insts)
        {
            instruction->_address = instruction->_address / 2;
        }
    }
}

