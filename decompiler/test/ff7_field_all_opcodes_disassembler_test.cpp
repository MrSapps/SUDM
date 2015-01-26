#include <gmock/gmock.h>
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "control_flow.h"
#include "util.h"
#include "make_unique.h"

// Remove DISABLED_ to make it execute
TEST(FF7Field, AllOpcodesDisassembler)
{
    InstVec insts;
    FF7::FF7FieldEngine engine;

    auto d = engine.getDisassembler(insts);
    d->open("decompiler/test/ff7_all_opcodes.dat");
    d->disassemble();
    d->dumpDisassembly(std::cout);
    std::cout << std::endl;


    ASSERT_EQ(insts[0]->_opcode, FF7::eOpcodes::RET);
    ASSERT_EQ(insts[0]->_params.size(), 0);

	ASSERT_EQ(insts[1]->_opcode, FF7::eOpcodes::REQ);
	ASSERT_EQ(insts[1]->_params.size(), 3);
	ASSERT_EQ(insts[1]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[1]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[1]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[2]->_opcode, FF7::eOpcodes::REQSW);
	ASSERT_EQ(insts[2]->_params.size(), 3);
	ASSERT_EQ(insts[2]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[2]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[2]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[3]->_opcode, FF7::eOpcodes::REQEW);
	ASSERT_EQ(insts[3]->_params.size(), 3);
	ASSERT_EQ(insts[3]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[3]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[3]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[4]->_opcode, FF7::eOpcodes::PREQ);
	ASSERT_EQ(insts[4]->_params.size(), 3);
	ASSERT_EQ(insts[4]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[4]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[4]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[5]->_opcode, FF7::eOpcodes::PREQSW);
	ASSERT_EQ(insts[5]->_params.size(), 3);
	ASSERT_EQ(insts[5]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[5]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[5]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[6]->_opcode, FF7::eOpcodes::PREQEW);
	ASSERT_EQ(insts[6]->_params.size(), 3);
	ASSERT_EQ(insts[6]->_params[0]->getSigned(), 0);
	//ASSERT_EQ(insts[6]->_params[1]->getSigned(), 2);
	//ASSERT_EQ(insts[6]->_params[2]->getSigned(), 1);

	ASSERT_EQ(insts[7]->_opcode, FF7::eOpcodes::JOIN);
	ASSERT_EQ(insts[7]->_params.size(), 1);
	ASSERT_EQ(insts[7]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[8]->_opcode, FF7::eOpcodes::SPLIT);
	ASSERT_EQ(insts[8]->_params.size(), 13);
	ASSERT_EQ(insts[8]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[8]->_params[1]->getSigned(), 2);
	ASSERT_EQ(insts[8]->_params[2]->getSigned(), 3);
	ASSERT_EQ(insts[8]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[8]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[8]->_params[5]->getSigned(), 6);
	ASSERT_EQ(insts[8]->_params[6]->getSigned(), 7);
	ASSERT_EQ(insts[8]->_params[7]->getSigned(), 8);


    // If it had a value then check the values using:
    //ASSERT_EQ(insts[0]->_params[0]->getSigned(), 1);

	// To check amount of parameters use:
	//ASSERT_EQ(insts[1]->_params.size(), 1);

}
