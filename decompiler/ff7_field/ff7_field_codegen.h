#pragma  once

#include "decompiler_codegen.h"
#include <boost/algorithm/string.hpp>
#include <deque>
#include "make_unique.h"

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
        FF7CodeGenerator(Engine *engine, std::ostream &output) 
            : CodeGenerator(engine, output, kFIFOArgOrder, kLIFOArgOrder)
        {
            mTargetLang = std::make_unique<LuaTargetLanguage>();
        }
    protected:
        virtual std::string constructFuncSignature(const Function &func) override;
        virtual void onEndFunction(const Function& func) override;
        virtual void onBeforeStartFunction(const Function& func) override;
        virtual void onStartFunction(const Function& func) override;
        virtual bool OutputOnlyRequiredLabels() const override { return true; }
    };
}
