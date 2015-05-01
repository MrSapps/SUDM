#include <gmock/gmock.h>

#include "decompiler/ff7_field/ff7_field_disassembler.h"
#include "decompiler/ff7_field/ff7_field_engine.h"
#include "decompiler/ff7_field/ff7_field_codegen.h"

#include "decompiler/ff7_world/ff7_world_disassembler.h"
#include "decompiler/ff7_world/ff7_world_engine.h"

#include "control_flow.h"
#include "util.h"
#include "graph.h"
#include "make_unique.h"
#include "sudm.h"
#include "lzs.h"
#include "ff7_field_dummy_formatter.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))


class TestReadParameterDisassembler : public SimpleDisassembler
{
public:
    TestReadParameterDisassembler(std::vector<unsigned char>&& data, InstVec& insts)
        : SimpleDisassembler(insts)
    {
        mStream = std::make_unique<BinaryReader>(std::move(data));
    }

    void readParams(InstPtr inst, const char *typeString)
    {
        return SimpleDisassembler::readParams(inst, typeString);
    }


    virtual void doDisassemble() throw(std::exception) override final
    {
        // NOP
    }

};

static InstPtr DoReadParameterTest(std::string str, std::vector<unsigned char> data)
{
    InstVec insts;
    TestReadParameterDisassembler d(std::move(data), insts);
    InstPtr inst = new FF7::FF7NoOperationInstruction();
    d.readParams(inst, str.c_str());
    return inst;
}

TEST(SimpleDisassembler, readParameter_U)
{
    std::vector<unsigned char> data = { 0xAA };
    InstPtr inst = DoReadParameterTest("U", data);

    ASSERT_EQ(inst->_params.size(), 2);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0x5);
    ASSERT_EQ(inst->_params[1]->getSigned(), 0xA);
}

TEST(SimpleDisassembler, readParameter_N)
{
    std::vector<unsigned char> data = { 0xAB };
    InstPtr inst = DoReadParameterTest("N", data);

    ASSERT_EQ(inst->_params.size(), 2);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0xA);
    ASSERT_EQ(inst->_params[1]->getSigned(), 0xB);
}

TEST(SimpleDisassembler, readParameter_b)
{
    std::vector<unsigned char> data = { (unsigned char)-100 };
    InstPtr inst = DoReadParameterTest("b", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), -100);
}

TEST(SimpleDisassembler, readParameter_B)
{
    std::vector<unsigned char> data = { 100 };
    InstPtr inst = DoReadParameterTest("B", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 100);
}

TEST(SimpleDisassembler, readParameter_s)
{
    std::vector<unsigned char> data = { (unsigned char)-40, 0x0 };
    InstPtr inst = DoReadParameterTest("s", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 216); // TODO: Casting removes the sign
}

TEST(SimpleDisassembler, readParameter_w)
{
    std::vector<unsigned char> data = { 0xFE, 0xAA };
    InstPtr inst = DoReadParameterTest("w", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 0xAAFE);
}

TEST(SimpleDisassembler, readParameter_i)
{
    std::vector<unsigned char> data = { (unsigned char)-0x30, 0x0, 0x0, 0x0 };
    InstPtr inst = DoReadParameterTest("i", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getSigned(), 208); // TODO: Casting removes the sign
}

TEST(SimpleDisassembler, readParameter_d)
{
    std::vector<unsigned char> data = { 0xde, 0xad, 0xbe, 0xef };
    InstPtr inst = DoReadParameterTest("d", data);

    ASSERT_EQ(inst->_params.size(), 1);
    ASSERT_EQ(inst->_params[0]->getUnsigned(), 0xEFBEADDE);
}

