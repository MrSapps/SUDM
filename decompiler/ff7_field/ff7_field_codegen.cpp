#include "ff7_field_codegen.h"
#include "ff7_field_engine.h"
#include <boost/algorithm/string/predicate.hpp>

void FF7::FF7CodeGenerator::onBeforeStartFunction(const Function& func)
{
    // Start class
    FunctionMetaData metaData(func._metadata);
    if (metaData.IsStart())
    {
        addOutputLine("EntityContainer[ \"" + metaData.EntityName() + "\" ] = {", false, true);
        if (metaData.CharacterId() != -1)
        {
            addOutputLine(metaData.EntityName() + " = nil,");
        }
        addOutputLine("");
    }
}

void FF7::FF7CodeGenerator::onStartFunction(const Function& func)
{
    addOutputLine("--[[");
    for (const auto& inst : mInsts)
    {
        if (inst->_address >= func.mStartAddr && inst->_address <= func.mEndAddr)
        {
            std::stringstream output;
            output <<inst;
            addOutputLine(output.str());
        }
    }
    addOutputLine("]]\n");
}

void FF7::FF7CodeGenerator::onEndFunction(const Function& func)
{
    // End function
    addOutputLine("end,", true, false);
    
    // End class
    FunctionMetaData metaData(func._metadata);
    if (metaData.IsEnd())
    {
        addOutputLine("}", true, false);
    }
}

std::string FF7::FF7CodeGenerator::constructFuncSignature(const Function &func)
{
    // Generate name
    return func._name + " = function( self )";
}
