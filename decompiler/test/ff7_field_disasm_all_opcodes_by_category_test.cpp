#include <gmock/gmock.h>
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "control_flow.h"
#include "util.h"
#include "make_unique.h"
#include "ff7_field_dummy_formatter.h"

#define MAKE_SUBOPCODE(high, low) ((uint16)(((uint8)(low)) | ((uint16)((uint8)(high))) << 8))

#define FLOW_BASE 0
#define FLOW_LABELS 13
#define FLOW_OPCODES (1/*init*/ + 37/*main, incl labels*/ - FLOW_LABELS)
#define MODULE_BASE (FLOW_BASE + FLOW_OPCODES + 1/*init*/)
#define MODULE_OPCODES 25
#define MATH_BASE (MODULE_BASE + MODULE_OPCODES + 1/*init*/)
#define MATH_OPCODES 40
#define WINDOW_BASE (MATH_BASE + MATH_OPCODES + 1/*init*/)
#define WINDOW_OPCODES 21
#define PARTY_BASE (WINDOW_BASE + WINDOW_OPCODES + 1/*init*/)
#define PARTY_OPCODES 26
#define MODEL_BASE (PARTY_BASE + PARTY_OPCODES + 1/*init*/)
#define MODEL_OPCODES 70
#define WALKMESH_BASE (MODEL_BASE + MODEL_OPCODES + 1/*init*/)
#define WALKMESH_OPCODES 6
#define BACKGND_BASE (WALKMESH_BASE + WALKMESH_OPCODES + 1/*init*/)
#define BACKGND_OPCODES 19
#define CAMERA_BASE (BACKGND_BASE + BACKGND_OPCODES + 1/*init*/)
#define CAMERA_OPCODES 15
#define AV_BASE (CAMERA_BASE + CAMERA_OPCODES + 1/*init*/)
#define AV_OPCODES 16
#define UNCAT_BASE (AV_BASE + AV_OPCODES + 1/*init*/)
#define UNCAT_OPCODES 4

#define ASSERT_OP_LEN(opcode, length) \
    ASSERT_EQ(insts[index]->_opcode, (opcode)); \
    ASSERT_EQ(insts[index]->_params.size(), (length)); \
    paramIndex = 0

#define ASSERT_OP_SUBOP_LEN(opcode, subopcode, length) \
    ASSERT_EQ(insts[index]->_opcode, MAKE_SUBOPCODE((opcode), (subopcode))); \
    ASSERT_EQ(insts[index]->_params.size(), (length)); \
    paramIndex = 0

#define ASSERT_PARAM_UNSIGNED(value) ASSERT_EQ(insts[index]->_params[paramIndex++]->getUnsigned(), (value))
#define ASSERT_PARAM_SIGNED(value) ASSERT_EQ(insts[index]->_params[paramIndex++]->getSigned(), (value))


void checkFlow(InstVec& insts);
void checkModule(InstVec& insts);
void checkMath(InstVec& insts);
void checkWindow(InstVec& insts);
void checkParty(InstVec& insts);
void checkModel(InstVec& insts);
void checkBackgnd(InstVec& insts);
void checkCamera(InstVec& insts);
void checkAv(InstVec& insts);
void checkUncat(InstVec& insts);


