#pragma  once

#include "decompiler_codegen.h"
#include <boost/algorithm/string.hpp>
#include <boost/utility/string_ref.hpp>
#include <deque>

namespace FF7
{
    class FunctionMetaData
    {
    public:
        // Meta data format can be:
        // start_end_entityname
        // start_entityname
        // end_entity_name
        FunctionMetaData(boost::string_ref metaData)
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

    private:
        void Parse(boost::string_ref str)
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
                    ParseEntity(tmp);
                }
            }
            else
            {
                ParseEntity(item);
            }
        }

        void ParseEntity(const std::string& item)
        {
            mEntityName = item;
        }

        bool mEnd = false;
        bool mStart = false;
        std::string mEntityName;
    };

    class FF7CodeGenerator : public CodeGenerator
    {
    public:
        FF7CodeGenerator(Engine *engine, std::ostream &output) 
            : CodeGenerator(engine, output, kFIFOArgOrder, kLIFOArgOrder)
        {

        }
    protected:
        virtual std::string constructFuncSignature(const Function &func) override;
        virtual void onEndFunction(const Function& func) override;
        virtual void onBeforeStartFunction(const Function& func) override;
        virtual bool OutputOnlyRequiredLabels() const override { return true; }
    };
}
