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

#include "decompiler_codegen.h"
#include "decompiler_engine.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <boost/format.hpp>
#include "make_unique.h"

#define GET(vertex)    (boost::get(boost::vertex_name, _g, vertex))
#define GET_EDGE(edge) (boost::get(boost::edge_attribute, _g, edge))

void CodeGenerator::onBeforeStartFunction(const Function&)
{
}

void CodeGenerator::onEndFunction(const Function &)
{
    addOutputLine("}", true, false);
}

std::string CodeGenerator::constructFuncSignature(const Function &)
{
    return "";
}

std::string CodeGenerator::indentString(std::string s)
{
    std::stringstream stream;
    stream << std::string(kIndentAmount * _indentLevel, ' ') << s;
    return stream.str();
}

CodeGenerator::CodeGenerator(Engine *engine, std::ostream &output, ArgOrder binOrder, ArgOrder callOrder)
   : _output(output),
    _binOrder(binOrder),
    _callOrder(callOrder)
{
    _engine = engine;
    _indentLevel = 0;
    mTargetLang = std::make_unique<CTargetLanguage>();
}

typedef std::pair<GraphVertex, ValueStack> DFSEntry;

void CodeGenerator::generatePass(InstVec& insts, const Graph& g)
{
    _g = g;
    for (FuncMap::iterator fn = _engine->_functions.begin(); fn != _engine->_functions.end(); ++fn)
    {
        while (!_stack.empty())
        {
            _stack.pop();
        }
        GraphVertex entryPoint = fn->second._v;
        std::string funcSignature = constructFuncSignature(fn->second);

        // Write the function start
        bool printFuncSignature = !funcSignature.empty();
        if (printFuncSignature)
        {
            mCurGroup = GET(entryPoint);
            if (!(fn == _engine->_functions.begin()))
            {
                 addOutputLine("");
            }
            onBeforeStartFunction(fn->second);

            addOutputLine(funcSignature, false, true); 

            onStartFunction(fn->second);
        }

        GroupPtr lastGroup = GET(entryPoint);

        // DFS from entry point to process each vertex
        Stack<DFSEntry> dfsStack;
        std::set<GraphVertex> seen;
        dfsStack.push(DFSEntry(entryPoint, ValueStack()));
        seen.insert(entryPoint);
        while (!dfsStack.empty())
        {
            DFSEntry e = dfsStack.pop();
            GroupPtr tmp = GET(e.first);
            if ((*tmp->_start)->_address > (*lastGroup->_start)->_address)
            {
                lastGroup = tmp;
            }
            _stack = e.second;
            GraphVertex v = e.first;
            process(fn->second, insts, v);
            OutEdgeRange r = boost::out_edges(v, _g);
            for (OutEdgeIterator i = r.first; i != r.second; ++i)
            {
                GraphVertex target = boost::target(*i, _g);
                if (seen.find(target) == seen.end())
                {
                    dfsStack.push(DFSEntry(target, _stack));
                    seen.insert(target);
                }
            }
        }

        // Write the function end
        if (printFuncSignature)
        {
            mCurGroup = lastGroup;
            onEndFunction(fn->second);
        }

        // Print output
        GroupPtr p = GET(entryPoint);
        while (p != NULL)
        {
            for (auto it = p->_code.begin(); it != p->_code.end(); ++it)
            {
                if (it->_unindentBefore)
                {
                    assert(_indentLevel > 0);
                    _indentLevel--;
                }

                if (OutputOnlyRequiredLabels())
                {
                    _output << indentString(it->_line) << std::endl;
                }
                else
                {
                    _output << boost::format("%08X: %s") % (*p->_start)->_address % indentString(it->_line) << std::endl;
                }

                if (it->_indentAfter)
                {
                    _indentLevel++;
                }
            }
            p = p->_next;
        }
    }
}

void CodeGenerator::generate(InstVec& insts, const Graph &g)
{
    if (OutputOnlyRequiredLabels())
    {
        // Call twice, once where no output is generated but instructions are
        // marked as "needs label", then a 2nd time to actually output the code
        mIsLabelPass = true;
        generatePass(insts, g);
    }
    mIsLabelPass = false;
    generatePass(insts, g);
}

void CodeGenerator::addOutputLine(std::string s, bool unindentBefore, bool indentAfter) 
{
    // We don't generate output in the labels pass, we just find instructions that
    // require a label to be outputted
    if (!mIsLabelPass)
    {
        mCurGroup->_code.push_back(CodeLine(s, unindentBefore, indentAfter));
    }
}