TEST(FF7Field, FunctionMetaData_Parse_Empty)
{
    FF7::FunctionMetaData meta("");
    ASSERT_EQ("", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_Empties)
{
    FF7::FunctionMetaData meta("__________________");
    ASSERT_EQ("", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_Start)
{
    FF7::FunctionMetaData meta("start_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(false, meta.IsEnd());
    ASSERT_EQ(true, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_End)
{
    FF7::FunctionMetaData meta("end_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}

TEST(FF7Field, FunctionMetaData_Parse_EntityName)
{
    FF7::FunctionMetaData meta("end_-1_TheName");
    ASSERT_EQ("TheName", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
    ASSERT_EQ(-1, meta.CharacterId());

}

TEST(FF7Field, FunctionMetaData_Parse_EntityNameAndId)
{
    FF7::FunctionMetaData meta("end_99_The_Name");
    ASSERT_EQ("The_Name", meta.EntityName());
    ASSERT_EQ(99, meta.CharacterId());

    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(false, meta.IsStart());
}


TEST(FF7Field, FunctionMetaData_Parse_StartEnd)
{
    FF7::FunctionMetaData meta("start_end_-1_entity");
    ASSERT_EQ("entity", meta.EntityName());
    ASSERT_EQ(true, meta.IsEnd());
    ASSERT_EQ(true, meta.IsStart());
}

/*
TEST(FF7World, DisAsm)
{
    for (int i = 0; i < 256; i++)
    {
        FF7::FF7WorldEngine engine(i);


        InstVec insts;
        std::cout << std::endl;

        std::cout << std::endl;

        std::cout << "SCRIPT(" << i << ")" << std::endl;


        auto d = engine.getDisassembler(insts);
        d->open("decompiler/ff7_world/wm0.ev");
        d->disassemble();
        d->dumpDisassembly(std::cout);
        std::cout << std::endl;


        auto c = std::make_unique<ControlFlow>(insts, engine);
        c->createGroups();




        Graph g = c->analyze();

        engine.postCFG(insts, g);


        onullstream ns;
        auto cg = engine.getCodeGenerator(insts, std::cout);

        std::ofstream out;
        out.open("graph.dot");
        if (out.is_open())
        {
            auto& g = c->getGraph();
            boost::write_graphviz(
                out, g, boost::make_label_writer(get(boost::vertex_name, g)),
                boost::makeArrowheadWriter(get(boost::edge_attribute, g)), GraphProperties(&engine, g));
        }
        out.close();

        cg->generate(insts, g);

        VertexIterator v = boost::vertices(g).first;
        GroupPtr gr = GET(*v);

        // Find first node
        while (gr->_prev != NULL)
        {
            gr = gr->_prev;
        }

        // Copy out all lines of code
        std::vector<std::string> output;
        while (gr != NULL)
        {
            for (std::vector<CodeLine>::iterator it = gr->_code.begin(); it != gr->_code.end(); ++it)
            {
                output.push_back(it->_line);
            }
            gr = gr->_next;
        }

        ASSERT_TRUE(output.empty() == false);
    }
}
*/

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

class Tokenzier
{
public:
    Tokenzier(const std::string& input)
        : mData(input)
    {

    }

    enum class eTokenType
    {
        eWhiteSpace,
        eText,
        eNumber,
        eInvalid,
        eArgumentDelmiter,
        eOpenBracket,
        eCloseBracket,
        eOpenCurlyBracket,
        eCloseCurlyBracket,
        eQuote,
        eEof
    };

    class Token
    {
    public:
        Token() = default;

        Token(eTokenType type, std::string text, int line, int column)
            : mType(type), mText(text), mColumn(column), mLine(line)
        {

        }

        Token(eTokenType type, int number, int line, int column)
            : mType(type), mNumber(number), mColumn(column), mLine(line)
        {

        }

        Token(eTokenType type, int line, int column)
            : mType(type), mColumn(column), mLine(line)
        {

        }

        std::string AsString() const
        {
            return mText;
        }

        int AsNumber() const
        {
            return mNumber;
        }

        eTokenType Type() const { return mType; }
        int Line() const { return mLine; }
        int Column() const { return mColumn; }

    private:
        std::string mText;
        int mNumber = 0;
        eTokenType mType = eTokenType::eInvalid;
        int mLine = -1;
        int mColumn = -1;
    };

    bool IsLineBreak(char item)
    {
        return item == '\r' || item == '\n';
    }

    bool IsSpace(char item, std::locale& loc)
    {
        return std::isspace(item, loc) && !IsLineBreak(item);
    }

    bool IsAlpha(char item)
    {
        return
            (item >= 'A' && item <= 'Z') || 
            (item >= 'a' && item <= 'z') ||
            item == '_';
    }

    bool IsDigit(char item, std::locale& loc)
    {
        return std::isdigit(item, loc);
    }

    Token Next()
    {
        char item = ' ';

        if (!mData)
        {
            return Token(eTokenType::eEof, mCurLine, mCurCol);
        }

        ReadChar(item);

        bool eof = false;
        item = ConsumeWhiteSpace(item, eof);
        if (eof)
        {
            return Token(eTokenType::eEof, mCurLine, mCurCol-1);
        }

        // Begin comment, consume until eLineBreak
        const bool startComment = item == ';';
        if (startComment)
        {
            bool isLineBreak = false;
            do
            {
                if (!ReadChar(item))
                {
                    return Token(eTokenType::eEof, mCurLine, mCurCol - 1);
                }
                isLineBreak = IsLineBreak(item);
            } while (!isLineBreak);

            item = ConsumeWhiteSpace(item, eof);
            if (eof)
            {
                return Token(eTokenType::eEof, mCurLine, mCurCol - 1);
            }
        }
        
        if (item == ',')
        {
            return Token(eTokenType::eArgumentDelmiter, ",", mCurLine, mCurCol - 1);
        }
        else if (item == '(')
        {
            return Token(eTokenType::eOpenBracket, "(", mCurLine, mCurCol - 1);
        }
        else if (item == ')')
        {
            return Token(eTokenType::eCloseBracket, ")", mCurLine, mCurCol - 1);
        }
        else if (item == '{')
        {
            return Token(eTokenType::eOpenCurlyBracket, "{", mCurLine, mCurCol - 1);
        }
        else if (item == '}')
        {
            return Token(eTokenType::eCloseCurlyBracket, "}", mCurLine, mCurCol - 1);
        }
        else if (item == '"')
        {
            return Token(eTokenType::eQuote, "\"", mCurLine, mCurCol-1);
        }
        const bool alpha = IsAlpha(item) || item == ';';
        if (alpha)
        {
            const int startCol = mCurCol - 1;

            std::string text(1, item);

            // eText, consume until eLineBreak or whitespace
            bool stillAlphaOrDigit = false;
            do
            {
                if (!ReadChar(item))
                {
                    return Token(eTokenType::eText, text, mCurLine, startCol);
                }
                // Strings can end with a :
                stillAlphaOrDigit = IsAlpha(item) || IsDigit(item, mLoc) || item == ':';
                if (stillAlphaOrDigit)
                {
                    text.push_back(item);
                    if (item == ':')
                    {
                        break;
                    }
                }
                else
                {
                    // Put the char back
                    mData.seekg(-1, std::ios::cur);
                }
            } while (stillAlphaOrDigit);

            return Token(eTokenType::eText, text, mCurLine, startCol);

        }
        else if (IsDigit(item, mLoc) || item == '-') // Start of negative number
        {
            // eNumber, consume until eLineBreak or whitespace, each item before that must be a digit
            const int startCol = mCurCol - 1;
            std::string text(1, item);
            bool isANumber = true;
            do
            {
                ReadChar(item);
                if (!mData && item == '-')
                {
                    // We ONLY have a -, this is invalid
                    return Token(eTokenType::eInvalid, mCurLine, startCol);
                }
                else if (!mData)
                {
                    return Token(eTokenType::eNumber, std::stoi(text), mCurLine, startCol);
                }
                isANumber = IsDigit(item, mLoc);
                if (isANumber)
                {
                    text.push_back(item);
                }
                else
                {
                    // Put the char back
                    mData.seekg(-1, std::ios::cur);
                }
            } while (isANumber);
            return Token(eTokenType::eNumber, std::stoi(text), mCurLine, startCol);
        }
        else
        {
            // eInvalid
            return Token(eTokenType::eInvalid, mCurLine, mCurCol-1);
        }
    }

private:
    char ConsumeWhiteSpace(char item, bool& eof)
    {
        // Eat all whitespace
        bool space = IsSpace(item, mLoc) || IsLineBreak(item);
        if (space)
        {
            do
            {
                if (!ReadChar(item))
                {
                    eof = true;
                    return item;
                }
                space = IsSpace(item, mLoc) || IsLineBreak(item);
            } while (space);
        }
        return item;
    }

    bool ReadChar(char& output)
    {
        mData.read(&output, 1);
        if (mData)
        {
            if (IsLineBreak(output))
            {
                mCurLine++;
                mCurCol = 1;
            }
            else
            {
                mCurCol++;
            }
            return true;
        }
        return false;
    }

    std::locale mLoc;
    std::stringstream mData;
    int mCurLine = 1;
    int mCurCol = 1;
};



TEST(Tokenzier, Empty)
{
    Tokenzier t("");
    ASSERT_EQ(Tokenzier::eTokenType::eEof, t.Next().Type());
}

TEST(Tokenzier, ReadText)
{
    Tokenzier t("Hello");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Hello", token.AsString());
}

TEST(Tokenzier, ReadText2)
{
    Tokenzier t("Hello world");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Hello", token.AsString());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("world", token.AsString());
}


// Check !"£$%^&*-+<>?;'[]#\/`¬ are marked as invalid and () is not part of text
TEST(Tokenzier, ReadInvalidText)
{
    const char kInvalidChars[] = R"(;!£$%^&*-+<>?'[]#\/`¬)";
    const int kNumChars = sizeof(kInvalidChars) / sizeof(char);
    for (int i = 0; i < kNumChars; i++)
    {
        Tokenzier t(std::string(1,kInvalidChars[i]));
        Tokenzier::Token token = t.Next();
        if (i == 0)
        {
            // ; without a line break becomes EOF
            ASSERT_EQ(Tokenzier::eTokenType::eEof, token.Type());
        }
        else
        {
            ASSERT_EQ(Tokenzier::eTokenType::eInvalid, token.Type());
        }
        ASSERT_EQ("", token.AsString());
    }
}

TEST(Tokenzier, ReadInvalidText2)
{
    Tokenzier t("H!£$%^");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("H", token.AsString());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eInvalid, token.Type());
    ASSERT_EQ("", token.AsString());

}


TEST(Tokenzier, ReadBrackets)
{
    {
        Tokenzier t("(");
        Tokenzier::Token token = t.Next();
        ASSERT_EQ(Tokenzier::eTokenType::eOpenBracket, token.Type());
        ASSERT_EQ("(", token.AsString());
    }
    {
        Tokenzier t(")");
        Tokenzier::Token token = t.Next();
        ASSERT_EQ(Tokenzier::eTokenType::eCloseBracket, token.Type());
        ASSERT_EQ(")", token.AsString());
    }
}

TEST(Tokenzier, ReadLabel)
{
    Tokenzier t("Hello:");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Hello:", token.AsString());
}

TEST(Tokenzier, ReadLabel2)
{
    Tokenzier t("Hello:a");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Hello:", token.AsString());
}

TEST(Tokenzier, ReadWhiteSpace)
{
    Tokenzier t("\t ");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eEof, token.Type());
    ASSERT_EQ("", token.AsString());

}

TEST(Tokenzier, ReadArgumentDelimiter)
{
    Tokenzier t(",");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eArgumentDelmiter, token.Type());
    ASSERT_EQ(",", token.AsString());

}

TEST(Tokenzier, ReadNewLine)
{
    Tokenzier t("\r\n\r\r\n\n");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eEof, token.Type());
    ASSERT_EQ("", token.AsString());
}

TEST(Tokenzier, ReadNegativeNumber)
{
    Tokenzier t("-12345");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eNumber, token.Type());
    ASSERT_EQ(-12345, token.AsNumber());
}

TEST(Tokenzier, ReadNumber)
{
    Tokenzier t("12345");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eNumber, token.Type());
    ASSERT_EQ(12345, token.AsNumber());
}

TEST(Tokenzier, ReadQuote)
{
    Tokenzier t("\"");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eQuote, token.Type());
    ASSERT_EQ("\"", token.AsString());
}

