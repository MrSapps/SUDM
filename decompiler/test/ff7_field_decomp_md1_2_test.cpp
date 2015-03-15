#include <gmock/gmock.h>

#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"

#include "control_flow.h"
#include "util.h"
#include "graph.h"
#include "make_unique.h"
#include "sudm.h"
#include "lzs.h"
#include "ff7_field_dummy_formatter.h"

TEST(FF7Field, Decomp_MD1_2)
{
    //std::cout << "ready" << std::endl;
    //std::cin.ignore();

    auto scriptBytes = Lzs::Decompress(BinaryReader::ReadAll("decompiler/test/md1_2.dat"));

    // Remove section pointers, leave everything after the script data as this doesn't matter
    const int kNumSections = 7;
    scriptBytes.erase(scriptBytes.begin(), scriptBytes.begin() + kNumSections * sizeof(uint32));
    DummyFormatter formatter;
    SUDM::FF7::Field::DecompiledScript ds = SUDM::FF7::Field::Decompile("md1_2", scriptBytes, formatter, "", "EntityContainer = {}\n\n");
    ASSERT_FALSE(ds.luaScript.empty());


    std::ofstream tmp("decompiler/test/md1_2.lua");
    if (!tmp.is_open())
    {
        throw std::runtime_error("Can't open md1_2.lua for writing");
    }

    tmp << ds.luaScript;
}