TEST(FF7Field, AllOpcodesDisassembler)
{
    //std::cout << "ready" << std::endl;
    //std::cin.ignore();


    InstVec insts;
    DummyFormatter formatter;
    FF7::FF7FieldEngine engine(formatter);

    auto d = engine.getDisassembler(insts);
    d->open("decompiler/test/ff7_all_opcodes_by_category.dat");
    d->disassemble();
    d->dumpDisassembly(std::cout);
    std::cout << std::endl;

    checkFlow(insts);
    checkModule(insts);
    /*
    checkMath(insts);
    checkWindow(insts);
    checkParty(insts);
    checkModel(insts);
    checkBackgnd(insts);
    checkCamera(insts);
    checkAv(insts);
    checkUncat(insts);
    */

    /*
    ASSERT_EQ(insts[0]->_opcode, FF7::eOpcodes::RET);
    ASSERT_EQ(insts[0]->_params.size(), 0);

    ASSERT_EQ(insts[1]->_opcode, FF7::eOpcodes::REQ);
    ASSERT_EQ(insts[1]->_params.size(), 3);
    ASSERT_EQ(insts[1]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[1]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[1]->_params[2]->getSigned(), 1);

    ASSERT_EQ(insts[2]->_opcode, FF7::eOpcodes::REQSW);
    ASSERT_EQ(insts[2]->_params.size(), 3);
    ASSERT_EQ(insts[2]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[2]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[2]->_params[2]->getSigned(), 1);

    ASSERT_EQ(insts[3]->_opcode, FF7::eOpcodes::REQEW);
    ASSERT_EQ(insts[3]->_params.size(), 3);
    ASSERT_EQ(insts[3]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[3]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[3]->_params[2]->getSigned(), 1);

    ASSERT_EQ(insts[4]->_opcode, FF7::eOpcodes::PREQ);
    ASSERT_EQ(insts[4]->_params.size(), 3);
    ASSERT_EQ(insts[4]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[4]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[4]->_params[2]->getSigned(), 1);

    ASSERT_EQ(insts[5]->_opcode, FF7::eOpcodes::PREQSW);
    ASSERT_EQ(insts[5]->_params.size(), 3);
    ASSERT_EQ(insts[5]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[5]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[5]->_params[2]->getSigned(), 1);

    ASSERT_EQ(insts[6]->_opcode, FF7::eOpcodes::PREQEW);
    ASSERT_EQ(insts[6]->_params.size(), 3);
    ASSERT_EQ(insts[6]->_params[0]->getSigned(), 0);
    ASSERT_EQ(insts[6]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[6]->_params[2]->getSigned(), 1);

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

    //ASSERT_EQ(insts[42]->_opcode, FF7::eOpcodes::BTMD2);
    //ASSERT_EQ(insts[42]->_params.size(), 4);
    //ASSERT_EQ(insts[42]->_params[1]->getSigned(), 0x80);

    ASSERT_EQ(insts[67]->_opcode, FF7::eOpcodes::BTRLD);
    ASSERT_EQ(insts[67]->_params.size(), 2);
    ASSERT_EQ(insts[67]->_params[0]->getSigned(), 1);
    //ASSERT_EQ(insts[67]->_params[0]->getSigned(), 1);

    ASSERT_EQ(insts[68]->_opcode, FF7::eOpcodes::WAIT);
    ASSERT_EQ(insts[68]->_params.size(), 1);
    ASSERT_EQ(insts[68]->_params[0]->getSigned(), 1);

    ASSERT_EQ(insts[69]->_opcode, FF7::eOpcodes::NFADE);
    ASSERT_EQ(insts[69]->_params.size(), 8);
    //ASSERT_EQ(insts[69]->_params[0]->getSigned(), 1); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(18)
    //ASSERT_EQ(insts[69]->_params[1]->getSigned(), 2); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(52)
    //ASSERT_EQ(insts[69]->_params[2]->getSigned(), 3); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(5)
    //ASSERT_EQ(insts[69]->_params[3]->getSigned(), 4); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(6)
    //ASSERT_EQ(insts[69]->_params[4]->getSigned(), 5); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(0)
    //ASSERT_EQ(insts[69]->_params[5]->getSigned(), 6); // TODO:These values are what they are set to in Makou Reactor however SUDM reads them as something else Reads:(0)
    ASSERT_EQ(insts[69]->_params[6]->getSigned(), 7);
    ASSERT_EQ(insts[69]->_params[7]->getSigned(), 8);

    ASSERT_EQ(insts[70]->_opcode, FF7::eOpcodes::BLINK);
    ASSERT_EQ(insts[70]->_params.size(), 1);
    ASSERT_EQ(insts[70]->_params[0]->getSigned(), 1);

    ASSERT_EQ(insts[71]->_opcode, FF7::eOpcodes::BGMOVIE);
    ASSERT_EQ(insts[71]->_params.size(), 1);
    ASSERT_EQ(insts[71]->_params[0]->getSigned(), 1);

    //KAWAI is normally here

    ASSERT_EQ(insts[72]->_opcode, FF7::eOpcodes::KAWIW);
    ASSERT_EQ(insts[72]->_params.size(), 0);

    ASSERT_EQ(insts[73]->_opcode, FF7::eOpcodes::PMOVA);
    ASSERT_EQ(insts[73]->_params.size(), 1);
    ASSERT_EQ(insts[73]->_params[0]->getSigned(), 1);

    ASSERT_EQ(insts[74]->_opcode, FF7::eOpcodes::SLIP);
    ASSERT_EQ(insts[74]->_params.size(), 1);
    ASSERT_EQ(insts[74]->_params[0]->getSigned(), 1);

    ASSERT_EQ(insts[75]->_opcode, FF7::eOpcodes::BGPDH);
    ASSERT_EQ(insts[75]->_params.size(), 4);
    ASSERT_EQ(insts[75]->_params[0]->getSigned(), 1);
    ASSERT_EQ(insts[75]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[75]->_params[2]->getSigned(), 3);
    ASSERT_EQ(insts[75]->_params[3]->getSigned(), 4);

    //ASSERT_EQ(insts[76]->_opcode, FF7::eOpcodes::BGSCR);
    //ASSERT_EQ(insts[76]->_params.size(), 5);
    //ASSERT_EQ(insts[76]->_params[0]->getSigned(), 1);
    //ASSERT_EQ(insts[76]->_params[1]->getSigned(), 2);
    //ASSERT_EQ(insts[76]->_params[2]->getSigned(), 3);
    //ASSERT_EQ(insts[76]->_params[3]->getSigned(), 4);
    //ASSERT_EQ(insts[76]->_params[4]->getSigned(), 5);

    ASSERT_EQ(insts[78]->_opcode, FF7::eOpcodes::WSIZW);
    ASSERT_EQ(insts[78]->_params.size(), 5);
    ASSERT_EQ(insts[78]->_params[0]->getSigned(), 1);
    ASSERT_EQ(insts[78]->_params[1]->getSigned(), 2);
    ASSERT_EQ(insts[78]->_params[2]->getSigned(), 3);
    ASSERT_EQ(insts[78]->_params[3]->getSigned(), 4);
    ASSERT_EQ(insts[78]->_params[4]->getSigned(), 5);

    //ASSERT_EQ(insts[79]->_opcode, FF7::eOpcodes::IFKEY); // TODO
    //ASSERT_EQ(insts[79]->_params.size(), 2);
    //ASSERT_EQ(0x0001, insts[79]->_params[0]->getSigned());

    //ASSERT_EQ(insts[80]->_opcode, FF7::eOpcodes::IFKEY); // TODO
    //ASSERT_EQ(insts[80]->_params.size(), 2);
    //ASSERT_EQ(0x8000, insts[78]->_params[0]->getSigned());

    //ASSERT_EQ(insts[81]->_opcode, FF7::eOpcodes::IFKEYON); // TODO
    //ASSERT_EQ(insts[81]->_params.size(), 2);
    //ASSERT_EQ(0x0001, insts[81]->_params[0]->getSigned());

    //ASSERT_EQ(insts[82]->_opcode, FF7::eOpcodes::IFKEYON); // TODO
    //ASSERT_EQ(insts[82]->_params.size(), 2);
    //ASSERT_EQ(0x8000, insts[82]->_params[0]->getSigned());

    //ASSERT_EQ(insts[83]->_opcode, FF7::eOpcodes::IFKEYOFF); // TODO
    //ASSERT_EQ(insts[83]->_params.size(), 2);
    //ASSERT_EQ(0x0001, insts[83]->_params[0]->getSigned());

    //ASSERT_EQ(insts[84]->_opcode, FF7::eOpcodes::IFKEYOFF); // TODO
    //ASSERT_EQ(insts[84]->_params.size(), 2);
    //ASSERT_EQ(0x8000, insts[84]->_params[0]->getSigned());

    ASSERT_EQ(insts[85]->_opcode, FF7::eOpcodes::UC);
    ASSERT_EQ(1, insts[85]->_params.size());
    ASSERT_EQ(1, insts[85]->_params[0]->getSigned());

    ASSERT_EQ(insts[86]->_opcode, FF7::eOpcodes::PDIRA);
    ASSERT_EQ(1, insts[86]->_params.size());
    ASSERT_EQ(1, insts[86]->_params[0]->getSigned());

    ASSERT_EQ(insts[87]->_opcode, FF7::eOpcodes::PTURA);
    ASSERT_EQ(3, insts[87]->_params.size());
    ASSERT_EQ(1, insts[87]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[87]->_params[1]->getSigned());
    ASSERT_EQ(3, insts[87]->_params[2]->getSigned());

    ASSERT_EQ(insts[88]->_opcode, FF7::eOpcodes::WSPCL);
    ASSERT_EQ(4, insts[88]->_params.size());
    ASSERT_EQ(1, insts[88]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[88]->_params[1]->getSigned());
    ASSERT_EQ(3, insts[88]->_params[2]->getSigned());
    ASSERT_EQ(4, insts[88]->_params[3]->getSigned());

    ASSERT_EQ(insts[89]->_opcode, FF7::eOpcodes::STTIM);
    ASSERT_EQ(7, insts[89]->_params.size());
    ASSERT_EQ(1, insts[89]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[89]->_params[1]->getSigned());
    ASSERT_EQ(3, insts[89]->_params[2]->getSigned());
    ASSERT_EQ(4, insts[89]->_params[3]->getSigned());
    ASSERT_EQ(5, insts[89]->_params[4]->getSigned());
    ASSERT_EQ(6, insts[89]->_params[5]->getSigned());
    ASSERT_EQ(7, insts[89]->_params[6]->getSigned());

    ASSERT_EQ(insts[90]->_opcode, FF7::eOpcodes::GOLDU);
    ASSERT_EQ(3, insts[90]->_params.size());
    ASSERT_EQ(0, insts[90]->_params[0]->getSigned());
    ASSERT_EQ(0, insts[90]->_params[1]->getSigned());
    ASSERT_EQ(1, insts[90]->_params[2]->getSigned());

    ASSERT_EQ(insts[91]->_opcode, FF7::eOpcodes::GOLDU);
    ASSERT_EQ(3, insts[91]->_params.size());
    ASSERT_EQ(1, insts[91]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[91]->_params[1]->getSigned());
    ASSERT_EQ(0, insts[91]->_params[2]->getSigned());

    ASSERT_EQ(insts[92]->_opcode, FF7::eOpcodes::GOLDD);
    ASSERT_EQ(3, insts[92]->_params.size());
    ASSERT_EQ(0, insts[92]->_params[0]->getSigned());
    ASSERT_EQ(0, insts[92]->_params[1]->getSigned());
    ASSERT_EQ(1, insts[92]->_params[2]->getSigned());

    ASSERT_EQ(insts[93]->_opcode, FF7::eOpcodes::GOLDD);
    ASSERT_EQ(3, insts[93]->_params.size());
    ASSERT_EQ(1, insts[93]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[93]->_params[1]->getSigned());
    ASSERT_EQ(0, insts[93]->_params[2]->getSigned());

    ASSERT_EQ(insts[94]->_opcode, FF7::eOpcodes::CHGLD);
    ASSERT_EQ(4, insts[94]->_params.size());
    ASSERT_EQ(1, insts[94]->_params[0]->getSigned());
    ASSERT_EQ(2, insts[94]->_params[1]->getSigned());
    ASSERT_EQ(3, insts[94]->_params[2]->getSigned());
    ASSERT_EQ(4, insts[94]->_params[3]->getSigned());

    */

    // If it had a value then check the values using:
    //ASSERT_EQ(insts[0]->_params[0]->getSigned(), 1);

    // To check amount of parameters use:
    //ASSERT_EQ(insts[1]->_params.size(), 1);

}



