#pragma  once

#include "decompiler_codegen.h"
#include <boost/algorithm/string.hpp>
#include <deque>
#include <unordered_map>
#include "make_unique.h"
#include "sudm.h"

namespace FF7
{
    class FunctionMetaData
    {
    public:
        // Meta data format can be:
        // start_end_entityname
        // start_entityname
        // end_entity_name
        FunctionMetaData(std::string metaData)
        {
            Parse(metaData);
        }

        bool IsStart() const
        {
            return mStart;
        }

        bool IsEnd() const
        {
            return mEnd;
        }

        std::string EntityName()
        {
            return mEntityName;
        }

        int CharacterId()
        {
            return mCharacterId;
        }

    private:
        void Parse(std::string str)
        {
            std::deque<std::string> strs;
            boost::split(strs, str, boost::is_any_of("_"), boost::token_compress_on);
            if (!strs.empty())
            {
                auto tmp = strs.front();
                strs.pop_front();
                ParseStart(tmp, strs);
            }
        }

        void ParseStart(const std::string& item, std::deque<std::string>& strs)
        {
            if (item == "start")
            {
                mStart = true;
                if (!strs.empty())
                {
                    auto tmp = strs.front();
                    strs.pop_front();
                    ParseEnd(tmp, strs);
                }
            }
            else
            {
                ParseEnd(item, strs);
            }
        }

        void ParseEnd(const std::string& item, std::deque<std::string>& strs)
        {
            if (item == "end")
            {
                mEnd = true;
                if (!strs.empty())
                {
                    auto tmp = strs.front();
                    strs.pop_front();
                    ParseCharId(tmp, strs);
                }
            }
            else
            {
                ParseCharId(item, strs);
            }
        }

        void ParseCharId(const std::string& item, std::deque<std::string>& strs)
        {
            if (!item.empty())
            {
                mCharacterId = std::stoi(item);
            }
            if (!strs.empty())
            {
                auto tmp = strs.front();
                strs.pop_front();
                ParseEntity(tmp, strs);
            }
        }

        void ParseEntity(const std::string& item, std::deque<std::string>& strs)
        {
            mEntityName = item;
            for (auto& part : strs)
            {
                if (!part.empty())
                {
                    mEntityName += "_" + part;
                }
            }
        }

        bool mEnd = false;
        bool mStart = false;
        std::string mEntityName;
        int mCharacterId = -1;
    };

    class FF7CodeGenerator : public CodeGenerator
    {
    public:
        FF7CodeGenerator(Engine *engine, const InstVec& insts, std::ostream &output)
            : CodeGenerator(engine, output, kFIFOArgOrder, kLIFOArgOrder),
              mInsts(insts)
        {
            mTargetLang = std::make_unique<LuaTargetLanguage>();
        }
    protected:
        virtual std::string constructFuncSignature(const Function &func) override;
        virtual void onEndFunction(const Function& func) override;
        virtual void onBeforeStartFunction(const Function& func) override;
        virtual void onStartFunction(const Function& func) override;
        virtual bool OutputOnlyRequiredLabels() const override { return true; }
    private:
        const InstVec& mInsts;
    };


    class FF7SimpleCodeGenerator : public CodeGenerator
    {
    public:
        FF7SimpleCodeGenerator(Engine *engine, const InstVec& insts, std::ostream &output, SUDM::IScriptFormatter& formatter)
            : CodeGenerator(engine, output, kFIFOArgOrder, kLIFOArgOrder),
            mInsts(insts), mFormatter(formatter)
        {
            mTargetLang = std::make_unique<LuaTargetLanguage>();
        }
        virtual void generate(InstVec& insts, const Graph &g) override;
        virtual void addOutputLine(std::string s, bool unindentBefore = false, bool indentAfter = false) override;
        float ScaleFactor() const;
    protected:
        virtual std::string constructFuncSignature(const Function &func) override;
        virtual void onEndFunction(const Function& func) override;
        virtual void onBeforeStartFunction(const Function& func) override;
        virtual void onStartFunction(const Function& func) override;
        virtual bool OutputOnlyRequiredLabels() const override { return true; }
    private:
        const InstVec& mInsts;
        std::vector<CodeLine> mLines;
    public:
        SUDM::IScriptFormatter& mFormatter;
    };

    namespace FF7CodeGeneratorHelpers
    {
        enum struct ValueType : int
        {
            Integer = 0,
            Float = 1
        };

        const std::string FormatInstructionNotImplemented(const std::string& entity, uint32 address, uint32 opcode);
        const std::string FormatInstructionNotImplemented(const std::string& entity, uint32 address, const Instruction& instruction);
        const std::string FormatBool(uint32 value);
        const std::string FormatInvertedBool(uint32 value);
 
        template<typename TValue>
        const std::string FormatValueOrVariable(SUDM::IScriptFormatter& formatter, uint32 bank, TValue valueOrAddress, ValueType valueType = ValueType::Integer, float scale = 1.0f)
        {
            switch (bank)
            {
            case 0:
                switch (valueType)
                {
                case ValueType::Float:
                    // TODO: check for zero
                    return std::to_string(valueOrAddress / scale);
                case ValueType::Integer:
                default:
                    return std::to_string(valueOrAddress);
                }
            case 1:
            case 2:
            case 3:
            case 13:
            case 15:
            {
                const auto address = static_cast<uint32>(valueOrAddress) & 0xFF;
                const auto friendlyName = formatter.VarName(bank, valueOrAddress);
                if (friendlyName.empty())
                {
                    return (boost::format("FFVII.Data.var_%1%_%2%") % bank % address).str();
                }
                return (boost::format("FFVII.Data.%1%") % friendlyName).str();
            }
            case 5:
            case 6:
            {
                const auto address = static_cast<uint32>(valueOrAddress)& 0xFF;
                const  auto friendlyName = formatter.VarName(bank, address);
                if (friendlyName.empty())
                {
                    return (boost::format("FFVII.Data.temp_%1%_%2%") % bank % address).str();
                }
                return "FFVII.Data." + friendlyName;
            }
            default:
                //throw UnknownBankException();
                return (boost::format("FFVII.Data.unknown_%1%_%2%") % bank % (static_cast<uint32>(valueOrAddress) & 0xFF)).str();
            }
        }
    }
}
