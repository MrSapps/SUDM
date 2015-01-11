#pragma once

#include "simple_disassembler.h"
#include <array>

class BinaryReader;
class Function;

namespace FF7 
{
    enum class eOpcodes : uint8
    {
        RET = 0x0,
        REQ = 0x01,
        REQSW = 0x02,
        REQEW = 0x03,
        RETTO = 0x07,
        JOIN = 0x08,
        SPLIT = 0x09,
        SPTYE = 0x0A,
        GTPYE = 0x0B,
        SPECIAL = 0x0F,
        JMPF = 0x10,
        JMPFL = 0x11,
        JMPB = 0x12,
        JMPBL = 0x13,
        IFUB = 0x14,
        IFUBL = 0x15,
        IFSW = 0x16,
        IFSWL = 0x17,
        IFUW = 0x18,
        TUTOR = 0x21,
        WAIT = 0x24,
        NFADE = 0x25,
        BLINK = 0x26,
        KAWAI = 0x28,
        WCLS = 0x2E,
        BGSCR = 0x2d,
        WSIZW = 0x2F,
        IFKEY = 0x30,
        IFKEYON = 0x31,
        UC = 0x33,
        WSPCL = 0x36,
        STTIM = 0x38,
        GOLDU = 0x39,
        GOLDD = 0x3A,
        CHGLD = 0x3B,
        MHMMX = 0x3E,
        HMPMAX3 = 0x3F,
        MESSAGE = 0x40,
        MPNAM = 0x43,
        ASK = 0x48,
        MENU = 0x49,
        MENU2 = 0x4A,
        BTLTB = 0x4B,
        WINDOW = 0x50,
        WMODE = 0x52,
        WREST = 0x53,
        WCLSE = 0x54,
        STITM = 0x58,
        CKITM = 0x5A,
        SMTRA = 0x5B,
        SHAKE = 0x5E,
        NOP = 0x5F,
        MAPJUMP = 0x60,
        SCRLC = 0x62,
        SCRLA = 0x63,
        SCR2D = 0x64,
        SCRCC = 0x65,
        SCR2DC = 0x66,
        SCRLW = 0x67,
        SCR2DL = 0x68,
        VWOFT = 0x6A,
        FADE = 0x6B,
        FADEW = 0x6C,
        IDLCK = 0x6D,
        LSTMP = 0x6E,
        BATTLE = 0x70,
        BTLON = 0x71,
        BTLMD = 0x72,
        PLUS_ = 0x76, // PLUS!
        PLUS2_ = 0x77, // PLUS2!
        MINUS_ = 0x78, // MINUS!
        MINUS2_ = 0x79, // MINUS2!
        INC_ = 0x7A, //  INC!
        TALKON = 0x7E,
        SETBYTE = 0x80,
        SETWORD = 0x81,
        BITON = 0x82,
        BITOFF = 0x83,
        PLUS = 0x85,
        MINUS = 0x87,
        MUL = 0x89,
        MOD = 0x8D,
        AND = 0x8F,
        AND2 = 0x90,
        OR = 0x91,
        OR2 = 0x92,
        INC = 0x95,
        DEC = 0x97,
        RANDOM = 0x99,
        PC = 0xA0,
        opCodeCHAR = 0xA1,
        DFANM = 0xA2,
        ANIME1 = 0xA3,
        VISI = 0xA4,
        XYZI = 0xA5,
        XYI = 0xA6,
        MOVE = 0xA8,
        CMOVE = 0xA9,
        MOVA = 0xAA,
        TURA = 0xAB,
        ANIMW = 0xAC,
        FMOVE = 0xAD,
        ANIME2 = 0xAE,
        ANIM_1 = 0xAF, // ANIM!1
        CANM_1 = 0xB1, // CANM!1 
        MSPED = 0xB2,
        DIR = 0xB3,
        TURNGEN = 0xB4,
        TURN = 0xB5,
        DIRA = 0xB6,
        GETDIR = 0xB7,
        GETAXY = 0xB8,
        GETAI = 0xB9,
        ANIM_2 = 0xBA, // ANIM!2
        CANIM2 = 0xBB,
        CANM_2 = 0xBC, // CANM!2
        ASPED = 0xBD,
        CC = 0xBF,
        JUMP = 0xC0,
        AXYZI = 0xC1,
        LADER = 0xC2,
        OFST = 0xC3,
        OFSTW = 0xC4,
        TALKR = 0xC5,
        CLIDR = 0xC6,
        SOLID = 0xC7,
        PRTYP = 0xC8,
        PRTYM = 0xC9,
        PRTYE = 0xCA,
        MMBUD = 0xCD,
        MMBLK = 0xCE,
        LINE = 0xD0,
        LINON = 0xD1,
        MPJPO = 0xD2,
        SLINE = 0xD3,
        PMJMP = 0xD8,
        AKAO2 = 0xDA,
        ANIMB = 0xDD,
        TURNW = 0xDE,
        BGON = 0xE0,
        BGOFF = 0xE1,
        BGCLR = 0xE4,
        STPAL = 0xE5,
        LDPAL = 0xE6,
        CPPAL = 0xE7,
        ADPAL = 0xE9,
        MPPAL2 = 0xEA,
        STPLS = 0xEB,
        LDPLS = 0xEC,
        RTPAL2 = 0xEE,
        ADPAL2 = 0xEF,
        MUSIC = 0xF0,
        SOUND = 0xF1,
        AKAO = 0xF2,
        MULCK = 0xF5,
        BMUSC = 0xF6,
        PMVIE = 0xF8,
        MOVIE = 0xF9,
        MVIEF = 0xFA,
        MVCAM = 0xFB,
        FMUSC = 0xFC,
        CHMST = 0xFE
    };

