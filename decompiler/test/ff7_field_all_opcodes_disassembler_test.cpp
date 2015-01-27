#include <gmock/gmock.h>
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "control_flow.h"
#include "util.h"
#include "make_unique.h"

#define MAKE_SUBOPCODE(high, low) ((uint16)(((uint8)(low)) | ((uint16)((uint8)(high))) << 8))

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
	ASSERT_EQ(insts[8]->_params[8]->getSigned(), 9);
	ASSERT_EQ(insts[8]->_params[9]->getSigned(), 10);
	ASSERT_EQ(insts[8]->_params[10]->getSigned(), 11);
	ASSERT_EQ(insts[8]->_params[11]->getSigned(), 12);
	ASSERT_EQ(insts[8]->_params[12]->getSigned(), 13);

	ASSERT_EQ(insts[9]->_opcode, FF7::eOpcodes::SPTYE);
	ASSERT_EQ(insts[9]->_params.size(), 7);
	ASSERT_EQ(insts[9]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[9]->_params[1]->getSigned(), 2);
	ASSERT_EQ(insts[9]->_params[2]->getSigned(), 3);
	ASSERT_EQ(insts[9]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[9]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[9]->_params[5]->getSigned(), 6);
	ASSERT_EQ(insts[9]->_params[6]->getSigned(), 7);

	ASSERT_EQ(insts[10]->_opcode, FF7::eOpcodes::GTPYE);
	ASSERT_EQ(insts[10]->_params.size(), 7);
	ASSERT_EQ(insts[10]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[10]->_params[1]->getSigned(), 2);
	ASSERT_EQ(insts[10]->_params[2]->getSigned(), 3);
	ASSERT_EQ(insts[10]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[10]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[10]->_params[5]->getSigned(), 6);
	ASSERT_EQ(insts[10]->_params[6]->getSigned(), 7);

	ASSERT_EQ(insts[11]->_opcode, FF7::eOpcodes::DSKCG);
	ASSERT_EQ(insts[11]->_params.size(), 1);
	ASSERT_EQ(insts[11]->_params[0]->getSigned(), 1);

	
	ASSERT_EQ(insts[12]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::ARROW));
	ASSERT_EQ(insts[12]->_params.size(), 1);
	ASSERT_EQ(insts[12]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[13]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::PNAME));
	ASSERT_EQ(insts[13]->_params.size(), 1);
	ASSERT_EQ(insts[13]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[14]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::GMSPD));
	ASSERT_EQ(insts[14]->_params.size(), 1);
	ASSERT_EQ(insts[14]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[15]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::SMSPD));
	ASSERT_EQ(insts[15]->_params.size(), 2);
	ASSERT_EQ(insts[15]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[15]->_params[1]->getSigned(), 2);

	ASSERT_EQ(insts[16]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::FLMAT));
	ASSERT_EQ(insts[16]->_params.size(), 0);

	ASSERT_EQ(insts[17]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::FLITM));
	ASSERT_EQ(insts[17]->_params.size(), 0);

	ASSERT_EQ(insts[18]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::BTLCK));
	ASSERT_EQ(insts[18]->_params.size(), 1);
	ASSERT_EQ(insts[18]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[19]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::MVLCK));
	ASSERT_EQ(insts[19]->_params.size(), 1);
	ASSERT_EQ(insts[19]->_params[0]->getSigned(), 1);

	ASSERT_EQ(insts[20]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::SPCNM));
	ASSERT_EQ(insts[20]->_params.size(), 2);
	ASSERT_EQ(insts[20]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[20]->_params[1]->getSigned(), 2);

	ASSERT_EQ(insts[21]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::RSGLB));
	ASSERT_EQ(insts[21]->_params.size(), 0);

	ASSERT_EQ(insts[22]->_opcode, MAKE_SUBOPCODE(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::CLITM));
	ASSERT_EQ(insts[22]->_params.size(), 0);

	ASSERT_EQ(insts[23]->_opcode, FF7::eOpcodes::JMPF);
	ASSERT_EQ(insts[23]->_params.size(), 1);

	ASSERT_EQ(insts[24]->_opcode, FF7::eOpcodes::JMPFL);
	ASSERT_EQ(insts[24]->_params.size(), 1);

	ASSERT_EQ(insts[25]->_opcode, FF7::eOpcodes::JMPB);
	ASSERT_EQ(insts[25]->_params.size(), 1);

	ASSERT_EQ(insts[26]->_opcode, FF7::eOpcodes::JMPBL);
	ASSERT_EQ(insts[26]->_params.size(), 1);

	ASSERT_EQ(insts[27]->_opcode, FF7::eOpcodes::IFUB);
	ASSERT_EQ(insts[27]->_params.size(), 6);
	ASSERT_EQ(insts[27]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[27]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[27]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[27]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[27]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[27]->_params[5]->getSigned(), 1);

	ASSERT_EQ(insts[28]->_opcode, FF7::eOpcodes::IFUBL);
	ASSERT_EQ(insts[28]->_params.size(), 6);
	ASSERT_EQ(insts[28]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[28]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[28]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[28]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[28]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[28]->_params[5]->getSigned(), 2); // For some reason this one has to be 2

	ASSERT_EQ(insts[29]->_opcode, FF7::eOpcodes::IFSW);
	ASSERT_EQ(insts[29]->_params.size(), 6);
	ASSERT_EQ(insts[29]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[29]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[29]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[29]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[29]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[29]->_params[5]->getSigned(), 1);

	ASSERT_EQ(insts[30]->_opcode, FF7::eOpcodes::IFSWL);
	ASSERT_EQ(insts[30]->_params.size(), 6);
	ASSERT_EQ(insts[30]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[30]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[30]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[30]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[30]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[30]->_params[5]->getSigned(), 2); // For some reason this one has to be 2

	ASSERT_EQ(insts[31]->_opcode, FF7::eOpcodes::IFUW);
	ASSERT_EQ(insts[31]->_params.size(), 6);
	ASSERT_EQ(insts[31]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[31]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[31]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[31]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[31]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[31]->_params[5]->getSigned(), 1);

	ASSERT_EQ(insts[32]->_opcode, FF7::eOpcodes::IFUWL);
	ASSERT_EQ(insts[32]->_params.size(), 6);
	ASSERT_EQ(insts[32]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[32]->_params[1]->getSigned(), 3);
	ASSERT_EQ(insts[32]->_params[2]->getSigned(), 2);
	ASSERT_EQ(insts[32]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[32]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[32]->_params[5]->getSigned(), 2); // For some reason this one has to be 2

	ASSERT_EQ(insts[33]->_opcode, FF7::eOpcodes::MINIGAME);
	ASSERT_EQ(insts[33]->_params.size(), 6);
	ASSERT_EQ(insts[33]->_params[0]->getSigned(), 1);
	ASSERT_EQ(insts[33]->_params[1]->getSigned(), 2);
	ASSERT_EQ(insts[33]->_params[2]->getSigned(), 3);
	ASSERT_EQ(insts[33]->_params[3]->getSigned(), 4);
	ASSERT_EQ(insts[33]->_params[4]->getSigned(), 5);
	ASSERT_EQ(insts[33]->_params[5]->getSigned(), 6);

	ASSERT_EQ(insts[34]->_opcode, FF7::eOpcodes::TUTOR);
	ASSERT_EQ(insts[34]->_params.size(), 1);
	ASSERT_EQ(insts[34]->_params[0]->getSigned(), 1);

	// BATTLE MODE STATES START

	ASSERT_EQ(insts[35]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[35]->_params.size(), 4);
	ASSERT_EQ(insts[35]->_params[0]->getSigned(), 0x80);

	ASSERT_EQ(insts[36]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[36]->_params.size(), 4);
	ASSERT_EQ(insts[36]->_params[0]->getSigned(), 0x40);

	ASSERT_EQ(insts[37]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[37]->_params.size(), 4);
	ASSERT_EQ(insts[37]->_params[0]->getSigned(), 0x20);

	ASSERT_EQ(insts[38]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[38]->_params.size(), 4);
	ASSERT_EQ(insts[38]->_params[0]->getSigned(), 0x10);

	ASSERT_EQ(insts[39]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[39]->_params.size(), 4);
	ASSERT_EQ(insts[39]->_params[0]->getSigned(), 0x08);

	ASSERT_EQ(insts[40]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[40]->_params.size(), 4);
	ASSERT_EQ(insts[40]->_params[0]->getSigned(), 0x04);

	ASSERT_EQ(insts[41]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[41]->_params.size(), 4);
	ASSERT_EQ(insts[41]->_params[0]->getSigned(), 0x02);

	ASSERT_EQ(insts[42]->_opcode, FF7::eOpcodes::BTMD2);
	ASSERT_EQ(insts[42]->_params.size(), 4);
	ASSERT_EQ(insts[42]->_params[1]->getSigned(), 0x80);


    // If it had a value then check the values using:
    //ASSERT_EQ(insts[0]->_params[0]->getSigned(), 1);

	// To check amount of parameters use:
	//ASSERT_EQ(insts[1]->_params.size(), 1);

}
