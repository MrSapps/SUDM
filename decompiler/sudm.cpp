#include "sudm.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"
#include "decompiler/control_flow.h"

namespace SUDM
{
    namespace FF7
    {
        namespace Field
        {
            std::string Decompile(std::string scriptName, 
                                  const std::vector<unsigned char>& scriptBytes,
                                  IScriptFormatter& formatter, 
                                  std::string textToAppend,
                                  std::string textToPrepend)
            {
                // Disassemble the script
                ::FF7::FF7FieldEngine engine;
                InstVec insts;

                auto disassembler = engine.getDisassembler(insts, scriptBytes);
                disassembler->disassemble();

                // Create CFG
                auto controlFlow = std::make_unique<ControlFlow>(insts, engine);
                controlFlow->createGroups();

                // Decompile/analyze
                Graph graph = controlFlow->analyze();
                engine.postCFG(insts, graph);

                // Generate code and return it
                std::stringstream output;
                auto cg = engine.getCodeGenerator(output);
                cg->generate(insts, graph);               
                return textToAppend + output.str() + textToPrepend;
            }
        }
    }
}
