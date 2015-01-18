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
#include "decompiler/scummv6/engine.h"
#include <gmock/gmock.h>
#include <vector>
#include "make_unique.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))


TEST(CFG, testUnreachable) {
    InstVec insts;
    Scumm::v6::Scummv6Engine *engine = new Scumm::v6::Scummv6Engine();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/unreachable.dmp");
    d->disassemble();
    ControlFlow *c = new ControlFlow(insts, *engine);
    Graph g = c->getGraph();
    ASSERT_TRUE(boost::num_vertices(g) == 4);
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        switch ((*gr->_start)->_address) {
        case 0:
            ASSERT_TRUE(boost::in_degree(*it, g) == 0 && boost::out_degree(*it, g) == 1);
            break;
        case 2:
            ASSERT_TRUE(boost::in_degree(*it, g) == 1 && boost::out_degree(*it, g) == 1);
            break;
        case 5:
            ASSERT_TRUE(boost::in_degree(*it, g) == 0 && boost::out_degree(*it, g) == 1);
            break;
        case 6:
            ASSERT_TRUE(boost::in_degree(*it, g) == 2 && boost::out_degree(*it, g) == 0);
            break;
        default:
            ASSERT_TRUE(false);
        }
    }
    delete c;
    delete engine;
};

TEST(CFG, testBranching) {
    InstVec insts;
    Scumm::v6::Scummv6Engine *engine = new Scumm::v6::Scummv6Engine();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/branches.dmp");
    d->disassemble();

    ControlFlow *c = new ControlFlow(insts, *engine);
    Graph g = c->getGraph();
    ASSERT_TRUE(boost::num_vertices(g) == 4);
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        switch ((*gr->_start)->_address) {
        case 0:
            ASSERT_TRUE(boost::in_degree(*it, g) == 0 && boost::out_degree(*it, g) == 1);
            break;
        case 2:
            ASSERT_TRUE(boost::in_degree(*it, g) == 1 && boost::out_degree(*it, g) == 2);
            break;
        case 5:
            ASSERT_TRUE(boost::in_degree(*it, g) == 1 && boost::out_degree(*it, g) == 1);
            break;
        case 6:
            ASSERT_TRUE(boost::in_degree(*it, g) == 2 && boost::out_degree(*it, g) == 0);
            break;
        default:
            ASSERT_TRUE(false);
        }
    }
    delete c;
    delete engine;
}

TEST(CFG, testGrouping) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/branches.dmp");
    d->disassemble();

    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->getGraph();
    ASSERT_TRUE(boost::num_vertices(g) == 3);
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        switch ((*gr->_start)->_address) {
        case 0:
            ASSERT_TRUE((*gr->_end)->_address == 2);
            ASSERT_TRUE(boost::in_degree(*it, g) == 0 && boost::out_degree(*it, g) == 2);
            break;
        case 5:
            ASSERT_TRUE(boost::in_degree(*it, g) == 1 && boost::out_degree(*it, g) == 1);
            break;
        case 6:
            ASSERT_TRUE(boost::in_degree(*it, g) == 2 && boost::out_degree(*it, g) == 0);
            break;
        default:
            ASSERT_TRUE(false);
            break;
        }
    }
}

// TODO: Fix me, this fails but looks like it shouldn't
TEST(CFG, DISABLED_testShortCircuitDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/short-circuit.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->getGraph();
    ASSERT_TRUE(boost::num_vertices(g) == 3);
}

TEST(CFG, testWhileDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
    }

}

TEST(CFG, testDoWhileDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 3)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
    }
}

TEST(CFG, testBreakDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x14)
            ASSERT_TRUE(gr->_type == kBreakGroupType);
    }


    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-do-while.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0xA)
            ASSERT_TRUE(gr->_type == kBreakGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-do-while2.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0xD)
            ASSERT_TRUE(gr->_type == kBreakGroupType);
    }

}

TEST(CFG, testContinueDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x14)
            ASSERT_TRUE(gr->_type == kContinueGroupType);
        if ((*gr->_start)->_address == 0x1a)
            ASSERT_TRUE(gr->_type == kNormalGroupType);
    }
