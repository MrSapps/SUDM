#pragma once

#include "decompiler/decompiler_engine.h"
#include <string>
#include <vector>

namespace FF7
{
    class FF7WorldEngine : public Engine
    {
    public:
        FF7WorldEngine(int scriptNumber)
            : mScriptNumber(scriptNumber)
        {
            setOutputStackEffect(true);
        }
        Disassembler* getDisassembler(InstVec &insts);
        CodeGenerator* getCodeGenerator(std::ostream &output);
        void postCFG(InstVec &insts, Graph g);
        bool detectMoreFuncs() const;
        void getVariants(std::vector<std::string> &variants) const;
        virtual bool usePureGrouping() const { return false; }
        std::vector<std::string> _textStrings; ///< Container for strings from the TEXT chunk.
    private:
        int mScriptNumber;
    };

    class FF7WorldLoadBankInstruction : public LoadInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class BankValue : public VarValue
    {
    public:
        BankValue(std::string varName) : VarValue(varName) { }
    };

    class FF7WorldLoadInstruction : public LoadInstruction 
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class FF7SubStackInstruction : public StackInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class BinaryEqualStackInstruction : public BinaryOpStackInstruction 
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen)
        {
            _codeGenData = "==";
            BinaryOpStackInstruction::processInst(stack, engine, codeGen);
        }
    };

    class FF7WorldStoreInstruction : public StoreInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };


    class FF7WorldStackInstruction : public StackInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class FF7WorldCondJumpInstruction : public CondJumpInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
        virtual uint32 getDestAddress() const;
        virtual std::ostream& print(std::ostream &output) const override;
    };

    class FF7WorldUncondJumpInstruction : public UncondJumpInstruction
    {
    public:
        bool _isCall;  ///< Whether or not this is really a call to a script function.
        FF7WorldUncondJumpInstruction() : _isCall(false) { }
        virtual bool isFuncCall() const;
        virtual bool isUncondJump() const;
        virtual uint32 getDestAddress() const;
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
        virtual std::ostream& print(std::ostream &output) const override;

    };


    class FF7WorldKernelCallInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

    class FF7WorldNoOutputInstruction : public Instruction
    {
    public:
        virtual void processInst(ValueStack &stack, Engine *engine, CodeGenerator *codeGen);
    };

}
