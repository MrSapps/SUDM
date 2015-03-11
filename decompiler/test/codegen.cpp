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


#include "decompiler/control_flow.h"
#include "decompiler/decompiler_disassembler.h"
#include "decompiler/graph.h"
#include "decompiler/decompiler_codegen.h"
#include "decompiler/scummv6/engine.h"

#include <vector>
#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

#include <streambuf>
#include <ostream>
#include <gmock/gmock.h>
#include "util.h"
#include "make_unique.h"

std::string removeSpaces(std::string s) {
    size_t found;
    while ((found = s.find(' ')) != std::string::npos)
        s = s.erase(found, 1);
    return s;
}

typedef std::vector<std::string>::iterator CodeIterator;


TEST(CodeGen, testContinue) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-do-while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    onullstream ns;
    auto cg = engine->getCodeGenerator(insts, ns);
    cg->generate(insts, g);

    VertexIterator v = boost::vertices(g).first;
    std::vector<std::string> output, expected;
    expected.push_back("do{");
    expected.push_back("if(18 != var321) {");
    expected.push_back("continue;");
    expected.push_back("}");
    expected.push_back("VAR_CHARSET_MASK--;");
    expected.push_back("} while (42 == VAR_CHARSET_MASK)");
    expected.push_back("stopObjectCodeA();");
    GroupPtr gr = GET(*v);
    // Find first node
    while (gr->_prev != NULL)
        gr = gr->_prev;
    // Copy out all lines of code
    while (gr != NULL) {
        for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
            output.push_back(it->_line);
        gr = gr->_next;
    }
    ASSERT_TRUE(output.size() == expected.size());
    CodeIterator it, it2;
    for (it = output.begin(), it2 = expected.begin(); it != output.end() && it2 != expected.end(); ++it, ++it2) {
        ASSERT_TRUE(removeSpaces(*it).compare(removeSpaces(*it2)) == 0);
    }
}

TEST(CodeGen, testBreak) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    onullstream ns;
    auto cg = engine->getCodeGenerator(insts, ns);
    cg->generate(insts, g);

    VertexIterator v = boost::vertices(g).first;
    std::vector<std::string> output, expected;
    expected.push_back("while (42 != VAR_CHARSET_MASK) {");
    expected.push_back("if (18 != var321) {");
    expected.push_back("break;");
    expected.push_back("}");
    expected.push_back("VAR_CHARSET_MASK--;");
    expected.push_back("}");
    expected.push_back("stopObjectCodeA();");
    GroupPtr gr = GET(*v);
    // Find first node
    while (gr->_prev != NULL)
        gr = gr->_prev;
    // Copy out all lines of code
    while (gr != NULL) {
        for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
            output.push_back(it->_line);
        gr = gr->_next;
    }
    ASSERT_TRUE(output.size() == expected.size());
    CodeIterator it, it2;
    for (it = output.begin(), it2 = expected.begin(); it != output.end() && it2 != expected.end(); ++it, ++it2) {
        ASSERT_TRUE(removeSpaces(*it).compare(removeSpaces(*it2)) == 0);
    }
}

TEST(CodeGen, testElse) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/if-else.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    onullstream ns;
    auto cg = engine->getCodeGenerator(insts, ns);
    cg->generate(insts, g);

    VertexIterator v = boost::vertices(g).first;
    std::vector<std::string> output, expected;
    expected.push_back("if (42 != VAR_CHARSET_MASK) {");
    expected.push_back("VAR_CHARSET_MASK--;");
    expected.push_back("} else {");
    expected.push_back("VAR_CHARSET_MASK++;");
    expected.push_back("}");
    expected.push_back("stopObjectCodeA();");
    GroupPtr gr = GET(*v);
    // Find first node
    while (gr->_prev != NULL)
        gr = gr->_prev;
    // Copy out all lines of code
    while (gr != NULL) {
        for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
            output.push_back(it->_line);
        gr = gr->_next;
    }
    ASSERT_TRUE(output.size() == expected.size());
    CodeIterator it, it2;
    for (it = output.begin(), it2 = expected.begin(); it != output.end() && it2 != expected.end(); ++it, ++it2) {
        ASSERT_TRUE(removeSpaces(*it).compare(removeSpaces(*it2)) == 0);
    }
}

// This test requires script-30 and script-48.dmp from Sam & Max: Hit The Road.
// 6e48faca13e1f6df9341567608962744 *script-30.dmp
// afd7dc5d377894b3b9d0504927adf1b1 *script-48.dmp
// Disabled as mentioned file is copyrighted
TEST(CodeGen, DISABLED_testCoalescing) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/script-30.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    onullstream ns;
    auto cg = engine->getCodeGenerator(insts, ns);
    cg->generate(insts, g);

    VertexIterator v = boost::vertices(g).first;
    GroupPtr gr = GET(*v);
    // Find first node
    while (gr->_prev != NULL)
        gr = gr->_prev;
    // Find vertex to test
    while ((*gr->_start)->_address != 0x91)
        gr = gr->_next;

    ASSERT_TRUE(gr->_code.size() == 2);
    ASSERT_TRUE(removeSpaces(gr->_code[0]._line).compare("}else{") == 0);
    ASSERT_TRUE(removeSpaces(gr->_code[1]._line).substr(0, 2).compare("if") == 0);

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/script-48.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    cg = engine->getCodeGenerator(insts, ns);
    cg->generate(insts, g);

    v = boost::vertices(g).first;
    gr = GET(*v);
    // Find first node
    while (gr->_prev != NULL)
        gr = gr->_prev;
    // Find vertex to test
    while ((*gr->_start)->_address != 0x191)
        gr = gr->_next;

    ASSERT_TRUE(gr->_code.size() == 1);
    ASSERT_TRUE(removeSpaces(gr->_code[0]._line).substr(0, 7).compare("}elseif") == 0);
}
