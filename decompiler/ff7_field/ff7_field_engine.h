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
        unsigned int ScaleFactor() const { return mScaleFactor; }
    private:
        void RemoveExtraneousReturnStatements(InstVec& insts, Graph g);
        void RemoveTrailingInfiniteLoops(InstVec& insts, Graph g);
        void MarkInfiniteLoopGroups(InstVec& insts, Graph g);
        SUDM::IScriptFormatter& mFormatter;
        std::map<size_t, Entity> mEntityIndexMap;
        unsigned int mScaleFactor = 1;
        friend class FF7Disassembler;
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
        static InstPtr Create() { return new FF7ControlFlowInstruction(); }
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processREQ(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processREQSW(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processREQEW(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processRETTO(CodeGenerator* codeGen);
        void processWAIT(CodeGenerator* codeGen);
    };

    class FF7ModuleInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processBATTLE(CodeGenerator* codeGen);
        void processBTLON(CodeGenerator* codeGen);
    };

    class FF7MathInstruction : public StoreInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processSaturatedPLUS(CodeGenerator* codeGen);
        void processSaturatedPLUS2(CodeGenerator* codeGen);
        void processSaturatedMINUS(CodeGenerator* codeGen);
        void processSaturatedMINUS2(CodeGenerator* codeGen);
        void processSaturatedINC(CodeGenerator* codeGen);
        void processSaturatedINC2(CodeGenerator* codeGen);
        void processSaturatedDEC(CodeGenerator* codeGen);
        void processSaturatedDEC2(CodeGenerator* codeGen);
        void processRDMSD(CodeGenerator* codeGen);
        void processSETBYTE_SETWORD(CodeGenerator* codeGen);
        void processBITON(CodeGenerator* codeGen);
        void processPLUSx_MINUSx(CodeGenerator* codeGen, const std::string& op);
        void processINCx_DECx(CodeGenerator* codeGen, const std::string& op);
        void processRANDOM(CodeGenerator* codeGen);
    };

    class FF7WindowInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processMESSAGE(CodeGenerator* codeGen);
        void processMPNAM(CodeGenerator* codeGen);
        void processMENU2(CodeGenerator* codeGen);
        void processWINDOW(CodeGenerator* codeGen);
    };

    class FF7PartyInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processSTITM(CodeGenerator* codeGen);
        void processPRTYE(CodeGenerator* codeGen);
    };

    class FF7ModelInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processTLKON(CodeGenerator* codeGen, const std::string& entity);
        void processPC(CodeGenerator* codeGen, const std::string& entity);
        void processCHAR(CodeGenerator* codeGen, const std::string& entity);
        void processDFANM(CodeGenerator* codeGen, const std::string& entity, int charId);
        void processANIME1(CodeGenerator* codeGen, const std::string& entity, int charId);
        void processVISI(CodeGenerator* codeGen, const std::string& entity);
        void processXYZI(CodeGenerator* codeGen, const std::string& entity);
        void processMOVE(CodeGenerator* codeGen, const std::string& entity);
        void processMSPED(CodeGenerator* codeGen, const std::string& entity);
        void processDIR(CodeGenerator* codeGen, const std::string& entity);
        void processTURNGEN(CodeGenerator* codeGen, const std::string& entity);
        void processGETAI(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processANIM_2(CodeGenerator* codeGen, const std::string& entity, int charId);
        void processCANIM2(CodeGenerator* codeGen, const std::string& entity, int charId);
        void processCANM_2(CodeGenerator* codeGen, const std::string& entity, int charId);
        void processCC(CodeGenerator* codeGen, const FF7FieldEngine& engine);
        void processSOLID(CodeGenerator* codeGen, const std::string& entity);
    };

    class FF7WalkmeshInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processUC(CodeGenerator* codeGen);
    };

    class FF7BackgroundInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processBGON(CodeGenerator* codeGen);
        void processBGOFF(CodeGenerator* codeGen);
        void processBGCLR(CodeGenerator* codeGen);
        void processSTPAL(CodeGenerator* codeGen);
        void processLDPAL(CodeGenerator* codeGen);
        void processCPPAL(CodeGenerator* codeGen);
        void processADPAL(CodeGenerator* codeGen);
        void processMPPAL2(CodeGenerator* codeGen);
        void processSTPLS(CodeGenerator* codeGen);
        void processLDPLS(CodeGenerator* codeGen);
    };

    class FF7CameraInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processNFADE(CodeGenerator* codeGen);
        void processSCR2D(CodeGenerator* codeGen);
        void processSCR2DC(CodeGenerator* codeGen);
        void processFADE(CodeGenerator* codeGen);
    };

    class FF7AudioVideoInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    private:
        void processAKAO2(CodeGenerator* codeGen);
        void processMUSIC(CodeGenerator* codeGen);
        void processSOUND(CodeGenerator* codeGen);
        void processAKAO(CodeGenerator* codeGen);
        void processMULCK(CodeGenerator* codeGen);
        void processPMVIE(CodeGenerator* codeGen);
        void processMOVIE(CodeGenerator* codeGen);
        void processMVIEF(CodeGenerator* codeGen);
    };

    class FF7UncategorizedInstruction : public KernelCallInstruction
    {
    public:
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };

    class FF7NoOperationInstruction : public Instruction
    {
    public:
        static InstPtr Create() { return new FF7NoOperationInstruction(); }
        virtual void processInst(Function& func, ValueStack &stack, Engine *engine, CodeGenerator *codeGen) override;
    };
}
