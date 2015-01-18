#include "ff7_world_codegen.h"
#include "ff7_world_engine.h"

std::string FF7::FF7WorldCodeGenerator::constructFuncSignature(const Function&)
{
    return "";
}

const InstPtr FF7::FF7WorldCodeGenerator::findFirstCall()
{
    ConstInstIterator it = mCurGroup->_start;
    do
    {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it++ != mCurGroup->_end);

    return *mCurGroup->_start;
}

const InstPtr FF7::FF7WorldCodeGenerator::findLastCall()
{
    ConstInstIterator it = mCurGroup->_end;
    do {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it-- != mCurGroup->_start);

    return *mCurGroup->_end;
}

void FF7::FF7WorldCodeGenerator::processSpecialMetadata(const InstPtr inst, char, int)
{

}
