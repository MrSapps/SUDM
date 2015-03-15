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
            output << inst;
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

const std::string FF7::FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(const std::string& entity, uint32 address, uint32 opcode)
{
    return (boost::format("-- log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: instruction 0x%3$04x not implemented\")") % entity % address % opcode).str();
}

const std::string FF7::FF7CodeGeneratorHelpers::FormatInstructionNotImplemented(const std::string& entity, uint32 address, const Instruction& instruction)
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
    return (boost::format("-- log:log(\"In entity \\\"%1%\\\", address 0x%2$08x: instruction %3%( %4% ) not implemented\")") % entity % address % instruction._name % parameterList.str()).str();
}

const std::string FF7::FF7CodeGeneratorHelpers::FormatBool(uint32 value)
{
    return value == 0 ? "false" : "true";
}

const std::string FF7::FF7CodeGeneratorHelpers::FormatInvertedBool(uint32 value)
{
    return value == 0 ? "true" : "false";
}

const std::unordered_map<uint32, const std::string> FF7::FF7CodeGeneratorHelpers::CharacterNamesById = {
    { 0, std::string("Cloud") },
    { 1, std::string("Barret") },
    { 2, std::string("Tifa") },
    { 3, std::string("Aeris") },
    { 4, std::string("Red XIII") },
    { 5, std::string("Yuffie") },
    { 6, std::string("Cait Sith") },
    { 7, std::string("Vincent") },
    { 8, std::string("Cid") },
    { 9, std::string("Young Cloud") },
    { 10, std::string("Sephiroth") },
    { 11, std::string("Chocobo") },
    { 254, std::string("") },
};

const std::unordered_map<uint32, const std::unordered_map<uint32, const std::string>> FF7::FF7CodeGeneratorHelpers::VariableNamesByBankAndAddress = {
    { 1, {
        { 3, std::string("love_point_aeris") },
        { 4, std::string("love_point_tifa") },
        { 5, std::string("love_point_yuffie") },
        { 6, std::string("love_point_barret") },
        { 20, std::string("timer_hours") },
        { 21, std::string("timer_minutes") },
        { 22, std::string("timer_seconds") },
        { 23, std::string("timer_frames") },
        { 36, std::string("graveyard_item") },
        { 48, std::string("item_mask2") },
        { 80, std::string("battle_love_aeris") },
        { 81, std::string("battle_love_tifa") },
        { 82, std::string("battle_love_yuffie") },
        { 83, std::string("battle_love_barret") },
        { 164, std::string("graveyard_train") },
        { 225, std::string("act1_1_flags1") },
        { 226, std::string("act1_1_flags2") },
    } },
    { 2, {
        { 0, std::string("progress_game") },
        { 28, std::string("menu_appear") },
        { 30, std::string("menu_selectable") },
    } },
    { 3, {
        { 66, std::string("act1_1_flags3") },
        { 111, std::string("flower_flag") },
        { 112, std::string("act1_3_flags5") },
        { 126, std::string("act1_3_unknown") },
        { 127, std::string("tunnel_room") },
        { 128, std::string("act1_3_flags1") },
        { 129, std::string("act1_3_flags2") },
        { 130, std::string("act1_3_flags3") },
        { 131, std::string("act1_3_flags4") },
        { 208, std::string("act1_2_flags1") },
        { 209, std::string("act1_2_flags2") },
        { 210, std::string("act1_2_flags3") },
        { 211, std::string("act1_2_flags4") },
        { 212, std::string("act1_2_flags5") },
        { 213, std::string("act1_2_flags6") },
        { 214, std::string("act1_2_flags7") },
        { 215, std::string("act1_2_flags8") },
        { 216, std::string("act1_2_flags9") },
        { 217, std::string("act1_7_flags1") },
        { 223, std::string("act1_1_flags4") },
    } },
    { 13, {
        { 30, std::string("pointer") },
        { 31, std::string("materia_full") },
        { 91, std::string("save_flag") },
    } },
    { 15, {
        { 32, std::string("sector1_item") },
        { 33, std::string("sector5_item") },
        { 80, std::string("subway_item") },
    } }
};

const std::string FF7::FF7CodeGeneratorHelpers::GetCharacterNameById(uint32 value)
{
    auto i = CharacterNamesById.find(value);
    if (i == CharacterNamesById.end())
    {
        return "";
    }
    return i->second;
}

const std::string FF7::FF7CodeGeneratorHelpers::GetVariableNameByBankAndAddress(uint32 bank, uint32 address)
{
    auto i = VariableNamesByBankAndAddress.find(bank);
    if (i == VariableNamesByBankAndAddress.end())
    {
        return "";
    }
    auto j = i->second.find(address);
    if (j == i->second.end())
    {
        return "";
    }
    return j->second;
}

