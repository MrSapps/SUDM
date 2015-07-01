#include "sudm.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "decompiler/control_flow.h"

namespace SUDM
{
    namespace FF7
    {
        namespace Animation
        {
            // TODO
        }

        namespace AI
        {
            // TODO
        }

        namespace World
        {
            // TODO
        }

        namespace Field
        {
            float ScaleFactor(const std::vector<unsigned char>& scriptBytes)
            {
                // Could be cleaner, but just does enough work to pull out the fields scale
                IScriptFormatter formatter;
                ::FF7::FF7FieldEngine engine(formatter, "Unused");
                InstVec insts;
                engine.getDisassembler(insts, scriptBytes);
                return engine.ScaleFactor();
            }

            DecompiledScript Decompile(std::string scriptName,
                                  const std::vector<unsigned char>& scriptBytes,
                                  IScriptFormatter& formatter, 
                                  std::string textToAppend,
                                  std::string textToPrepend)
            {
                // Disassemble the script
                ::FF7::FF7FieldEngine engine(formatter, scriptName);
                InstVec insts;

                auto disassembler = engine.getDisassembler(insts, scriptBytes);
                disassembler->disassemble();

                //disassembler->dumpDisassembly(std::cout);

                // Create CFG
                auto controlFlow = std::make_unique<ControlFlow>(insts, engine);
                controlFlow->createGroups();

                // Decompile/analyze
                //Graph graph = controlFlow->analyze();
                //engine.postCFG(insts, graph);
                Graph graph;

                DecompiledScript ds;

                // Generate code and return it
                std::stringstream output;
                auto cg = engine.getCodeGenerator(insts, output);
                cg->generate(insts, graph);
                ds.luaScript = textToPrepend + output.str() + textToAppend;
                ds.entities = engine.GetEntities();
                return ds;
            }
        }
    }

    namespace FF8
    {
        namespace Field
        {
            // TODO
        }
    }

    namespace FF9
    {
        namespace Field
        {
            // TODO
        }
    }
}
