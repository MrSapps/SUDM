#include "codegen.h"
#include "engine.h"

std::string FF7::FF7WorldCodeGenerator::constructFuncSignature(const Function &func)
{
    return "";
}

const InstPtr FF7::FF7WorldCodeGenerator::findFirstCall()
{
    ConstInstIterator it = _curGroup->_start;
    do
    {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it++ != _curGroup->_end);

    return *_curGroup->_start;
}

const InstPtr FF7::FF7WorldCodeGenerator::findLastCall()
{
    ConstInstIterator it = _curGroup->_end;
    do {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it-- != _curGroup->_start);

    return *_curGroup->_end;
}

void FF7::FF7WorldCodeGenerator::processSpecialMetadata(const InstPtr inst, char c, int pos)
{

}
