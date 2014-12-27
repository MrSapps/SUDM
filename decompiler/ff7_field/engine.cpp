#include "engine.h"
#include "disassembler.h"
#include "codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

Disassembler* FF7::FF7Engine::getDisassembler(InstVec &insts)
{
    return new FF7Disassembler(this, insts);
}

CodeGenerator* FF7::FF7Engine::getCodeGenerator(std::ostream &output)
{
    return new FF7CodeGenerator(this, output);
}

void FF7::FF7Engine::postCFG(InstVec &insts, Graph g)
{

}

bool FF7::FF7Engine::detectMoreFuncs() const
{
	return false;
}

void FF7::FF7Engine::getVariants(std::vector<std::string> &variants) const
{

}

void FF7::FF7LoadInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7StoreInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7StackInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7CondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{
    stack.push(new VarValue("foo"));
}

uint32 FF7::FF7CondJumpInstruction::getDestAddress() const
{
    return _address + _params[4]->getUnsigned() + 3; // TODO: Should actually be +4 or 5?
}

bool FF7::FF7UncondJumpInstruction::isFuncCall() const
{
	return _isCall;
}

bool FF7::FF7UncondJumpInstruction::isUncondJump() const
{
	return !_isCall;
}

uint32 FF7::FF7UncondJumpInstruction::getDestAddress() const
{
	return _address - _params[0]->getUnsigned();
}

void FF7::FF7UncondJumpInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7KernelCallInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}

void FF7::FF7NoOutputInstruction::processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
{

}
