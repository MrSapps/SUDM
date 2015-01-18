#include "ff7_field_codegen.h"
#include "ff7_field_engine.h"
#include <boost/algorithm/string/predicate.hpp>

void FF7::FF7CodeGenerator::onBeforeStartFunction(const Function& func)
{
    FunctionMetaData metaData(func._metadata);
    if (metaData.IsStart())
    {
        addOutputLine("class " + metaData.EntityName() + " {", false, true);
    }
}

void FF7::FF7CodeGenerator::onEndFunction(const Function& func)
{
    addOutputLine("}", true, false);

    FunctionMetaData metaData(func._metadata);
    if (metaData.IsEnd())
    {
        addOutputLine("};", true, false);
    }
}


std::string FF7::FF7CodeGenerator::constructFuncSignature(const Function &func)
{
    return "void " + func._name + "() { ";
}