TEST(Tokenzier, ReadCurlyBrackets)
{
    Tokenzier t("{}");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eOpenCurlyBracket, token.Type());
    ASSERT_EQ("{", token.AsString());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eCloseCurlyBracket, token.Type());
    ASSERT_EQ("}", token.AsString());
}

// Combos, whitespace, new line, text, number, argument delimiter eof
TEST(Tokenzier, Combos)
{

    Tokenzier t(" \nText1234:1234,;Fool1\nFool2");
    Tokenzier::Token token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Text1234:", token.AsString());
    ASSERT_EQ(2, token.Line());
    ASSERT_EQ(1, token.Column());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eNumber, token.Type());
    ASSERT_EQ(1234, token.AsNumber());
    ASSERT_EQ(2, token.Line());
    ASSERT_EQ(10, token.Column());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eArgumentDelmiter, token.Type());
    ASSERT_EQ(",", token.AsString());
    ASSERT_EQ(2, token.Line());
    ASSERT_EQ(15, token.Column());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Fool2", token.AsString());
    ASSERT_EQ(3, token.Line());
    ASSERT_EQ(1, token.Column());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eEof, token.Type());
    ASSERT_EQ("", token.AsString());
}

typedef std::vector<std::pair<std::string, Tokenzier::Token>> JumpToken;

