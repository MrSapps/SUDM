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
    std::vector<unsigned char> data = { (unsigned char)-40 };
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
    std::vector<unsigned char> data = { (unsigned char)-0x30 };
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
            return Token(eTokenType::eEof, mCurLine, mCurCol);
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
                    return Token(eTokenType::eEof, mCurLine, mCurCol);
                }
                isLineBreak = IsLineBreak(item);
            } while (!isLineBreak);

            item = ConsumeWhiteSpace(item, eof);
            if (eof)
            {
                return Token(eTokenType::eEof, mCurLine, mCurCol);
            }
        }
        
        if (item == ',')
        {
            return Token(eTokenType::eArgumentDelmiter, ",", mCurLine, mCurCol);
        }
        else if (item == '(')
        {
            return Token(eTokenType::eOpenBracket, "(", mCurLine, mCurCol);
        }
        else if (item == ')')
        {
            return Token(eTokenType::eCloseBracket, ")", mCurLine, mCurCol);
        }
        else if (item == '{')
        {
            return Token(eTokenType::eOpenCurlyBracket, "{", mCurLine, mCurCol);
        }
        else if (item == '}')
        {
            return Token(eTokenType::eCloseCurlyBracket, "}", mCurLine, mCurCol);
        }
        else if (item == '"')
        {
            return Token(eTokenType::eQuote, "\"", mCurLine, mCurCol);
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
                    return Token(eTokenType::eText, text, mCurLine, mCurCol);
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
            return Token(eTokenType::eInvalid, mCurLine, mCurCol);
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
                mCurCol = 0;
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

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eNumber, token.Type());
    ASSERT_EQ(1234, token.AsNumber());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eArgumentDelmiter, token.Type());
    ASSERT_EQ(",", token.AsString());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eText, token.Type());
    ASSERT_EQ("Fool2", token.AsString());

    token = t.Next();
    ASSERT_EQ(Tokenzier::eTokenType::eEof, token.Type());
    ASSERT_EQ("", token.AsString());
}

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
        std::string entityName = ExpectQuotedString(tokens);
        ExpectToken(Tokenzier::eTokenType::eCloseBracket, tokens);

        std::cout << "ParseEntity: " << entityName.c_str() << std::endl;

        // Entity body within { }'s
        ExpectToken(Tokenzier::eTokenType::eOpenCurlyBracket, tokens);

        // Handle empty {}'s case
        if (PeekTokenType(tokens) != Tokenzier::eTokenType::eCloseCurlyBracket)
        {
            ParseEntityMethods(tokens);
        }
        ExpectToken(Tokenzier::eTokenType::eCloseCurlyBracket, tokens);

    }
    
    void ParseEntityMethods(std::deque<Tokenzier::Token>& tokens)
    {
        while (tokens.empty() == false)
        {
            // Marks the end of functions
            if (PeekTokenType(tokens) == Tokenzier::eTokenType::eCloseCurlyBracket)
            {
                break;
            }
            // Otherwise must be fn name() { } decl
            ParseEntityMethod(tokens);
        }
    }

    void ParseEntityMethod(std::deque<Tokenzier::Token>& tokens)
    {
        // Starts with fn keyword
        ExpectText("fn", NextToken(tokens));

        // Function name
        Tokenzier::Token text = NextToken(tokens);
        ExpectTokenType(Tokenzier::eTokenType::eText, text);
      
        std::cout << "Entity function: " << text.AsString().c_str() << std::endl;

        // Brackets end function name
        ExpectToken(Tokenzier::eTokenType::eOpenBracket, tokens);
        ExpectToken(Tokenzier::eTokenType::eCloseBracket, tokens);

        //  Body within { } 's,
        ExpectToken(Tokenzier::eTokenType::eOpenCurlyBracket, tokens);

        // handle empty {}'s case
        if (PeekTokenType(tokens) != Tokenzier::eTokenType::eCloseCurlyBracket)
        {
            ParseEntityMethodBody(tokens);
        }
        ExpectToken(Tokenzier::eTokenType::eCloseCurlyBracket, tokens);

    }
    
    void ParseEntityMethodBody(std::deque<Tokenzier::Token>& tokens)
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
            ExpectTokenType(Tokenzier::eTokenType::eText, text);

            // Label
            std::string str = text.AsString();
            if (str.back() == ':')
            {
                std::cout << "LABEL:" << str.c_str() << std::endl;
            }
            // instruction/mnemonic
            else
            {
                std::cout << "INSTRUCTION:" << str.c_str() << std::endl;
                ParseInstruction(text, tokens);
            }
        }
    }

    void ParseInstruction(const Tokenzier::Token& inst, std::deque<Tokenzier::Token>& tokens)
    {
        const auto insts = FF7::FieldInstructions();
        auto it = insts.find(inst.AsString());
        if (it == std::end(insts))
        {
            throw Exception(inst.AsString() + " is not a known instruction", inst);
        }

        // TODO: Handle arguments correctly, validate labels
        const FF7::TInstructRecord* rec = it->second;
        const char* fmt = rec->mArgumentFormat;
        while (*fmt)
        {
            bool handled = false;
            switch (*fmt)
            {
            case 'B':
                ExpectTokenType(Tokenzier::eTokenType::eNumber, NextToken(tokens));
                handled = true;
                break;

            case 'U':
                ExpectTokenType(Tokenzier::eTokenType::eNumber, NextToken(tokens));
                handled = true;
                break;

            default:
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

    }

    std::string ExpectQuotedString(std::deque<Tokenzier::Token>& tokens)
    {
        ExpectToken(Tokenzier::eTokenType::eQuote, tokens);
        Tokenzier::Token text = NextToken(tokens);
        ExpectTokenType(Tokenzier::eTokenType::eText, text);
        ExpectToken(Tokenzier::eTokenType::eQuote, tokens);
        return text.AsString();
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
            throw Exception("Unknown token type: " + std::to_string(static_cast<int>(type)));
        }
        return ret;
    }

    Tokenzier mTokenzier;
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
    ASSERT_THROW(p.Parse(), Parser::Exception);
}

// Parse not enough arguments
// Parse too many arguments
// Parse duplicate entity name
// Parse duplicate function name
// Parse too many entities
// Parse too many functions
// Parse IF's with missing labels
// Parse generates warning on missing label

// TODO: Assembler

TEST(FF7Field, Asm)
{
    DummyFormatter dummy;
    FF7::FF7FieldEngine eng(dummy);

    InstVec insts;
    FF7::FF7Disassembler d(dummy, &eng, insts);


}

