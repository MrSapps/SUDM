#include <gmock/gmock.h>
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "control_flow.h"
#include "util.h"
#include "make_unique.h"
#include "ff7_field_dummy_formatter.h"

// Verify next 3 instructions are NOP's and return address of final NOP
static uint32 Is3Nops(InstVec& insts, uint32& pos)
{
    for (int i = 0; i < 3; i++)
    {
        pos++;
        EXPECT_EQ(insts[pos]->_opcode, FF7::eOpcodes::NOP);
        EXPECT_EQ(insts[pos]->_params.size(), 0);
    }
    return insts[pos]->_address;
}

// Each forward jump type has its own script that starts with the
// jump opcode followed by 2 NOP's, the 3rd NOP is the end of the script
// and should be the target address of the jump.
static void CheckForwardJump(FF7::eOpcodes opCode, size_t paramsSize, InstVec& insts, uint32& pos)
{
    pos++;
    const InstPtr& jumpInst = insts[pos];
    ASSERT_EQ(jumpInst->_opcode, opCode);
    ASSERT_EQ(jumpInst->_params.size(), paramsSize);

    const uint32 lastNopAddr = Is3Nops(insts, pos);
    ASSERT_EQ(jumpInst->getDestAddress(), lastNopAddr);
}

// Verify next instructions is a NOP and return address of final NOP
static uint32 IsNop(InstVec& insts, uint32& pos)
{
    pos++;
    EXPECT_EQ(insts[pos]->_opcode, FF7::eOpcodes::NOP);
    EXPECT_EQ(insts[pos]->_params.size(), 0);
    return insts[pos]->_address;
}


// For a backwards jump we start with 3 NOPS and the last instruction is the jump itself which
// should be targeting the second/middle NOP
static void CheckBackwardJump(FF7::eOpcodes opCode, size_t paramsSize, InstVec& insts, uint32& pos)
{
    IsNop(insts, pos);

    // Second is the target, so check is NOP & grab addr
    const uint32 firstNopAddr = IsNop(insts, pos);

    IsNop(insts, pos);

    // Grab the final instruction, the jump itself
    pos++;
    const InstPtr& jumpInst = insts[pos];

    ASSERT_EQ(jumpInst->_opcode, opCode);
    ASSERT_EQ(jumpInst->_params.size(), paramsSize);
    ASSERT_EQ(jumpInst->getDestAddress(), firstNopAddr);
}



TEST(FF7Field, ControlFlow)
{
    InstVec insts;
    DummyFormatter formatter;
    FF7::FF7FieldEngine engine(formatter, "test");

    auto d = engine.getDisassembler(insts);
    d->open("decompiler/test/ff7_control_flow_test.dat");
    d->disassemble();
    d->dumpDisassembly(std::cout);
    std::cout << std::endl;

    uint32 pos = 0;
    ASSERT_EQ(insts[pos]->_opcode, FF7::eOpcodes::RET);
    ASSERT_EQ(insts[pos]->_params.size(), 0);

    CheckForwardJump(FF7::eOpcodes::IFUB, 6, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFSWL, 6, insts, pos);
    CheckForwardJump(FF7::eOpcodes::JMPF, 1, insts, pos);
    CheckForwardJump(FF7::eOpcodes::JMPFL, 1, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFKEY, 2, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFKEYON, 2, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFKEYOFF, 2, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFPRTYQ, 2, insts, pos);
    CheckForwardJump(FF7::eOpcodes::IFMEMBQ, 2, insts, pos);
    CheckBackwardJump(FF7::eOpcodes::JMPB, 1, insts, pos);
    CheckBackwardJump(FF7::eOpcodes::JMPBL, 1, insts, pos);
}
