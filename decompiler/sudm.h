#pragma once

#include <map>
#include <vector>
#include <string>
#include "unknown_opcode_exception.h"

namespace SUDM
{
    class IScriptFormatter
    {
    public:
        virtual ~IScriptFormatter() = default;

        // Renames a variable, return empty string for generated name
        virtual std::string VarName(unsigned int bank, unsigned int addr) = 0;

        // Renames an entity
        virtual std::string EntityName(const std::string& entity) = 0;

        // Renames an animation, return empty string for generated name
        virtual std::string AnimationName(int id) = 0;

        // Renames a function in an entity
        virtual std::string FunctionName(const std::string& entity, const std::string& funcName) = 0;

        // Sets the header comment for a function in an entity
        virtual std::string FunctionComment(const std::string& entity, const std::string& funcName) = 0;
    };

    namespace FF7
    {
        namespace Field
        {
            // Entities list, with entity type of
            // entity_script
            // entity_model, which somehow links to model loader
            struct DecompiledScript
            {
                std::string luaScript;
                std::map<std::string, int> entities;
            };

            /*
            * Throws ::InternalDecompilerError on failure.
            * scriptName - name of the script to be converted, should match file name.
            * scriptBytes - vector of raw byte data that makes up the script.
            * formatter - used to rename variables, drop functions etc.
            * textToAppend - raw text that is glued on to the end of the decompiled output.
            * textToPrepend - raw text that is glued to to the start of the decompiled output.
            * returns a string containing [textToPrepend] [decompiled script] [textToAppend]
            */
            DecompiledScript Decompile(std::string scriptName,
                                  const std::vector<unsigned char>& scriptBytes,
                                  IScriptFormatter& formatter, 
                                  std::string textToAppend = "", 
                                  std::string textToPrepend = "");
        }
    }
}