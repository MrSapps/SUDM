#include "sudm.h"

class DummyFormatter : public SUDM::IScriptFormatter
{
public:
    // Renames a variable, return empty string for generated name
    virtual std::string VarName(uint32, uint32) override
    {
        return "";
    }

    // Renames an entity
    virtual std::string EntityName(const std::string& entity) override
    {
        return entity;
    }

    // Renames an animation, return empty string for generated name
    virtual std::string AnimationName(int) override
    {
        return "";
    }

    // Renames a function in an entity
    virtual std::string FunctionName(const std::string&, const std::string& funcName) override
    {
        return funcName;
    }

    // Sets the header comment for a function in an entity
    virtual std::string FunctionComment(const std::string& , const std::string& ) override
    {
        return "";
    }
};