class Assembler
{
public:
    class Method
    {
    public:
        Method(const std::string& name)
            : mName(name)
        {

        }

        bool AddLabel(std::string name, const Tokenzier::Token& token)
        {
            // Labels end with ":" so chop that char off
            name = name.substr(0, name.length() - 1);

            auto it = mLabels.find(name);
            if (it != std::end(mLabels))
            {
                return false;
            }
            mLabels.insert(std::make_pair(name, token));
            return true;
        }

        void AddJump(const std::string& label, const Tokenzier::Token& token)
        {
            auto it = mJumps.find(label);
            if (it != std::end(mJumps))
            {
                it->second.push_back(token);
            }
            else
            {
                std::vector<Tokenzier::Token> t;
                t.push_back(token);
                mJumps.insert(std::make_pair(label, t));
            }
        }

        void VerifyAndResolveLabels(JumpToken& unreferencedLabels, JumpToken& undefinedLabels)
        {
            
            for (const auto& jumpTarget : mJumps)
            {
                auto it = mLabels.find(jumpTarget.first);
                if (it == std::end(mLabels))
                {
                    undefinedLabels.push_back(std::make_pair(jumpTarget.first, jumpTarget.second[0]));
                }
            }

            for (const auto& label : mLabels)
            {
                auto it = mJumps.find(label.first);
                if (it == std::end(mJumps))
                {
                    unreferencedLabels.push_back(std::make_pair(label.first, label.second));
                }
            }

        }

    private:
        std::string mName;
        std::map<std::string, Tokenzier::Token> mLabels;
        std::map<std::string, std::vector<Tokenzier::Token>> mJumps;
    };

    class Object
    {
    public:
        Object(const std::string& name)
            : mName(name)
        {

        }

        const std::string& Name() const { return mName; }

        Method* AddMethod(const std::string& name)
        {
            auto method = FindMethod(name);
            if (!method)
            {
                mMethods[name] = std::make_unique<Method>(name);
                return FindMethod(name);
            }
            return nullptr;
        }

        size_t MethodCount() const { return mMethods.size(); }
        bool HasMethod(const std::string& name) const
        {
            return FindMethod(name) != nullptr;
        }

    private:
        Method* FindMethod(const std::string& name) const
        {
            auto it = mMethods.find(name);
            if (it == std::end(mMethods))
            {
                return nullptr;
            }
            return it->second.get();
        }