;

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-do-while.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0xA)
            ASSERT_TRUE(gr->_type == kContinueGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-do-while2.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0xD)
            ASSERT_TRUE(gr->_type == kContinueGroupType);
    }
}

TEST(CFG, testIfDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/if.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-do-while.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/break-do-while2.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x3)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
    }


    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-do-while.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/continue-do-while2.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x3)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
    }
}

TEST(CFG, testElseDetection) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/if-else.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x10) {
            ASSERT_TRUE(gr->_startElse);
            ASSERT_TRUE(gr->_endElse.size() == 1 && gr->_endElse[0] == gr);
        }
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/if-no-else.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
        ASSERT_TRUE(!gr->_startElse);
    }
}

TEST(CFG, testNestedLoops) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/do-while-in-while.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
        if ((*gr->_start)->_address == 0xd)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/nested-do-while.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x6)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
        if ((*gr->_start)->_address == 0x10)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/nested-while.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
        if ((*gr->_start)->_address == 0xa)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/nested-while2.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
        if ((*gr->_start)->_address == 0xd)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/while-in-do-while.dmp");
    d->disassemble();
    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x0)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
        if ((*gr->_start)->_address == 0x10)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
    }

    insts.clear();
    engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    d = engine->getDisassembler(insts);
    d->open("decompiler/test/while-in-do-while2.dmp");
    d->disassemble();

    c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    g = c->analyze();
    range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        if ((*gr->_start)->_address == 0x3)
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
        if ((*gr->_start)->_address == 0x13)
            ASSERT_TRUE(gr->_type == kDoWhileCondGroupType);
    }
}

// This test requires script-30.dmp from Sam & Max: Hit The Road.
// 6e48faca13e1f6df9341567608962744 *script-30.dmp
// Disabled as mentioned file is copyrighted
TEST(CFG, DISABLED_testSamAndMaxScript30) {
    InstVec insts;
    auto engine = std::make_unique<Scumm::v6::Scummv6Engine>();
    auto d = engine->getDisassembler(insts);
    d->open("decompiler/test/script-30.dmp");
    d->disassemble();
    auto c = std::make_unique<ControlFlow>(insts, *engine);
    c->createGroups();
    Graph g = c->analyze();
    VertexRange range = boost::vertices(g);
    for (VertexIterator it = range.first; it != range.second; ++it) {
        GroupPtr gr = GET(*it);
        switch ((*gr->_start)->_address) {
        case 0x6:
            ASSERT_TRUE(gr->_type == kWhileCondGroupType);
            ASSERT_TRUE(!gr->_startElse);
            ASSERT_TRUE(gr->_endElse.empty());
            break;
        case 0x19:
        case 0x3A:
        case 0x4F:
        case 0x68:
        case 0x74: // Allow inclusion of the pop instruction immediately before
        case 0x75:
        case 0x92:
            ASSERT_TRUE(gr->_type == kIfCondGroupType);
            ASSERT_TRUE(!gr->_startElse);
            ASSERT_TRUE(gr->_endElse.empty());
            break;
        case 0x8B:
            ASSERT_TRUE(gr->_type == kNormalGroupType);
            ASSERT_TRUE(gr->_startElse);
            ASSERT_TRUE(gr->_endElse.size() == 1 && (*gr->_endElse[0]->_start)->_address == 0x8B);
            break;
        case 0x91:
            ASSERT_TRUE(gr->_type == kNormalGroupType || gr->_type == kIfCondGroupType); // Allow inclusion of the pop instruction immediately before
            ASSERT_TRUE(gr->_startElse);
            ASSERT_TRUE(gr->_endElse.empty());
            break;
        case 0xA6:
            ASSERT_TRUE(gr->_type == kNormalGroupType);
            ASSERT_TRUE(!gr->_startElse);
            ASSERT_TRUE(gr->_endElse.size() == 1 && (*gr->_endElse[0]->_start)->_address == 0x91);
            break;
        default:
            ASSERT_TRUE(gr->_type == kNormalGroupType);
            ASSERT_TRUE(!gr->_startElse);
            ASSERT_TRUE(gr->_endElse.empty());
            break;
        }
    }
}
