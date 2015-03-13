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

std::string FF7::FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(const std::string& entity, uint32 address, uint32 opcode)
{
    return (boost::format("log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: instruction 0x%3$04x not implemented\")") % entity % address % opcode).str();
}

std::string FF7::FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(const std::string& entity, uint32 address, const Instruction& instruction)
{
    std::stringstream parameterList;
    for (auto i = instruction._params.begin(); i != instruction._params.end(); ++i)
    {
        if (i != instruction._params.begin())
        {
            parameterList << ", ";
        }
        parameterList << *i;
    }
    return (boost::format("log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: instruction %3%( %4% ) not implemented\")") % entity % address % instruction._name % parameterList.str()).str();
}

std::string FF7::FF7CodeGeneratorHelpers::FormatBool(uint32 value)
{
    return value == 0 ? "false" : "true";
}

std::string FF7::FF7CodeGeneratorHelpers::FormatInvertedBool(uint32 value)
{
    return value == 0 ? "true" : "false";
}



