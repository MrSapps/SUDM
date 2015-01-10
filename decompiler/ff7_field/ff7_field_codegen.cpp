#include "ff7_field_codegen.h"
#include "ff7_field_engine.h"

std::string FF7::FF7CodeGenerator::constructFuncSignature(const Function &func)
{
    return "void " + func._name + "() { ";
}

const InstPtr FF7::FF7CodeGenerator::findFirstCall()
{
    ConstInstIterator it = _curGroup->_start;
    do
    {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it++ != _curGroup->_end);

    return *_curGroup->_start;
}

const InstPtr FF7::FF7CodeGenerator::findLastCall()
{
    ConstInstIterator it = _curGroup->_end;
    do {
        if ((*it)->isFuncCall() || (*it)->isKernelCall())
            return *it;
    } while (it-- != _curGroup->_start);

    return *_curGroup->_end;
}

void FF7::FF7CodeGenerator::processSpecialMetadata(const InstPtr inst, char c, int pos)
{

}