    class FF7Engine;
    class FF7Disassembler : public SimpleDisassembler 
    {
    public:
        FF7Disassembler(FF7Engine* engine, InstVec& insts);
        ~FF7Disassembler();
        virtual void open(const char *filename) override;
	    virtual void doDisassemble() throw(std::exception) override;
    private:
        void ReadOpCodes(size_t endPos);
        std::unique_ptr<Function> StartFunction(size_t entityNumber, size_t scriptIndex);

        FF7Engine* mEngine;

        uint32 mHeaderEndPos = 0;
        void ReadHeader(BinaryReader& reader)
        {
            mHeader.Read(reader);
            mHeaderEndPos = reader.Position();
        }

        const static int kMagic = 0x0502;
        const static int kNumSections = 7;
        const static int kSectionPointersSize = sizeof(uint32) * kNumSections;
        enum eSections
        {
            eScript = 0,
            eWalkMesh = 1,
            eTileMap = 2,
            eCameraMatrix = 3,
            eTriggers = 4,
            eEncounter = 5,
            eModels = 6
        };

        std::array<uint32, 7> mSections;
        uint32 GetEndOfScriptOffset(uint16 curEntryPoint, size_t entityIndex, size_t scriptIndex);

        struct ScriptHeader
        {
            uint16 mMagic;
            char mNumberOfEntities;
            char mNumberOfModels;
            uint16 mOffsetToStrings;
            uint16 mNumberOfAkaoOffsets; // Specifies the number of Akao/tuto blocks/offsets
            uint16 mScale; // Scale of field. For move and talk calculation (9bit fixed point).
            std::array<uint16, 3> mBlank;
            std::array<char, 8> mCreator;// Field creator (never shown)
            std::array<char, 8> mName;// Field name (never shown)
            std::vector<std::array<char, 8>> mFieldEntityNames;  // Count is mNumberOfEntities
            std::vector<uint32> mAkaoOffsets;	// Akao/Tuto block offsets, count is mNumberOfAkaoOffsets

            // Entity script entry points, or more explicitly, subroutine offsets
            std::vector<std::array<uint16, 32>> mEntityScripts; // Count is mNumberOfEntities
          
            void Read(BinaryReader& r)
            {
                mMagic = r.ReadU16();
                if (mMagic != kMagic)
                {
                    abort();
                }
                mNumberOfEntities = r.ReadU8();
                mNumberOfModels = r.ReadU8();
                mOffsetToStrings = r.ReadU16();
                mNumberOfAkaoOffsets = r.ReadU16();
                mScale = r.ReadU16();
                for (int i = 0; i < 3; i++)
                {
                    mBlank[i] = r.ReadU16();
                }

                for (int i = 0; i < 8; i++)
                {
                    mCreator[i] = r.ReadU8();
                }

                for (int i = 0; i < 8; i++)
                {
                    mName[i] = r.ReadU8();
                }

                for (int i = 0; i < mNumberOfEntities; i++)
                {
                    std::array<char, 8> name;
                    for (int j = 0; j < 8; j++)
                    {
                        name[j] = r.ReadU8();
                    }
                    mFieldEntityNames.emplace_back(name);
                }

                for (int i = 0; i < mNumberOfAkaoOffsets; i++)
                {
                    mAkaoOffsets.emplace_back(r.ReadU32());
                }

                mEntityScripts.reserve(mNumberOfEntities);
                for (int i = 0; i < mNumberOfEntities; i++)
                {
                    std::array<uint16, 32> scripts;
                    for (int j = 0; j < 32; j++)
                    {
                        scripts[j] = r.ReadU16();
                    }
                    mEntityScripts.push_back(scripts);
                }
            }
        };
        ScriptHeader mHeader;
    };

} 