void checkFlow(InstVec& insts)
{
    int index = FLOW_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::RET, 0);
    index++;
    
    ASSERT_OP_LEN(FF7::eOpcodes::REQ, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::REQSW, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::REQEW, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PREQ, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PRQSW, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PRQEW, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::RETTO, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::JMPF, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::JMPFL, 1);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::JMPB, 1);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::JMPBL, 1);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFUB, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFUBL, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFSW, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFSWL, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFUW, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFUWL, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(3);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WAIT, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFKEY, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFKEYON, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFKEYOFF, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::NOP, 0);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFPRTYQ, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IFMEMBQ, 2);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(1);
}


void checkModule(InstVec& insts)
{
    int index = MODULE_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::DSKCG, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::ARROW, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::PNAME, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::GMSPD, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::SMSPD, 2);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::FLMAT, 0);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::FLITM, 0);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::BTLCK, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::MVLCK, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::SPCNM, 2);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::RSGLB, 0);
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::SPECIAL, FF7::eSpecialOpcodes::CLITM, 0);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MINIGAME, 6);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_SIGNED(-2);
    ASSERT_PARAM_SIGNED(-3);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(5);
    ASSERT_PARAM_UNSIGNED(6);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BTMD2, 1);
    ASSERT_PARAM_UNSIGNED(0xFFFFFFFF);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BTRLD, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BTLTB, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MAPJUMP, 5);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_SIGNED(-2);
    ASSERT_PARAM_SIGNED(-3);
    ASSERT_PARAM_UNSIGNED(4);
    ASSERT_PARAM_UNSIGNED(5);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LSTMP, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BATTLE, 3);
    ASSERT_PARAM_UNSIGNED(1);
    ASSERT_PARAM_UNSIGNED(2);
    ASSERT_PARAM_UNSIGNED(3);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BTLON, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BTLMD, 1);
    ASSERT_PARAM_UNSIGNED(0xFFFF);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPJPO, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PMJMP, 1);
    ASSERT_PARAM_UNSIGNED(1);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PMJMP2, 0);
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GAMEOVER, 0);
}


