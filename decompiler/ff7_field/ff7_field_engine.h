#pragma once

#include "decompiler/decompiler_engine.h"
#include <string>
#include <vector>
#include "sudm.h"

namespace FF7
{
    class FF7FieldEngine : public Engine
    {
    public:
        class Entity
        {
        public:
            Entity() = default;

            Entity(const std::string& name)
                : mName(name)
            {

            }

            std::string Name() const
            {
                return mName;
            }

            std::string FunctionByIndex(size_t index) const
            {
                auto it = mFunctions.find(index);
                if (it == std::end(mFunctions))
                {
                    throw InternalDecompilerError();
                }
                return it->second;
            }

            void AddFunction(const std::string& funcName, size_t funcIndex)
            {
                mFunctions[funcIndex] = funcName;
            }

        private:
            std::string mName;
            std::map< size_t, std::string > mFunctions;
        };

        FF7FieldEngine(const FF7FieldEngine&) = delete;
        FF7FieldEngine& operator = (const FF7FieldEngine&) = delete;

        FF7FieldEngine(SUDM::IScriptFormatter& formatter)
            : mFormatter(formatter)
        {
            setOutputStackEffect(false);
        }
        virtual std::unique_ptr<Disassembler> getDisassembler(InstVec &insts, const std::vector<unsigned char>& rawScriptData) override;
        virtual std::unique_ptr<Disassembler> getDisassembler(InstVec &insts) override;
        virtual std::unique_ptr<CodeGenerator> getCodeGenerator(const InstVec& insts, std::ostream &output) override;
        virtual void postCFG(InstVec &insts, Graph g) override;
        virtual bool usePureGrouping() const override { return false; }
        std::map<std::string, int> GetEntities() const;
        void AddEntityFunction(const std::string& entityName, size_t entityIndex, const std::string& funcName, size_t funcIndex);

        const Entity& EntityByIndex(size_t index) const
        {
            auto it = mEntityIndexMap.find(index);
            if (it == std::end(mEntityIndexMap))
            {
                throw InternalDecompilerError();
            }
            return it->second;
        }

    private:
        void RemoveExtraneousReturnStatements(InstVec& insts, Graph g);
        void RemoveTrailingInfiniteLoops(InstVec& insts, Graph g);
        void MarkInfiniteLoopGroups(InstVec& insts, Graph g);
        SUDM::IScriptFormatter& mFormatter;
        std::map<size_t, Entity> mEntityIndexMap;
    };

    class FF7UncondJumpInstruction : public UncondJumpInstruction
    {
    public:
        bool _isCall;  ///< Whether or not this is really a call to a script function.
        FF7UncondJumpInstruction() : _isCall(false) {}
        virtual bool isFuncCall() const;
        virtual bool isUncondJump() const;
        virtual uint32 getDestAddress() const;
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
        virtual std::ostream& print(std::ostream &output) const override;
    };

    class FF7CondJumpInstruction : public CondJumpInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
        virtual uint32 getDestAddress() const override;
        virtual std::ostream& print(std::ostream &output) const override;
    };

    class FF7ControlFlowInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processWAIT(CodeGenerator* codeGen);
    };

    class FF7ModuleInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7MathInstruction : public StoreInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processSETBYTE_SETWORD(CodeGenerator* codeGen);
    };

    class FF7WindowInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processMPNAM(CodeGenerator* codeGen);
    };

    class FF7PartyInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7ModelInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processTLKON(CodeGenerator* codeGen, const std::string& entity);
        void processPC(CodeGenerator* codeGen, const std::string& entity);
        void processCHAR(CodeGenerator* codeGen, const std::string& entity);
        void processDFANM(CodeGenerator* codeGen, const std::string& entity);
        void processANIME1(CodeGenerator* codeGen, const std::string& entity);
        void processVISI(CodeGenerator* codeGen, const std::string& entity);
        void processXYZI(CodeGenerator* codeGen, const std::string& entity);
        void processMOVE(CodeGenerator* codeGen, const std::string& entity);
        void processGETAI(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processMSPED(CodeGenerator* codeGen, const std::string& entity);
        void processDIR(CodeGenerator* codeGen, const std::string& entity);
        void processSOLID(CodeGenerator* codeGen, const std::string& entity);
    };

    class FF7WalkmeshInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7BackgroundInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processADPAL(CodeGenerator* codeGen);
        void processSTPLS(CodeGenerator* codeGen);
        void processLDPLS(CodeGenerator* codeGen);
    };

    class FF7CameraInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7AudioVideoInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processMUSIC(CodeGenerator* codeGen);
    };

    class FF7UncategorizedInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7NoOperationInstruction : public Instruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };
}
