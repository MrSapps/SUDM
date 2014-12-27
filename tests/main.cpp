#include <gmock/gmock.h>
#include "decompiler/ff7_field/disassembler.h"
#include "decompiler/ff7_field/engine.h"
#include "control_flow.h"
#include "util.h"
#include "graph.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

TEST(FF7, DisAsm)
{
    FF7::FF7Engine engine;


    InstVec insts;
    FF7::FF7Disassembler d(&engine, insts);
    d.open("decompiler/test/ff7.dat");
    d.disassemble();

    d.dumpDisassembly(std::cout);

    ControlFlow *c = new ControlFlow(insts, &engine);
    c->createGroups();




    Graph g = c->analyze();
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

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