void checkMath(InstVec& insts)
{
    int index = MATH_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::PLUS_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PLUS2_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MINUS_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MINUS2_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::INC_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::INC2_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DEC_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DEC2_, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::RDMSD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SETBYTE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SETWORD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BITON, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BITOFF, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BITXOR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PLUS, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PLUS2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MINUS, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MINUS2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MUL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MUL2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DIV, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DIV2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MOD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MOD2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::AND, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::AND2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::OR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::OR2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::XOR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::XOR2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::INC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::INC2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DEC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DEC2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::RANDOM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LBYTE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HBYTE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TWOBYTE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SIN, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::COS, 3);
    // params
}


void checkWindow(InstVec& insts)
{
    int index = WINDOW_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::TUTOR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WCLS, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WSIZW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WSPCL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WNUMB, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::STTIM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MESSAGE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPARA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPRA2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPNAM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ASK, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MENU, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MENU2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WINDOW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WMOVE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WMODE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WREST, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WCLSE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::WROW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GWCOL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SWCOL, 3);
    // params
}


void checkParty(InstVec& insts)
{
    int index = PARTY_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::SPTYE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GTPYE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GOLDU, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GOLDD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CHGLD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HMPMAX1, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HMPMAX2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MHMMX, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HMPMAX3, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPU, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HPU, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::HPD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::STITM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DLITM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CKITM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SMTRA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DMTRA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CMTRA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GETPC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PRTYP, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PRTYM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PRTYE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MMBUD, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MMBLK, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MMBUK, 3);
    // params
}


