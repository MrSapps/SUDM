#pragma  once

#include "decompiler_codegen.h"

namespace FF7
{
    class FF7WorldCodeGenerator : public CodeGenerator
    {
    public:
        FF7WorldCodeGenerator(Engine *engine, std::ostream &output)
            : CodeGenerator(engine, output, kFIFOArgOrder, kLIFOArgOrder)
        {

        }
        const InstPtr findFirstCall();
        const InstPtr findLastCall();
        virtual void processSpecialMetadata(const InstPtr inst, char c, int pos);
    protected:
        std::string constructFuncSignature(const Function &func);
    };
}
