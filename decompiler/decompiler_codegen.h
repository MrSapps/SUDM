#pragma once

/* ScummVM Tools
 *
 * ScummVM Tools is the legal property of its developers, whose
 * names are too numerous to list here. Please refer to the
 * COPYRIGHT file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "graph.h"
#include "value.h"

#include <ostream>
#include <utility>

#include <boost/intrusive_ptr.hpp>
#include <memory>


class Engine;

class Function;

const int kIndentAmount = 2; ///< How many spaces to use for each indent.

/**
 * Enumeration for the different argument/operand orderings.
 */
enum ArgOrder {
	kFIFOArgOrder, ///< First argument is pushed to stack first.
	kLIFOArgOrder  ///< First argument is pushed to stack last.
};

class ITargetLanaguge
{
public:
    virtual ~ITargetLanaguge() = default;
    virtual std::string LoopBreak() = 0;
    virtual std::string LoopContinue() = 0;
    virtual std::string Goto(uint32 target) = 0;
    virtual std::string DoLoopHeader() = 0;
    virtual std::string DoLoopFooter(bool beforeExpr) = 0;
    virtual std::string If(bool beforeExpr) = 0;
    virtual std::string WhileHeader(bool beforeExpr) = 0;
    virtual std::string FunctionCallArgumentSeperator() = 0;
    virtual std::string FunctionCallBegin() = 0;
    virtual std::string FunctionCallEnd() = 0;
};

class CTargetLanguage : public ITargetLanaguge
{
public:
    virtual std::string LoopBreak() override
    {
        return "break;";
    }

    virtual std::string LoopContinue() override
    {
        return "continue;";
    }

    virtual std::string Goto(uint32 target) override
    {
        std::stringstream s;
        s << boost::format("goto label_0x%X;") % target;
        return s.str();
    }

    virtual std::string DoLoopHeader() override
    {
        return "do {";
    }

    virtual std::string DoLoopFooter(bool beforeExpr) override
    {
        if (beforeExpr)
        {
            return " } while (";
        }
        return ");";
    }

    virtual std::string If(bool beforeExpr) override
    {
        if (beforeExpr)
        {
            return "if (";
        }
        return ") {";
    }

    virtual std::string WhileHeader(bool beforeExpr) override
    {
        if (beforeExpr)
        {
            return "while (";
        }
        return ") {";
    }

    virtual std::string FunctionCallArgumentSeperator() override
    {
        return ",";
    }
    
    virtual std::string FunctionCallBegin() override
    {
        return "(";
    }
    
    virtual std::string FunctionCallEnd() override
    {
        return ");";
    }
};

/**
 * Base class for code generators.
 */
class CodeGenerator {
private:
	Graph _g;                  ///< The annotated graph of the script.

	/**
	 * Processes a GraphVertex.
	 *
	 * @param v The vertex to process.
	 */
	void process(GraphVertex v);

protected:
	Engine *_engine;        ///< Pointer to the Engine used for the script.
	std::ostream &_output;  ///< The std::ostream to output the code to.
	ValueStack _stack;      ///< The stack currently being processed.
	uint _indentLevel;      ///< Indentation level.
	GraphVertex _curVertex; ///< Graph vertex currently being processed.
    std::unique_ptr<ITargetLanaguge> mTargetLang;

	/**
	 * Processes an instruction. Called by process() for each instruction.
	 * Call the base class implementation for opcodes you cannot handle yourself,
	 * or where the base class implementation is preferable.
	 *
	 * @param inst The instruction to process.
	 */
	void processInst(const InstPtr inst);
    void processUncondJumpInst(const InstPtr inst);
    void processCondJumpInst(const InstPtr inst);

	/**
	 * Indents a string according to the current indentation level.
	 *
	 * @param s The string to indent.
	 * @result The indented string.
	 */
	std::string indentString(std::string s);

	/**
	 * Construct the signature for a function.
	 *
	 * @param func Reference to the function to construct the signature for.
	 */
	virtual std::string constructFuncSignature(const Function &func);

public:
    void writeFunctionCall(std::string functionName, std::string paramsFormat, const std::vector<ValuePtr>& params);

	const ArgOrder _binOrder;  ///< Order of operands for binary operations.
	const ArgOrder _callOrder; ///< Order of operands for call arguments.
	ValueList _argList;        ///< Storage for lists of arguments to be built when processing function calls.
	GroupPtr _curGroup;     ///< Pointer to the group currently being processed.

	virtual ~CodeGenerator() { }

	/**
	 * Constructor for CodeGenerator.
	 *
	 * @param engine Pointer to the Engine used for the script.
	 * @param output The std::ostream to output the code to.
	 * @param binOrder Order of arguments for binary operators.
	 * @param callOrder Order of arguments for function calls.
	 */
	CodeGenerator(Engine *engine, std::ostream &output, ArgOrder binOrder, ArgOrder callOrder);

	/**
	 * Generates code from the provided graph and outputs it to stdout.
	 *
	 * @param g The annotated graph of the script.
	 */
	void generate(const Graph &g);

	/**
	 * Adds a line of code to the current group.
	 *
	 * @param s The line to add.
	 * @param unindentBefore Whether or not to remove an indentation level before the line. Defaults to false.
	 * @param indentAfter Whether or not to add an indentation level after the line. Defaults to false.
	 */
	void addOutputLine(std::string s, bool unindentBefore = false, bool indentAfter = false);

	/**
	 * Generate an assignment statement.
	 *
	 * @param dst The variable being assigned to.
	 * @param src The value being assigned.
	 */
	void writeAssignment(ValuePtr dst, ValuePtr src);

	/**
	 * Add an argument to the argument list.
	 *
	 * @param p The argument to add.
	 */
	void addArg(ValuePtr p);

	/**
	 * Process a single character of metadata.
	 *
	 * @param inst The instruction being processed.
	 * @param c The character signifying the action to be taken.
	 * @param pos The position at which c occurred in the metadata.
	 */
	virtual void processSpecialMetadata(const InstPtr inst, char c, int pos);

};