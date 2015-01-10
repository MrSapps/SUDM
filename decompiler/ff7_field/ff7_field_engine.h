#pragma once

#include "decompiler/decompiler_engine.h"
#include <string>
#include <vector>

namespace FF7
{
    class FF7Engine : public Engine
    {
    public:
        FF7Engine()
        {
            setOutputStackEffect(false);
        }
        Disassembler* getDisassembler(InstVec &insts);
        CodeGenerator* getCodeGenerator(std::ostream &output);
        void postCFG(InstVec &insts, Graph g);
        bool detectMoreFuncs() const;
        void getVariants(std::vector<std::string> &variants) const;
        virtual bool usePureGrouping() const { return false; }
        std::vector<std::string> _textStrings; ///< Container for strings from the TEXT chunk.
    };


    class FF7LoadInstruction : public LoadInstruction 
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };


    class FF7StoreInstruction : public StoreInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };


    class FF7StackInstruction : public StackInstruction 
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class FF7CondJumpInstruction : public CondJumpInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
        virtual uint32 getDestAddress() const;
        virtual std::ostream& print(std::ostream &output) const override;
    };

    class FF7UncondJumpInstruction : public UncondJumpInstruction
    {
    public:
        bool _isCall;  ///< Whether or not this is really a call to a script function.
        FF7UncondJumpInstruction() : _isCall(false) { }
        virtual bool isFuncCall() const;
        virtual bool isUncondJump() const;
        virtual uint32 getDestAddress() const;
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
        virtual std::ostream& print(std::ostream &output) const override;

    };


    class FF7KernelCallInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class FF7NoOutputInstruction : public Instruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

}