void CodeGenerator::writeAssignment(ValuePtr dst, ValuePtr src) 
{
    std::stringstream s;
    s << dst << " = " << src << mTargetLang->LineTerminator();
    addOutputLine(s.str());
}

void CodeGenerator::process(Function& func, InstVec& insts, GraphVertex v)
{
    _curVertex = v;
    mCurGroup = GET(v);

    // Check if we should add else start
    if (mCurGroup->_startElse)
    {
        addOutputLine(mTargetLang->EndBlock(ITargetLanaguge::eToElseBlock) + " " + mTargetLang->Else() + " " + mTargetLang->StartBlock(ITargetLanaguge::eBeginElse), true, true);
    }

    // Check ingoing edges to see if we want to add any extra output
    InEdgeRange ier = boost::in_edges(v, _g);
    for (InEdgeIterator ie = ier.first; ie != ier.second; ++ie)
    {
        GraphVertex in = boost::source(*ie, _g);
        GroupPtr inGroup = GET(in);

        if (!boost::get(boost::edge_attribute, _g, *ie)._isJump || inGroup->_stackLevel == -1)
        {
            continue;
        }

        switch (inGroup->_type)
        {
        case kDoWhileCondGroupType:
            addOutputLine(mTargetLang->DoLoopHeader(), false, true);
            break;
        case kIfCondGroupType:
            if (!mCurGroup->_startElse)
            {
                addOutputLine(mTargetLang->EndBlock(ITargetLanaguge::eEndOfIf), true, false);
            }
            break;
        case kWhileCondGroupType:
            addOutputLine(mTargetLang->EndBlock(ITargetLanaguge::eEndOfWhile), true, false);
            break;
        default:
            break;
        }
    }

    ConstInstIterator it = mCurGroup->_start;
    do 
    {
        // If we only want to write labels that targets of goto's then check if this is the pass
        // after we've setup mLabelRequired on each instruction. If this is set then it needs a label
        // so write one out.
        if (OutputOnlyRequiredLabels() && !mIsLabelPass && (*it)->mLabelRequired)
        {
            addOutputLine(mTargetLang->Label((*it)->_address));
        }
        processInst(func, insts, *it);
    } while (it++ != mCurGroup->_end);

    // Add else end if necessary
    for (ElseEndIterator elseIt = mCurGroup->_endElse.begin(); elseIt != mCurGroup->_endElse.end(); ++elseIt)
    {
        if (!(*elseIt)->_coalescedElse)
        {
            addOutputLine(mTargetLang->EndBlock(ITargetLanaguge::eEndIfElseChain), true, false);
        }
    }
}

void CodeGenerator::processUncondJumpInst(Function& func, InstVec& insts, const InstPtr inst)
{
    switch (mCurGroup->_type)
    {
    case kBreakGroupType:
        addOutputLine(mTargetLang->LoopBreak());
        break;
    case kContinueGroupType:
        addOutputLine(mTargetLang->LoopContinue());
        break;
    default: // Might be a goto
    {
        bool printJump = true;
        OutEdgeRange jumpTargets = boost::out_edges(_curVertex, _g);
        for (OutEdgeIterator target = jumpTargets.first; target != jumpTargets.second && printJump; ++target)
        {
            Group* next = mCurGroup->_next;
            if (next)
            {
                // Don't output jump to next vertex
                if (boost::target(*target, _g) == next->_vertex)
                {
                    printJump = false;
                    break;
                }

                // Don't output jump if next vertex starts an else block
                if (next->_startElse)
                {
                    printJump = false;
                    break;
                }

                OutEdgeRange targetR = boost::out_edges(boost::target(*target, _g), _g);
                for (OutEdgeIterator targetE = targetR.first; targetE != targetR.second; ++targetE)
                {
                    // Don't output jump to while loop that has jump to next vertex
                    if (boost::target(*targetE, _g) == next->_vertex)
                    {
                        printJump = false;
                    }
                }

                if (printJump)
                {
                    // Check if this instruction is the last instruction in the function
                    // and its an uncond jump
                    if (mCurGroup->_type == kDoWhileCondGroupType && inst->_address == func.mEndAddr && inst->isUncondJump())
                    {
                        printJump = false;
                        addOutputLine(mTargetLang->DoLoopFooter(true) + "true" + mTargetLang->DoLoopFooter(false), true, false);
                    }
                }
            }
        }

        if (printJump)
        {
            const uint32 dstAddr = inst->getDestAddress();
            if (mIsLabelPass)
            {
                // Mark the goto target
                for (auto& i : insts)
                {
                    if (i->_address == dstAddr)
                    {
                        i->mLabelRequired = true;
                        break;
                    }
                }
            }
            addOutputLine(mTargetLang->Goto(dstAddr));
        }
    }
        break;
    }
}

