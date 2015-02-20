#include "sudm.h"
#include "decompiler/ff7_field/ff7_field_engine.h"

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
                // TODO: Implement this
                ::FF7::FF7FieldEngine engine;

                throw ::NotImplementedException();
            }
        }
    }
}
