#pragma once

#include "simple_disassembler.h"
#include <array>

class BinaryReader;
class Function;

namespace FF7 
{
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