void checkModel(InstVec& insts)
{
    int index = MODEL_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::JOIN, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SPLIT, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BLINK, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::EYETX, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::TRNSP, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::AMBNT, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::LIGHT, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::SBOBJ, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::SHINE, 3);
    // params
    index++;

    ASSERT_OP_SUBOP_LEN(FF7::eOpcodes::KAWAI, FF7::eKawaiOpcodes::RESET, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::KAWIW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PMOVA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PDIRA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PTURA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PGTDR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PXYZI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TLKON, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::opCodeCHAR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DFANM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIME1, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::VISI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::XYZI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::XYI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::XYZ, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MOVE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CMOVE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MOVA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TURA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIMW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::FMOVE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIME2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIM_1, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CANIM1, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CANM_1, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MSPED, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DIR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TURNGEN, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TURN, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::DIRA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GETDIR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GETAXY, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GETAI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIM_2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CANIM2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CANM_2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ASPED, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::JUMP, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::AXYZI, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LADER, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::OFST, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::OFSTW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TALKR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SLIDR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SOLID, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TLKR2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SLDR2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CCANM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::FCFIX, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ANIMB, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::TURNW, 3);
    // params
    index++;
}


void checkWalkmesh(InstVec& insts)
{
    int index = WALKMESH_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::SLIP, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::UC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::IDLCK, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LINE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LINON, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SLINE, 3);
    // params
    index++;
}


