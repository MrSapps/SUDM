#pragma once

#include "simple_disassembler.h"
#include <array>

class BinaryReader;
class Function;

namespace FF7 
{
    enum eOpcodes
    {
        RET = 0x0,
        REQ = 0x01,
        REQSW = 0x02,
        REQEW = 0x03,
		PREQ = 0x04,
		PREQSW = 0x05,
		PREQEW = 0x06,
        RETTO = 0x07,
        JOIN = 0x08,
        SPLIT = 0x09,
        SPTYE = 0x0A,
        GTPYE = 0x0B,
		DSKCG = 0x0E,
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
        IFUWL = 0x19,
		MINIGAME = 0x20,
        TUTOR = 0x21,
		BTMD2 = 0x22,
		BTRLD = 0x23,
        WAIT = 0x24,
        NFADE = 0x25,
        BLINK = 0x26,
		BGMOVIE = 0x27,
        KAWAI = 0x28,
		KAWIW = 0x29,
		PMOVA = 0x2A,
		SLIP = 0x2B,
		BGPDH = 0x2C,
		BGSCR = 0x2D,
        WCLS = 0x2E,
        WSIZW = 0x2F,
        IFKEY = 0x30,
        IFKEYON = 0x31,
		IFKEYOFF = 0x32,
        UC = 0x33,
		PDIRA = 0x34,
		PTURA = 0x35,
        WSPCL = 0x36,
		WNUMB = 0x37,
        STTIM = 0x38,
        GOLDU = 0x39,
        GOLDD = 0x3A,
        CHGLD = 0x3B,
		HMPMAX1 = 0x3C,
		HMPMAX2 = 0x3D,
        MHMMX = 0x3E,
        HMPMAX3 = 0x3F,
        MESSAGE = 0x40,
		MPARA = 0x41,
		MPRA2 = 0x42,
        MPNAM = 0x43,
		MPU = 0x45,
		MPD = 0x47,
        ASK = 0x48,
        MENU = 0x49,
        MENU2 = 0x4A,
        BTLTB = 0x4B,
		HPU = 0x4D,
		HPD = 0x4F,
        WINDOW = 0x50,
		WMOVE = 0x51,
        WMODE = 0x52,
        WREST = 0x53,
        WCLSE = 0x54,
		WROW = 0x55,
		GWCOL = 0x56,
		SWCOL = 0x57,
        STITM = 0x58,
		DLITM = 0x59,
        CKITM = 0x5A,
        SMTRA = 0x5B,
		DMTRA = 0x5C,
		CMTRA = 0x5D,
        SHAKE = 0x5E,
        NOP = 0x5F,
        MAPJUMP = 0x60,
		SCRLO = 0x61,
        SCRLC = 0x62,
        SCRLA = 0x63,
        SCR2D = 0x64,
        SCRCC = 0x65,
        SCR2DC = 0x66,
        SCRLW = 0x67,
        SCR2DL = 0x68,
		MPDSP = 0x69,
        VWOFT = 0x6A,
        FADE = 0x6B,
        FADEW = 0x6C,
        IDLCK = 0x6D,
        LSTMP = 0x6E,
		SCRLP = 0x6F,
        BATTLE = 0x70,
        BTLON = 0x71,
        BTLMD = 0x72,
		PGTDR = 0x73,
		GETPC = 0x74,
		PXYZI = 0x75,
        PLUS_ = 0x76, // PLUS!
        PLUS2_ = 0x77, // PLUS2!
        MINUS_ = 0x78, // MINUS!
        MINUS2_ = 0x79, // MINUS2!
        INC_ = 0x7A, //  INC!
		INC2_ = 0x7B, // INC2!
		DEC_ = 0x7C, // DEC!
		DEC2_ = 0x7D, // DEC2!
        TLKON = 0x7E,
		RDMSD = 0x7F,
        SETBYTE = 0x80,
        SETWORD = 0x81,
        BITON = 0x82,
        BITOFF = 0x83,
		BITXOR = 0x84,
        PLUS = 0x85,
		PLUS2 = 0x86,
        MINUS = 0x87,
		MINUS2 = 0x88,
        MUL = 0x89,
		MUL2 = 0x8A,
		DIV = 0x8B,
		DIV2 = 0x8C,
        MOD = 0x8D,
		MOD2 = 0x8E,
        AND = 0x8F,
        AND2 = 0x90,
        OR = 0x91,
        OR2 = 0x92,
		XOR = 0x93,
		XOR2 = 0x94,
        INC = 0x95,
		INC2 = 0x96,
        DEC = 0x97,
		DEC2 = 0x98,
        RANDOM = 0x99,
		LBYTE = 0x9A,
		HBYTE = 0x9B,
		TWOBYTE = 0x9C,
		SETX = 0x9D,
		GETX = 0x9E,
		SEARCHX = 0x9F,
        PC = 0xA0,
        opCodeCHAR = 0xA1,
        DFANM = 0xA2,
        ANIME1 = 0xA3,
        VISI = 0xA4,
        XYZI = 0xA5,
        XYI = 0xA6,
		XYZ = 0xA7,
        MOVE = 0xA8,
        CMOVE = 0xA9,
        MOVA = 0xAA,
        TURA = 0xAB,
        ANIMW = 0xAC,
        FMOVE = 0xAD,
        ANIME2 = 0xAE,
        ANIM_1 = 0xAF, // ANIM!1
		CANIM1 = 0xB0,
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
        SLIDR = 0xC6,
        SOLID = 0xC7,
        PRTYP = 0xC8,
        PRTYM = 0xC9,
        PRTYE = 0xCA,
		IFPRTYQ = 0xCB,
		IFMEMBQ = 0xCC,
        MMBUD = 0xCD,
        MMBLK = 0xCE,
		MMBUK = 0xCF,
        LINE = 0xD0,
        LINON = 0xD1,
        MPJPO = 0xD2,
        SLINE = 0xD3,
		SIN = 0xD4,
		COS = 0xD5,
		TLKR2 = 0xD6,
		SLDR2 = 0xD7,
        PMJMP = 0xD8,
		PMJMP2 = 0xD9,
        AKAO2 = 0xDA,
		FCFIX = 0xDB,
		CCANM = 0xDC,
        ANIMB = 0xDD,
        TURNW = 0xDE,
		MPPAL = 0xDF,
        BGON = 0xE0,
        BGOFF = 0xE1,
		BGROL = 0xE2,
		BGROL2 = 0xE3,
        BGCLR = 0xE4,
        STPAL = 0xE5,
        LDPAL = 0xE6,
        CPPAL = 0xE7,
		RTPAL = 0xE8,
        ADPAL = 0xE9,
        MPPAL2 = 0xEA,
        STPLS = 0xEB,
        LDPLS = 0xEC,
		CPPAL2 = 0xED,
        RTPAL2 = 0xEE,
        ADPAL2 = 0xEF,
        MUSIC = 0xF0,
        SOUND = 0xF1,
        AKAO = 0xF2,
		MUSVT = 0xF3,
		MUSVM = 0xF4,
        MULCK = 0xF5,
        BMUSC = 0xF6,
		CHMPH = 0xF7,
        PMVIE = 0xF8,
        MOVIE = 0xF9,
        MVIEF = 0xFA,
        MVCAM = 0xFB,
        FMUSC = 0xFC,
		CMUSC = 0xFD,
        CHMST = 0xFE,
		GAMEOVER = 0xFF
    };

	enum eSpecialOpcodes
	{ 
		ARROW = 0xF5,
		PNAME = 0xF6,
		GMSPD = 0xF7,
		SMSPD = 0xF8,
		FLMAT = 0xF9,
		FLITM = 0xFA,
		BTLCK = 0xFB,
		MVLCK = 0xFC,
		SPCNM = 0xFD,
		RSGLB = 0xFE,
		CLITM = 0xFF
	};

    class FF7FieldEngine;
    class FF7Disassembler : public SimpleDisassembler 
    {
    public:
        FF7Disassembler(FF7FieldEngine* engine, InstVec& insts);
        ~FF7Disassembler();
        virtual void open(const char *filename) override;
	    virtual void doDisassemble() throw(std::exception) override;
    private:
        void DisassembleIndivdualScript(std::string entityName,
            size_t scriptIndex,
            int16 scriptEntryPoint,
            uint32 nextScriptEntryPoint,
            bool isStart,
            bool isEnd);

        void ReadOpCodes(size_t endPos);
        std::unique_ptr<Function> StartFunction(size_t scriptIndex);

        FF7FieldEngine* mEngine;

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
                    throw TooManyReturnStatementsException();
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
