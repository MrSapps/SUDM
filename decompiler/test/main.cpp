#include <gmock/gmock.h>

#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"

#include "decompiler/ff7_world/ff7_world_disassembler.h"
#include "decompiler/ff7_world/ff7_world_engine.h"

#include "control_flow.h"
#include "util.h"
#include "graph.h"
#include "make_unique.h"
#include "sudm.h"
#include "lzs.h"
#include "ff7_field_dummy_formatter.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))


class TestReadParameterDisassembler : public SimpleDisassembler
{
public:
    TestReadParameterDisassembler(std::vector<unsigned char>&& data, InstVec& insts)
        : SimpleDisassembler(insts)
    {
        mStream = std::make_unique<BinaryReader>(std::move(data));
    }

    void readParams(InstPtr inst, const char *typeString)
    {
        return SimpleDisassembler::readParams(inst, typeString);
    }


    virtual void doDisassemble() throw(std::exception) override final
    {
        // NOP
    }

};

static InstPtr DoReadParameterTest(std::string str, std::vector<unsigned char> data)
{
    InstVec insts;
    TestReadParameterDisassembler d(std::move(data), insts);
    InstPtr inst = new FF7::FF7NoOperationInstruction();
    d.readParams(inst, str.c_str());
    return inst;
}

TEST(SimpleDisassembler, readParameter_U)
{
    std::vector<unsigned char> data = { 0xAA };
    InstPtr inst = DoReadParameterTest("U", data);

    ASSERT_EQ(inst->_params.size(), 2);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0x5);
    ASSERT_EQ(inst->_params[1]->getSigned(), 0xA);
}

TEST(SimpleDisassembler, readParameter_N)
{
    std::vector<unsigned char> data = { 0xAB };
    InstPtr inst = DoReadParameterTest("N", data);

    ASSERT_EQ(inst->_params.size(), 2);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0xA);
    ASSERT_EQ(inst->_params[1]->getSigned(), 0xB);
}

TEST(SimpleDisassembler, readParameter_b)
{
    std::vector<unsigned char> data = { (unsigned char)-100 };
    InstPtr inst = DoReadParameterTest("b", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), -100);
}

TEST(SimpleDisassembler, readParameter_B)
{
    std::vector<unsigned char> data = { 100 };
    InstPtr inst = DoReadParameterTest("B", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 100);
}

TEST(SimpleDisassembler, readParameter_s)
{
    std::vector<unsigned char> data = { (unsigned char)-40 };
    InstPtr inst = DoReadParameterTest("s", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 216); // TODO: Casting removes the sign
}

TEST(SimpleDisassembler, readParameter_w)
{
    std::vector<unsigned char> data = { 0xFE, 0xAA };
    InstPtr inst = DoReadParameterTest("w", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0xAAFE);
}

TEST(SimpleDisassembler, readParameter_i)
{
    std::vector<unsigned char> data = { (unsigned char)-0x30 };
    InstPtr inst = DoReadParameterTest("i", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 208); // TODO: Casting removes the sign
}

TEST(SimpleDisassembler, readParameter_d)
{
    std::vector<unsigned char> data = { 0xde, 0xad, 0xbe, 0xef };
    InstPtr inst = DoReadParameterTest("d", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getUnsigned(), 0xEFBEADDE);
}

TEST(FF7Field, FunctionMetaData_Parse_Empty)
{
    FF7::FunctionMetaData meta("");
    ASSERT_EQ("", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_Empties)
{
    FF7::FunctionMetaData meta("__________________");
    ASSERT_EQ("", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_Start)
{
    FF7::FunctionMetaData meta("start_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(true, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_End)
{
    FF7::FunctionMetaData meta("end_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_EntityName)
{
    FF7::FunctionMetaData meta("end_-1_TheName");
    ASSERT_EQ("TheName", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
    ASSERT_EQ(-1, meta.CharacterId());

}

TEST(FF7Field, FunctionMetaData_Parse_EntityNameAndId)
{
    FF7::FunctionMetaData meta("end_99_The_Name");
    ASSERT_EQ("The_Name", meta.EntityName());
    ASSERT_EQ(99, meta.CharacterId());

    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}


TEST(FF7Field, FunctionMetaData_Parse_StartEnd)
{
    FF7::FunctionMetaData meta("start_end_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(true, meta.IsStart());
}

TEST(FF7World, Asm)
{
    DummyFormatter dummy;
    FF7::FF7FieldEngine eng(dummy);

    InstVec insts;
    FF7::FF7Disassembler d(dummy, &eng, insts);

    d.Assemble("NOP");

}


TEST(FF7World, DISABLED_DisAsm)
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


        auto c = std::make_unique<ControlFlow>(insts, engine);
        c->createGroups();




        Graph g = c->analyze();

        engine.postCFG(insts, g);


        onullstream ns;
        auto cg = engine.getCodeGenerator(insts, std::cout);

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

        cg->generate(insts, g);

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