        std::string mName;
        std::map<std::string, std::unique_ptr<Method>> mMethods;
    };

    Assembler()
    {

    }

    // Something which wraps "methods"
    Object* AddObject(const std::string& objectName)
    {
        auto obj = FindObject(objectName);
        if (!obj)
        {
            mObjects[objectName] = std::make_unique<Object>(objectName);
            return FindObject(objectName);
        }
        return nullptr;
    }

    size_t ObjectCount() const { return mObjects.size(); }
private:
    Object* FindObject(const std::string& objectName) const
    {
        auto it = mObjects.find(objectName);
        if (it == std::end(mObjects))
        {
            return nullptr;
        }
        return it->second.get();
    }

    std::map<std::string, std::unique_ptr<Object>> mObjects;
};

class Parser
{
public:
    class Exception : public std::exception
    {
    public:
        Exception(const std::string& msg)
            : mMsg(msg)
        {

        }

        Exception(const std::string& msg, const Tokenzier::Token& token)
            : mMsg(msg)
        {
            mMsg += " on line: " + std::to_string(token.Line()) + ", col: " + std::to_string(token.Column());
            if (token.Type() == Tokenzier::eTokenType::eNumber)
            {
                mMsg += ", with value: '" + std::to_string(token.AsNumber()) + "'";
            }
            else if (token.AsString().empty() == false)
            {
                mMsg += ", with value: '" + token.AsString() + "'";
            }
        }

        virtual const char* what() const throw() override
        {
            return mMsg.c_str();
        }

    private:
        std::string mMsg;
    };