void CodeGenerator::writeFunctionCall(std::string functionName, std::string paramsFormat, const std::vector<ValuePtr>& params)
{
    std::string strFuncCall = functionName + mTargetLang->FunctionCallBegin();
    const char* str = paramsFormat.c_str();
    int paramIndex = 0;
    while (*str)
    {
        bool skipArgument = false;
        switch (*str)
        {
        case 'b':
            strFuncCall += params[paramIndex]->getUnsigned() ? "true" : "false";
            break;

        case 'n':
            strFuncCall += std::to_string(params[paramIndex]->getUnsigned());
            break;

        case 'f':
            strFuncCall += std::to_string(static_cast<float>(params[paramIndex]->getUnsigned()) / 30.0f);
            break;

        case '_': // Ignore param
            skipArgument = true;
            break;

        default:
            throw std::runtime_error("Unknown param type");
            break;
        }
        paramIndex++;
        str++;
        if (*str)
        {
            // There is another param
            if (!skipArgument)
            {
                strFuncCall += mTargetLang->FunctionCallArgumentSeperator() + " ";
            }
        }
    }
    strFuncCall += mTargetLang->FunctionCallEnd();
    addOutputLine(strFuncCall);
}

void CodeGenerator::processCondJumpInst(const InstPtr inst)
{
    std::stringstream s;
    switch (mCurGroup->_type)
    {
    case kIfCondGroupType:
        if (mCurGroup->_startElse && mCurGroup->_code.size() == 1)
        {
            OutEdgeRange oer = boost::out_edges(_curVertex, _g);
            bool coalesceElse = false;
            for (OutEdgeIterator oe = oer.first; oe != oer.second; ++oe)
            {
                GroupPtr oGr = GET(boost::target(*oe, _g))->_prev;
                if (std::find(oGr->_endElse.begin(), oGr->_endElse.end(), mCurGroup.get()) != oGr->_endElse.end())
                {
                    coalesceElse = true;
                }
            }
            if (coalesceElse)
            {
                mCurGroup->_code.clear();
                mCurGroup->_coalescedElse = true;
                s << mTargetLang->EndBlock(ITargetLanaguge::eToElseBlock) << " " << mTargetLang->Else() << " ";
            }
        }
        s << mTargetLang->If(true) << _stack.pop()->negate() << mTargetLang->If(false);
        addOutputLine(s.str(), mCurGroup->_coalescedElse, true);
        break;
    case kWhileCondGroupType:
        s << mTargetLang->WhileHeader(true) << _stack.pop()->negate() << mTargetLang->WhileHeader(false) << " " << mTargetLang->StartBlock(ITargetLanaguge::eBeginWhile);
        addOutputLine(s.str(), false, true);
        break;
    case kDoWhileCondGroupType:
        s << mTargetLang->EndBlock(ITargetLanaguge::eEndWhile) <<  " " << mTargetLang->WhileHeader(true) << _stack.pop() << mTargetLang->WhileHeader(false);
        addOutputLine(s.str(), true, false);
        break;
    default:
        break;
    }
}

void CodeGenerator::processInst(Function& func, InstVec& insts, const InstPtr inst)
{
    inst->processInst(func, _stack, _engine, this);
    if (inst->isCondJump())
    {
        processCondJumpInst(inst);
    }
    else if (inst->isUncondJump())
    {
        processUncondJumpInst(func, insts, inst);
    }
}

void CodeGenerator::addArg(ValuePtr p) 
{
    if (_callOrder == kFIFOArgOrder)
        _argList.push_front(p);
    else if (_callOrder == kLIFOArgOrder)
        _argList.push_back(p);
}

void CodeGenerator::processSpecialMetadata(const InstPtr inst, char c, int) 
{
    switch (c) 
    {
    case 'p':
        addArg(_stack.pop());
        break;
    default:
        std::cerr << boost::format("WARNING: Unknown character in metadata: %c\n") % c;
        break;
    }
}