void checkBackgnd(InstVec& insts)
{
    int index = BACKGND_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::BGPDH, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGSCR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGON, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGOFF, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGROL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGROL2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BGCLR, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::STPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LDPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CPPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::RTPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ADPAL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MPPAL2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::STPLS, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::LDPLS, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CPPAL2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::RTPAL2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::ADPAL2, 3);
    // params
    index++;
}


void checkCamera(InstVec& insts)
{
    int index = CAMERA_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::NFADE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SHAKE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRLO, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRLC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRLA, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCR2D, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRCC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCR2DC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRLW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCR2DL, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::VWOFT, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::FADE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::FADEW, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SCRLP, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MVCAM, 3);
    // params
    index++;
}


void checkAv(InstVec& insts)
{
    int index = AV_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::BGMOVIE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::AKAO2, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MUSIC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SOUND, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::AKAO, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MUSVT, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MUSVM, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MULCK, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::BMUSC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CHMPH, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::PMVIE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MOVIE, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::MVIEF, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::FMUSC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CMUSC, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::CHMST, 3);
    // params
    index++;
}


void checkUncat(InstVec& insts)
{
    int index = UNCAT_BASE, paramIndex = 0;

    ASSERT_OP_LEN(FF7::eOpcodes::MPDSP, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SETX, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::GETX, 3);
    // params
    index++;

    ASSERT_OP_LEN(FF7::eOpcodes::SEARCHX, 3);
    // params
    index++;
}


#undef ASSERT_OP_LEN
#undef ASSERT_OP_SUBOP_LEN
#undef ASSERT_PARAM_UNSIGNED
#undef ASSERT_PARAM_SIGNED

#undef FLOW_BASE
#undef FLOW_LABELS
#undef FLOW_OPCODES
#undef MODULE_BASE
#undef MODULE_OPCODES
#undef MATH_BASE
#undef MATH_OPCODES
#undef WINDOW_BASE
#undef WINDOW_OPCODES
#undef PARTY_BASE
#undef PARTY_OPCODES
#undef MODEL_BASE
#undef MODEL_OPCODES
#undef WALKMESH_BASE
#undef WALKMESH_OPCODES
#undef BACKGND_BASE
#undef BACKGND_OPCODES
#undef CAMERA_BASE
#undef CAMERA_OPCODES
#undef AV_BASE
#undef AV_OPCODES
#undef UNCAT_BASE
#undef UNCAT_OPCODES