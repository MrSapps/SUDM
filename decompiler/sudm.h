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

        virtual void AddSpawnPoint(unsigned int /*targetMapId*/, const std::string& /*entity*/, const std::string& /*funcName*/, unsigned int /*address*/, int /*x*/, int /*y*/, int /*triangleId*/, int /*angle*/)
        {

        }

        virtual std::string SpawnPointName(unsigned int targetMapId, const std::string& entity, const std::string& funcName, unsigned int address)
        {
            return MapName(targetMapId) + "_" + entity + "_" + funcName + "_" + std::to_string(address);
        }

        virtual std::string MapName(unsigned int mapId) { return std::to_string(mapId); }

        // Renames a variable, return empty string for generated name, can return empty
        virtual std::string VarName(unsigned int /*bank*/, unsigned int /*addr*/) { return ""; }

        // Renames an entity, can't return empty
        virtual std::string EntityName(const std::string& entity) { return entity; }

        // Names an animation, can't return empty
        virtual std::string AnimationName(int /*charId*/, int id)  { return std::to_string(id); }

        // Get name of char from its id, can't return empty
        virtual std::string CharName(int charId) { return std::to_string(charId);  }

        // Renames a function in an entity, can't return empty
        virtual std::string FunctionName(const std::string& /*entity*/, const std::string& funcName) { return funcName; }

        // Sets the header comment for a function in an entity, can return empty
        virtual std::string FunctionComment(const std::string& /*entity*/, const std::string& /*funcName*/)  { return ""; }
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

            float ScaleFactor(const std::vector<unsigned char>& scriptBytes);

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