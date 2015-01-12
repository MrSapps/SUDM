#include <gmock/gmock.h>

#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"

#include "decompiler/ff7_world/ff7_world_disassembler.h"
#include "decompiler/ff7_world/ff7_world_engine.h"

#include "control_flow.h"
#include "util.h"
#include "graph.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

TEST(FF7Field, DisAsm)
{
    FF7::FF7Engine engine;


    InstVec insts;
    FF7::FF7Disassembler d(&engine, insts);
    d.open("decompiler/test/md1_2.dat");
    d.disassemble();

    d.dumpDisassembly(std::cout);
    std::cout << std::endl;

    ControlFlow *c = new ControlFlow(insts, &engine);
    c->createGroups();




    Graph g = c->analyze();

    engine.postCFG(insts, g);


    onullstream ns;
    CodeGenerator *cg = engine.getCodeGenerator(std::cout);

    std::ofstream out;
    out.open("graph.dot");
    if (out.is_open())
    {
        auto& g = c->getGraph();
        boost::write_graphviz(
            out, g, boost::make_label_writer(get(boost::vertex_name, g)), 
            boost::makeArrowheadWriter(get(boost::edge_attribute, g)), GraphProperties(&engine, g));
    }
    out.close();

    cg->generate(g);

    VertexIterator v = boost::vertices(g).first;
    GroupPtr gr = GET(*v);

    // Find first node
    while (gr->_prev != NULL)
    {
        gr = gr->_prev;
    }

    // Copy out all lines of code
    std::vector<std::string> output;
    while (gr != NULL) 
    {
        for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
        {
            output.push_back(it->_line);
        }
        gr = gr->_next;
    }

    ASSERT_TRUE(output.empty() == false);
}

TEST(FF7World, DisAsm)
{
    for (int i = 0; i < 256; i++)
    {
        FF7::FF7WorldEngine engine(i);


        InstVec insts;
        std::cout << std::endl;

        std::cout << std::endl;

        std::cout << "SCRIPT(" << i << ")" << std::endl;


        auto d = engine.getDisassembler(insts);
        d->open("decompiler/ff7_world/wm0.ev");
        d->disassemble();
        d->dumpDisassembly(std::cout);
        std::cout << std::endl;


        ControlFlow *c = new ControlFlow(insts, &engine);
        c->createGroups();




        Graph g = c->analyze();

        engine.postCFG(insts, g);


        onullstream ns;
        CodeGenerator *cg = engine.getCodeGenerator(std::cout);

        std::ofstream out;
        out.open("graph.dot");
        if (out.is_open())
        {
            auto& g = c->getGraph();
            boost::write_graphviz(
                out, g, boost::make_label_writer(get(boost::vertex_name, g)),
                boost::makeArrowheadWriter(get(boost::edge_attribute, g)), GraphProperties(&engine, g));
        }
        out.close();

        cg->generate(g);

        VertexIterator v = boost::vertices(g).first;
        GroupPtr gr = GET(*v);

        // Find first node
        while (gr->_prev != NULL)
        {
            gr = gr->_prev;
        }

        // Copy out all lines of code
        std::vector<std::string> output;
        while (gr != NULL)
        {
            for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
            {
                output.push_back(it->_line);
            }
            gr = gr->_next;
        }

        ASSERT_TRUE(output.empty() == false);
    }
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}