    class DuplicateObjectNameException : public Exception
    {
    public:
        DuplicateObjectNameException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class DuplicateMethodNameException : public Exception
    {
    public:
        DuplicateMethodNameException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class TooManyObjectsException : public Exception
    {
    public:
        TooManyObjectsException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class TooManyMethodsException : public Exception
    {
    public:
        TooManyMethodsException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class TooManyArgumentsException : public Exception
    {
    public:
        TooManyArgumentsException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class ArgumentOutOfRangeException : public Exception
    {
    public:
        ArgumentOutOfRangeException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class DuplicateLabelException : public Exception
    {
    public:
        DuplicateLabelException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class UndefinedLabelException : public Exception
    {
    public:
        UndefinedLabelException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    class UnknownOpCodeException : public Exception
    {
    public:
        UnknownOpCodeException(const std::string& msg, const Tokenzier::Token& token) : Exception(msg, token) { }
    };

    Parser(const std::string& src)
        : mTokenzier(src)
    {

    }

    void Parse()
    {
        std::deque<Tokenzier::Token> tokenStack;
        for (;;)
        {
            Tokenzier::Token token = mTokenzier.Next();
            if (token.Type() == Tokenzier::eTokenType::eInvalid)
            {
                throw Exception("Invalid syntax", token);
            }

            if (token.Type() == Tokenzier::eTokenType::eEof)
            {
                break;
            }
            tokenStack.push_back(token);
        }
        Parse(tokenStack);
    }

private:
    void Parse(std::deque<Tokenzier::Token>& tokens)
    {
        while (tokens.empty() == false)
        {
            // Everything at the top level should be an entity
            ParseEntity(tokens);
        }
    }

    void ParseEntity(std::deque<Tokenzier::Token>& tokens)
    {
        // Starts with entity keyword
        ExpectText("Entity", NextToken(tokens));

        // Entity name is quoted string within brackets
        ExpectToken(Tokenzier::eTokenType::eOpenBracket, tokens);
        Tokenzier::Token entityNameToken = ExpectQuotedString(tokens);
        ExpectToken(Tokenzier::eTokenType::eCloseBracket, tokens);

        std::cout << "ParseEntity: " << entityNameToken.AsString().c_str() << std::endl;

        auto pObj = mAssembler.AddObject(entityNameToken.AsString());
        if (!pObj)
        {
            throw DuplicateObjectNameException("Duplicated entity name", entityNameToken);
        }

        if (mAssembler.ObjectCount() >= 32)
        {
            throw TooManyObjectsException("There can only be 32 entities", entityNameToken);
        }

        // Entity body within { }'s
        ExpectToken(Tokenzier::eTokenType::eOpenCurlyBracket, tokens);

        // Handle empty {}'s case
        if (PeekTokenType(tokens) != Tokenzier::eTokenType::eCloseCurlyBracket)
        {
            ParseEntityMethods(tokens, *pObj);
        }
        ExpectToken(Tokenzier::eTokenType::eCloseCurlyBracket, tokens);

    }
    
    void ParseEntityMethods(std::deque<Tokenzier::Token>& tokens, Assembler::Object& obj)
    {
        while (tokens.empty() == false)
        {
            // Marks the end of functions
            if (PeekTokenType(tokens) == Tokenzier::eTokenType::eCloseCurlyBracket)
            {
                break;
            }
            // Otherwise must be fn name() { } decl
            ParseEntityMethod(tokens, obj);
        }
    }

    void ParseEntityMethod(std::deque<Tokenzier::Token>& tokens, Assembler::Object& obj)
    {
        // Starts with fn keyword
        ExpectText("fn", NextToken(tokens));

        // Function name
        Tokenzier::Token text = NextToken(tokens);
        ExpectTokenType(Tokenzier::eTokenType::eText, text);
      
        std::cout << "Entity function: " << text.AsString().c_str() << std::endl;
        auto pMethod = obj.AddMethod(text.AsString());
        if (!pMethod)
        {
            throw DuplicateMethodNameException("Duplicated function name", text);
        }

        size_t methodCount = obj.MethodCount();
        if (obj.HasMethod("init") && obj.HasMethod("main"))
        {
            // init and main count as 1 script
            methodCount--;
        }
        if (methodCount >= 32)
        {
            throw TooManyMethodsException("There can only be 31 scripts excluding init and main", text);
        }

        // Brackets end function name
        ExpectToken(Tokenzier::eTokenType::eOpenBracket, tokens);
        ExpectToken(Tokenzier::eTokenType::eCloseBracket, tokens);

        //  Body within { } 's,
        ExpectToken(Tokenzier::eTokenType::eOpenCurlyBracket, tokens);

        // handle empty {}'s case
        if (PeekTokenType(tokens) != Tokenzier::eTokenType::eCloseCurlyBracket)
        {
            ParseEntityMethodBody(tokens, *pMethod);
        }
        ExpectToken(Tokenzier::eTokenType::eCloseCurlyBracket, tokens);

        JumpToken unreferencedLabels;
        JumpToken undefinedLabels;
        pMethod->VerifyAndResolveLabels(unreferencedLabels, undefinedLabels);
        if (!undefinedLabels.empty())
        {
            throw UndefinedLabelException("Undefined label", undefinedLabels[0].second);
        }
        if (!unreferencedLabels.empty())
        {
            // Warn about unused labels
            for (const auto& label : unreferencedLabels)
            {
                std::cout << "WARNING: label " + label.first + " is not used on line : " + std::to_string(label.second.Line()) + ", col : " + std::to_string(label.second.Column()) << std::endl;
            }
        }
    }
    
    void ParseEntityMethodBody(std::deque<Tokenzier::Token>& tokens, Assembler::Method& method)
    {
        while (tokens.empty() == false)
        {
            // } is the end of the function
            if (PeekTokenType(tokens) == Tokenzier::eTokenType::eCloseCurlyBracket)
            {
                // TODO: Validate semantics of instructions parsed

                break;
            }
            Tokenzier::Token text = NextToken(tokens);

            // Probably someone has a passed a number argument to an opcode that takes no arguments
            if (text.Type() == Tokenzier::eTokenType::eNumber)
            {
                throw UnknownOpCodeException("Expected opcode text", text);
            }

            ExpectTokenType(Tokenzier::eTokenType::eText, text);

            // Label
            std::string str = text.AsString();
            if (str.back() == ':')
            {
                std::cout << "LABEL:" << str.c_str() << std::endl;
                if (!method.AddLabel(str.c_str(), text))
                {
                    throw DuplicateLabelException("Duplicated label", text);
                }
            }
            // instruction/mnemonic
            else
            {
                std::cout << "INSTRUCTION:" << str.c_str() << std::endl;
                ParseInstruction(text, tokens, method);
            }
        }
    }

    void ParseInstruction(const Tokenzier::Token& inst, std::deque<Tokenzier::Token>& tokens, Assembler::Method& method)
    {
        const auto insts = FF7::FieldInstructions();
        auto it = insts.find(inst.AsString());
        if (it == std::end(insts))
        {
            throw UnknownOpCodeException(inst.AsString() + " is not a known instruction", inst);
        }

        // TODO: Handle arguments correctly, validate labels
        const FF7::TInstructRecord* rec = it->second;
        const char* fmt = rec->mArgumentFormat;

        // method.AddInstruction(rec->mOpCode, rec->mOpCodeSize);
        // TODO: Flow control needs special handling

        while (*fmt)
        {
            bool handled = false;
            switch (*fmt)
            {
                // TODO: Handle nibbles correctly
                /*
            case 'N':
            {
                // TODO
                ExpectTokenType(Tokenzier::eTokenType::eNumber, PeekToken(tokens));
                NextToken(tokens);
                secondNib = !secondNib;
            }
            break;*/
            case 'N':
            case 'B':
            {
                ExpectTokenType(Tokenzier::eTokenType::eNumber, PeekToken(tokens));
                const auto token = NextToken(tokens);
                if (token.AsNumber() < 0 || token.AsNumber() > 255)
                {
                    throw ArgumentOutOfRangeException("Byte argument must be between 0 and 255", token);
                }
                //method.AddInstructionArgument<unsigned char>(token.AsNumber());
                handled = true;
            }
            break;

            case 'U':
            {
                ExpectTokenType(Tokenzier::eTokenType::eNumber, PeekToken(tokens));
                const auto token = NextToken(tokens);
                if (token.AsNumber() < 0 || token.AsNumber() > 65535)
                {
                    throw ArgumentOutOfRangeException("Unsigned short argument must be between 0 and 65535", token);
                }
                //method.AddInstructionArgument<unsigned short int>(token.AsNumber());
                handled = true;
            }
            break;

            // Single byte label
            case 'L':
            {
                ExpectTokenType(Tokenzier::eTokenType::eText, PeekToken(tokens));
                const auto token = NextToken(tokens);
                std::cout << "ADD JUMP: " << token.AsString() << std::endl;
                method.AddJump(token.AsString(), token);
                handled = true;
            }
            break;

            default:
                // TODO: Throw
                break;
            }
            
            fmt++;

            // If we parsed and argument and there is another argument, then they
            // should be separated by an argument delimiter
            if (handled && *fmt)
            {
                ExpectTokenType(Tokenzier::eTokenType::eArgumentDelmiter, NextToken(tokens));
            }
        }

        if (PeekTokenType(tokens) == Tokenzier::eTokenType::eArgumentDelmiter)
        {
            throw TooManyArgumentsException("Too many arguments for opcode", NextToken(tokens));
        }
    }

    Tokenzier::Token ExpectQuotedString(std::deque<Tokenzier::Token>& tokens)
    {
        ExpectToken(Tokenzier::eTokenType::eQuote, tokens);
        Tokenzier::Token text = NextToken(tokens);
        ExpectTokenType(Tokenzier::eTokenType::eText, text);
        ExpectToken(Tokenzier::eTokenType::eQuote, tokens);
        return text;
    }

    Tokenzier::eTokenType PeekTokenType(const std::deque<Tokenzier::Token>& tokens)
    {
        return PeekToken(tokens).Type();
    }

    void ExpectTokenType(Tokenzier::eTokenType type, const Tokenzier::Token& token)
    {
        if (token.Type() != type)
        {
            throw Exception("Expected token of type: " + ToString(type) + " but got: " + ToString(token.Type()), token);
        }
    }

    void ExpectToken(Tokenzier::eTokenType type, std::deque<Tokenzier::Token>& tokens)
    {
        ExpectTokenType(type, NextToken(tokens));
    }

    void ExpectText(const std::string& expectedText, const Tokenzier::Token& token)
    {
        if (token.Type() != Tokenzier::eTokenType::eText)
        {
            throw Exception("Expected token of type: " + ToString(Tokenzier::eTokenType::eText) + " but got: " + ToString(token.Type()), token);
        }

        if (token.AsString() != expectedText)
        {
            throw Exception("Expected keyword: " + expectedText + " but got: " + token.AsString(), token);
        }
    }

    Tokenzier::Token NextToken(std::deque<Tokenzier::Token>& tokens)
    {
        if (tokens.empty())
        {
            throw Exception("Expected more tokens before end of input");
        }
        Tokenzier::Token ret = tokens.front();
        tokens.pop_front();
        return ret;
    }

    Tokenzier::Token PeekToken(const std::deque<Tokenzier::Token>& tokens)
    {
        if (tokens.empty())
        {
            throw Exception("Expected more tokens before end of input");
        }
        return tokens.front();
    }

    std::string ToString(Tokenzier::eTokenType type)
    {
        std::string ret;
        switch (type)
        {
        case Tokenzier::eTokenType::eArgumentDelmiter:
            ret = "argument delimiter";
            break;

        case Tokenzier::eTokenType::eWhiteSpace:
            ret = "white space";
            break;

        case Tokenzier::eTokenType::eText:
            ret = "text";
            break;

        case Tokenzier::eTokenType::eNumber:
            ret = "number";
            break;

        case Tokenzier::eTokenType::eInvalid:
            ret = "invalid";
            break;

        case Tokenzier::eTokenType::eOpenBracket:
            ret = "open bracket";
            break;

        case Tokenzier::eTokenType::eCloseBracket:
            ret = "close bracket";
            break;

        case Tokenzier::eTokenType::eOpenCurlyBracket:
            ret = "open curly brace";
            break;

        case Tokenzier::eTokenType::eCloseCurlyBracket:
            ret = "close curly brace";
            break;

        case Tokenzier::eTokenType::eQuote:
            ret = "quote";
            break;

        case Tokenzier::eTokenType::eEof:
            ret = "end of file";
            break;

        default:
            // Shouldn't actually be possible
            throw Exception("Unknown token type: " + std::to_string(static_cast<int>(type)));
        }
        return ret;
    }

    Tokenzier mTokenzier;
    Assembler mAssembler;
};

TEST(Parser, SimpleScript)
{
    Parser p("Entity(\"Testing\") { fn init() { start: NOP REQ 1,2 ;JMPB start\r\n  } fn main() { }  fn script1() { ;IFUB 0, 0, 1, 2, 0, lfalse NOP lfalse:\n  }  } ; Example\n Entity(\"Second\")\n {\n fn\n init()\n {\n }\n }");
    ASSERT_NO_THROW(p.Parse());
}

// Parse unknown opcode
TEST(Parser, UnknownOpcode)
{
    Parser p("Entity(\"Testing\") { fn init() { FLUB } } ");
    ASSERT_THROW(p.Parse(), Parser::UnknownOpCodeException);
}

// Parse duplicate entity name
TEST(Parser, DuplicateObjectName)
{
    Parser p("Entity(\"Testing\") { fn init() {  } } Entity(\"Testing\") { fn init() {  } } ");
    ASSERT_THROW(p.Parse(), Parser::DuplicateObjectNameException);
}

// Parse duplicate function name
TEST(Parser, DuplicateMethodName)
{
    Parser p("Entity(\"Testing\") { fn init() { } fn init() { }  }");
    ASSERT_THROW(p.Parse(), Parser::DuplicateMethodNameException);
}

// Parse too many entities
TEST(Parser, TooManyEntities)
{
    std::string strScript;
    for (int i = 0; i < 32; i++) // TODO: Find out what the actual limit is
    {
        const std::string entityName = "Test" + std::to_string(i);
        const std::string entity = "Entity(\"" +  entityName  + "\") { fn init() { } }";
        strScript += entity;
    }
    Parser p(strScript);
    ASSERT_THROW(p.Parse(), Parser::TooManyObjectsException);
}


// Parse too many functions
TEST(Parser, TooManyFunctions)
{
    std::string strFunctions;
    for (int i = 0; i < 31; i++)
    {
        strFunctions += "fn test" + std::to_string(i) +"() { NOP NOP } ";
    }

    Parser p("Entity(\"Testing\") { fn init() { } fn main() { }" + strFunctions  +"}");
    ASSERT_THROW(p.Parse(), Parser::TooManyMethodsException);
}


// Parse too many arguments
TEST(Parser, TooManyArguments)
{
    {
        Parser p("Entity(\"Testing\") { fn init() { NOP 1 } }");
        ASSERT_THROW(p.Parse(), Parser::UnknownOpCodeException);
    }

    {
         Parser p("Entity(\"Testing\") { fn init() { REQSW 1,2,3 } }");
         ASSERT_THROW(p.Parse(), Parser::TooManyArgumentsException);
    }
}


// Parse not enough arguments
TEST(Parser, NotEnoughArguments)
{
    Parser p("Entity(\"Testing\") { fn init() { REQSW 1 } }");
    ASSERT_THROW(p.Parse(), Parser::Exception); // Expects , after 1
}

// Parse argument value out of range
TEST(Parser, ArgumentOutOfRange)
{
    Parser p("Entity(\"Testing\") { fn init() { REQSW 256, 9999999 } }");
    ASSERT_THROW(p.Parse(), Parser::ArgumentOutOfRangeException);
}

// Parse duplicated labels
TEST(Parser, DuplicatedLabels)
{
    Parser p("Entity(\"Testing\") { fn init() { label: label: } }");
    ASSERT_THROW(p.Parse(), Parser::DuplicateLabelException);
}

// Parse IF
TEST(Parser, ParseIf)
{
    // TODO: Could allow cleaner syntax:
    //Parser p("Entity(\"Testing\") { fn init() { IFUB Var(1,2) > Var(0,4) foo NOP foo:  } }");
    Parser p("Entity(\"Testing\") { fn init() { IFUB 1, 0, 20, 30, 1, foo NOP foo: } }");
    p.Parse();
}

// Parse IF's with missing labels
TEST(Parser, ParseIfMissingLabel)
{
    Parser p("Entity(\"Testing\") { fn init() { IFUB 1, 0, 20, 30, 1, foo NOP } }");
    ASSERT_THROW(p.Parse(), Parser::UndefinedLabelException);
}

// Parse generates warning on unused label
TEST(Parser, WarnsOnUnusedLabels)
{
    // TODO: Not auto checked as writes to std out, it should probably call back to us so we can check
    Parser p("Entity(\"Testing\") { fn init() { IFUB 1, 0, 20, 30, 1, foo NOP foo: asdf: \nasdsds: \nhello: } }");
    p.Parse();

}

// Simple assemble / disassemble
TEST(Parser, AssembleDisassemble)
{
    Parser p("Entity(\"Testing\") { fn main() { NOP }  fn init() { NOP } }");

}

// Parse IF's that jump after the if with no "gap"
/*
TEST(Parser, ZeroGapIf)
{
    Parser p("Entity(\"Testing\") { fn init() { IFUB 1, 0, 20, 30, 1, foo foo: } }");
    ASSERT_THROW(p.Parse(), Parser::JumpHasNoGap);
}
*/

// Parse var length arguments

// Parse double meaning arguments

// Parse all op codes

// Assemble all op codes

// Disassemble all assembled op codes + compare

// Check function ends with RET or JUMPB/JMPBL - is this actually required?

TEST(FF7Field, Asm)
{
    DummyFormatter dummy;
    FF7::FF7FieldEngine eng(dummy);

    InstVec insts;
    FF7::FF7Disassembler d(dummy, &eng, insts);


}

