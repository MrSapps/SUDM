#pragma once

#include <vector>
#include <string>
#include "unknown_opcode_exception.h"

namespace SUDM
{
    class IScriptFormatter
    {
    public:
        virtual ~IScriptFormatter() = default;

        // Return false to drop this function from the decompiled output
        virtual bool ExcludeFunction(const std::string& functionName) = 0;

        // Used to rename any identifier
        virtual std::string RenameIdentifer(const std::string& name) = 0;
    };

    namespace FF7
    {
        namespace Field
        {
            /*
            * Throws ::InternalDecompilerError on failure.
            * scriptName - name of the script to be converted, should match file name.
            * scriptBytes - vector of raw byte data that makes up the script.
            * formatter - used to rename variables, drop functions etc.
            * textToAppend - raw text that is glued on to the end of the decompiled output.
            * textToPrepend - raw text that is glued to to the start of the decompiled output.
            * returns a string containing [textToPrepend] [decompiled script] [textToAppend]
            */
            std::string Decompile(std::string scriptName, 
                                  const std::vector<unsigned char>& scriptBytes,
                                  IScriptFormatter& formatter, 
                                  std::string textToAppend = "", 
                                  std::string textToPrepend = "");
        }
    }
}