/* ScummVM Tools
 *
 * ScummVM Tools is the legal property of its developers, whose
 * names are too numerous to list here. Please refer to the
 * COPYRIGHT file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#include "disassembler/pasc.h"
#include "disassembler/subopcode.h"
#include "decompiler/scummv6/disassembler.h"
#include <gmock/gmock.h>


TEST(Disassembler, testDisassembly) {
    try {
        InstVec insts;
        PasCDisassembler p(insts);
        p.open("decompiler/test/hanoi20.pasb");
        p.disassemble();
        ASSERT_EQ(insts[0]->_address, 0);
        ASSERT_EQ(insts[0]->_opcode, 0x00);
        ASSERT_EQ(insts[0]->_name, "PUSH");
        ASSERT_EQ(insts[0]->_stackChange, 0);
        ASSERT_TRUE(insts[0]->_params[0]->isInteger());
        ASSERT_EQ(insts[0]->_params[0]->getSigned(), 0x60);
    }
    catch (UnknownOpcodeException &uoe) {
        printf("Exception message: %s\n", uoe.what());
        ASSERT_TRUE(false);
    }
    catch (std::exception &ex) {
        printf("Exception message: %s\n", ex.what());
        ASSERT_TRUE(false);
    }
}

TEST(Disassembler, testSubOpcodeDisassembly) {
    InstVec insts;
    SubOpcodeDisassembler s(insts);
    s.open("decompiler/test/subopcode_test.bin");
    s.disassemble();
    ASSERT_TRUE(insts[0]->_name == "FOO");
    ASSERT_TRUE(insts[0]->_opcode == 0xFFFF);
}

TEST(Disassembler, testUnknownOpcodeException) {
    try {
        InstVec insts;
        SubOpcodeDisassembler s(insts);
        s.open("decompiler/test/unknownopcode_test.bin");
        s.disassemble();
        ASSERT_TRUE(false);
    }
    catch (UnknownOpcodeException) {
        ASSERT_TRUE(true);
    }
}

// This test requires script-15.dmp from Sam & Max: Hit The Road.
// 1ab08298c9c8fb4c77953756989c7449 *script-15.dmp
// Disabled as mentioned file is copyrighted
TEST(Disassembler, DISABLED_testScummv6DisassemblerScript15) {
    InstVec insts;
    Scumm::v6::Scummv6Disassembler s(insts);
    s.open("decompiler/test/script-15.dmp");
    s.disassemble();
    ASSERT_TRUE(insts.size() == 11);
    ASSERT_TRUE(insts[0]->_address == 0);
    ASSERT_TRUE(insts[0]->_opcode == 0x03);
    ASSERT_TRUE(insts[0]->_name == "pushWordVar");
    ASSERT_TRUE(insts[0]->_params[0]->getUnsigned() == 16384);
    ASSERT_TRUE(insts[1]->_address == 3);
    ASSERT_TRUE(insts[1]->_opcode == 0x43);
    ASSERT_TRUE(insts[1]->_name == "writeWordVar");
    ASSERT_TRUE(insts[1]->_params[0]->getUnsigned() == 197);
    ASSERT_TRUE(insts[2]->_address == 6);
    ASSERT_TRUE(insts[2]->_opcode == 0x01);
    ASSERT_TRUE(insts[2]->_name == "pushWord");
    ASSERT_TRUE(insts[2]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[3]->_address == 9);
    ASSERT_TRUE(insts[3]->_opcode == 0x01);
    ASSERT_TRUE(insts[3]->_name == "pushWord");
    ASSERT_TRUE(insts[3]->_params[0]->getSigned() == 11);
    ASSERT_TRUE(insts[4]->_address == 12);
    ASSERT_TRUE(insts[4]->_opcode == 0x01);
    ASSERT_TRUE(insts[4]->_name == "pushWord");
    ASSERT_TRUE(insts[4]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[5]->_address == 15);
    ASSERT_TRUE(insts[5]->_opcode == 0x5E);
    ASSERT_TRUE(insts[5]->_name == "startScript");
    ASSERT_TRUE(insts[6]->_address == 16);
    ASSERT_TRUE(insts[6]->_opcode == 0x01);
    ASSERT_TRUE(insts[6]->_name == "pushWord");
    ASSERT_TRUE(insts[6]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[7]->_address == 19);
    ASSERT_TRUE(insts[7]->_opcode == 0x01);
    ASSERT_TRUE(insts[7]->_name == "pushWord");
    ASSERT_TRUE(insts[7]->_params[0]->getSigned() == 14);
    ASSERT_TRUE(insts[8]->_address == 22);
    ASSERT_TRUE(insts[8]->_opcode == 0x01);
    ASSERT_TRUE(insts[8]->_name == "pushWord");
    ASSERT_TRUE(insts[8]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[9]->_address == 25);
    ASSERT_TRUE(insts[9]->_opcode == 0x5E);
    ASSERT_TRUE(insts[9]->_name == "startScript");
    ASSERT_TRUE(insts[10]->_address == 26);
    ASSERT_TRUE(insts[10]->_opcode == 0x66);
    ASSERT_TRUE(insts[10]->_name == "stopObjectCodeB");
}

// This test requires script-31.dmp from Sam & Max: Hit The Road.
// f75f7ce110f378735d449f8eeb4a68e5 *script-31.dmp
// Disabled as mentioned file is copyrighted
TEST(Disassembler, DISABLED_testScummv6DisassemblerScript31) {
    InstVec insts;
    Scumm::v6::Scummv6Disassembler s(insts);
    s.open("decompiler/test/script-31.dmp");
    s.disassemble();
    ASSERT_TRUE(insts.size() == 5);
    ASSERT_TRUE(insts[0]->_address == 0);
    ASSERT_TRUE(insts[0]->_opcode == 0x01);
    ASSERT_TRUE(insts[0]->_name == "pushWord");
    ASSERT_TRUE(insts[0]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[1]->_address == 3);
    ASSERT_TRUE(insts[1]->_opcode == 0x43);
    ASSERT_TRUE(insts[1]->_name == "writeWordVar");
    ASSERT_TRUE(insts[1]->_params[0]->getUnsigned() == 180);
    ASSERT_TRUE(insts[2]->_address == 6);
    ASSERT_TRUE(insts[2]->_opcode == 0x01);
    ASSERT_TRUE(insts[2]->_name == "pushWord");
    ASSERT_TRUE(insts[2]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[3]->_address == 9);
    ASSERT_TRUE(insts[3]->_opcode == 0x43);
    ASSERT_TRUE(insts[3]->_name == "writeWordVar");
    ASSERT_TRUE(insts[3]->_params[0]->getUnsigned() == 181);
    ASSERT_TRUE(insts[4]->_address == 12);
    ASSERT_TRUE(insts[4]->_opcode == 0x66);
    ASSERT_TRUE(insts[4]->_name == "stopObjectCodeB");
}

// This test requires script-33.dmp from Sam & Max: Hit The Road.
// 9f09418bf34abbdec0ec54f388d8dca4 *script-33.dmp
// Disabled as mentioned file is copyrighted
TEST(Disassembler, DISABLED_testScummv6DisassemblerScript33) {
    InstVec insts;
    Scumm::v6::Scummv6Disassembler s(insts);
    s.open("decompiler/test/script-33.dmp");
    s.disassemble();
    ASSERT_TRUE(insts.size() == 10);
    ASSERT_TRUE(insts[0]->_address == 0);
    ASSERT_TRUE(insts[0]->_opcode == 0x01);
    ASSERT_TRUE(insts[0]->_name == "pushWord");
    ASSERT_TRUE(insts[0]->_params[0]->getSigned() == 0);
    ASSERT_TRUE(insts[1]->_address == 3);
    ASSERT_TRUE(insts[1]->_opcode == 0x43);
    ASSERT_TRUE(insts[1]->_name == "writeWordVar");
    ASSERT_TRUE(insts[1]->_params[0]->getUnsigned() == 71);
    ASSERT_TRUE(insts[2]->_address == 6);
    ASSERT_TRUE(insts[2]->_opcode == 0x03);
    ASSERT_TRUE(insts[2]->_name == "pushWordVar");
    ASSERT_TRUE(insts[2]->_params[0]->getUnsigned() == 177);
    ASSERT_TRUE(insts[3]->_address == 9);
    ASSERT_TRUE(insts[3]->_opcode == 0x43);
    ASSERT_TRUE(insts[3]->_name == "writeWordVar");
    ASSERT_TRUE(insts[3]->_params[0]->getUnsigned() == 173);
    ASSERT_TRUE(insts[4]->_address == 12);
    ASSERT_TRUE(insts[4]->_opcode == 0x01);
    ASSERT_TRUE(insts[4]->_name == "pushWord");
    ASSERT_TRUE(insts[4]->_params[0]->getSigned() == 874);
    ASSERT_TRUE(insts[5]->_address == 15);
    ASSERT_TRUE(insts[5]->_opcode == 0x43);
    ASSERT_TRUE(insts[5]->_name == "writeWordVar");
    ASSERT_TRUE(insts[5]->_params[0]->getUnsigned() == 177);
    ASSERT_TRUE(insts[6]->_address == 18);
    ASSERT_TRUE(insts[6]->_opcode == 0x03);
    ASSERT_TRUE(insts[6]->_name == "pushWordVar");
    ASSERT_TRUE(insts[6]->_params[0]->getUnsigned() == 177);
    ASSERT_TRUE(insts[7]->_address == 21);
    ASSERT_TRUE(insts[7]->_opcode == 0x01);
    ASSERT_TRUE(insts[7]->_name == "pushWord");
    ASSERT_TRUE(insts[7]->_params[0]->getSigned() == 93);
    ASSERT_TRUE(insts[8]->_address == 24);
    ASSERT_TRUE(insts[8]->_opcode == 0x6B99);
    ASSERT_TRUE(insts[8]->_name == "cursorCommand.setCursorImg");
    ASSERT_TRUE(insts[9]->_address == 26);
    ASSERT_TRUE(insts[9]->_opcode == 0x66);
    ASSERT_TRUE(insts[9]->_name == "stopObjectCodeB");
}

// This test requires room-9-202.dmp from Sam & Max: Hit The Road.
// f010dc659264674a2b6da298acd0b88b *room-9-202.dmp
TEST(Disassembler, DISABLED_testScummv6StackChangeFixRoom9202) {
    InstVec insts;
    Scumm::v6::Scummv6Disassembler s(insts);
    s.open("decompiler/test/room-9-202.dmp");
    s.disassemble();
    InstIterator it = insts.end();
    it -= 8;
    ASSERT_TRUE((*it)->_stackChange == -3);
}

// This test requires script-30.dmp from Sam & Max: Hit The Road.
// 6e48faca13e1f6df9341567608962744 *script-30.dmp
TEST(Disassembler, DISABLED_testScummv6StackChangeFixScript30) {
    InstVec insts;
    Scumm::v6::Scummv6Disassembler s(insts);
    s.open("decompiler/test/script-30.dmp");
    s.disassemble();
    InstIterator it = insts.end();
    it -= 3;
    ASSERT_TRUE((*it)->_stackChange == -6);
}
