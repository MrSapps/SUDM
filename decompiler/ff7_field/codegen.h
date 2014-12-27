#pragma  once

#include "../codegen.h"

namespace FF7
{
    class FF7CodeGenerator : public CodeGenerator
    {
    public:
        FF7CodeGenerator(Engine *engine, std::ostream &output) 
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
