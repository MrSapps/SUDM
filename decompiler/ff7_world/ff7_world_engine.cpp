#include "ff7_world_engine.h"
#include "ff7_world_disassembler.h"
#include "ff7_world_codegen.h" 

#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include "make_unique.h"

#define GET(vertex) (boost::get(boost::vertex_name, g, vertex))

std::unique_ptr<Disassembler> FF7::FF7WorldEngine::getDisassembler(InstVec &insts)
{
    return std::make_unique<FF7WorldDisassembler>(this, insts, mScriptNumber);
}

std::unique_ptr<CodeGenerator> FF7::FF7WorldEngine::getCodeGenerator(const InstVec& /*insts*/, std::ostream &output)
{
    return std::make_unique<FF7WorldCodeGenerator>(this, output);
}

void FF7::FF7WorldEngine::postCFG(InstVec&, Graph g)
{
    /*
    VertexRange vr = boost::vertices(g);
    for (VertexIterator v = vr.first; v != vr.second; ++v) 
    {
        GroupPtr gr = GET(*v);

        // If this group is the last instruction and its an unconditional jump
        if ((*gr->_start)->_address == insts.back()->_address && insts.back()->isUncondJump())
        {
            // Then assume its an infinite do { } while(true) loop that wraps part of the script
            gr->_type = kDoWhileCondGroupType;
        }
    }*/
}

void FF7::FF7WorldEngine::getVariants(std::vector<std::string>&) const
{

}

void FF7::FF7WorldLoadBankInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator*)
{
    stack.push(new BankValue("Read(" + _params[0]->getString() + ")"));
}

void FF7::FF7WorldLoadInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator*)
{
    stack.push(new VarValue(_params[0]->getString()));
}

void FF7::FF7SubStackInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator*)
{
    std::string op;
    switch (_opcode)
    {
    case 0x41:
        op = "-";
        break;

    case 0x40:
        op = "+";
        break;

    case 0x80:
        op = "&&";
        break;

    case 0xa0:
        op = "||";
        break;

    case 0xc0:
        op = "|";
        break;

    case 0x15: // neg
    case 0x17: // not
        stack.push(stack.pop()->negate());
        return;

    case 0x30:
        op = "*";
        break;

    case 0x60:
        op = "<";
        break;

    case 0x61:
        op = ">";
        break;

    case 0x62:
        op = "<=";
        break;

    case 0x63:
        op = ">=";
        break;

    case 0x0b0:
        op = "&";
        break;

    case 0x51:
        op = "<<";
        break;

    default:
        op = "unknown_operation";
    }

    std::string strValue = stack.pop()->getString() + " " + op + " " + stack.pop()->getString();
    stack.push(new VarValue(strValue));
}

void FF7::FF7WorldStoreInstruction::processInst(Function&, ValueStack &stack, Engine*, CodeGenerator *codeGen)
{
    std::string strValue = stack.pop()->getString();

    // If the bank address is from a load bank instruction, then we only want
    // the bank address, not whats *at* the bank address
    ValuePtr bankValue = stack.pop();
    std::string bankAddr = bankValue->getString();
    codeGen->addOutputLine("Write(" + bankAddr + ", " + strValue + ");");
}

void FF7::FF7WorldStackInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}

void FF7::FF7WorldCondJumpInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}

uint32 FF7::FF7WorldCondJumpInstruction::getDestAddress() const
{
    return 0x200 + _params[0]->getUnsigned();
}

std::ostream& FF7::FF7WorldCondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
    return output;
}


bool FF7::FF7WorldUncondJumpInstruction::isFuncCall() const
{
	return _isCall;
}

bool FF7::FF7WorldUncondJumpInstruction::isUncondJump() const
{
	return !_isCall;
}

uint32 FF7::FF7WorldUncondJumpInstruction::getDestAddress() const
{
    // the world map while loops are incorrect without +1'in this, but
    // doing that will break some if elses.. hmm
    return 0x200 + _params[0]->getUnsigned();// +1; // TODO: +1 to skip param?
}

std::ostream& FF7::FF7WorldUncondJumpInstruction::print(std::ostream &output) const
{
    Instruction::print(output);
 //   output << " (Jump target address: 0x" << std::hex << getDestAddress() << std::dec << ")";
    return output;
}


void FF7::FF7WorldUncondJumpInstruction::processInst(Function& , ValueStack&, Engine*, CodeGenerator*)
{

}

void FF7::FF7WorldKernelCallInstruction::processInst(Function& , ValueStack &stack, Engine*, CodeGenerator *codeGen)
{
    std::string strFunc;
    switch (_opcode)
    {
    case 0x203:
        strFunc = "return;";
        break;

    case 0x317:
        strFunc = "TriggerBattle(" + stack.pop()->getString() + ");";
        break;

    case 0x324:
        // x, y, w, h
        strFunc = "SetWindowDimensions(" + 
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x32D:
        strFunc = "WaitForWindowReady();";
        break;

    case 0x325:
        strFunc = "SetWindowMessage(" + stack.pop()->getString() + ");";
        break;

    case 0x333:
        strFunc = "Unknown333(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x308:
        strFunc = "SetActiveEntityMeshCoordinates(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x309:
        strFunc = "SetActiveEntityMeshCoordinatesInMesh(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x32e:
        strFunc = "WaitForMessageAcknowledge();";
        break;

    case 0x32c:
        // Mode, Permanency
        strFunc = "SetWindowParameters(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x318:
        strFunc = "EnterFieldScene(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x348:
        strFunc = "FadeIn(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x33b:
        strFunc = "FadeOut(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x310:
        strFunc = "SetActivePoint(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x311:
        strFunc = "SetLightMeshCoordinates(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x312:
        strFunc = "SetLightMeshCoordinatesInMesh(" +
            stack.pop()->getString() + ", " +
            stack.pop()->getString() + ");";
        break;

    case 0x31D:
        strFunc = "PlaySoundEffect(" + stack.pop()->getString() + ");";
        break;

    case 0x328:
        strFunc = "SetActiveEntityDirection(" + stack.pop()->getString() + ");";
        break;

    case 0x336: // honours walk mesh
    case 0x303:
        strFunc = "SetActiveEntityMovespeed(" + stack.pop()->getString() + ");";
        break;

    case 0x304:
        strFunc = "SetActiveEntityDirectionAndFacing(" + stack.pop()->getString() + ");";
        break;

    case 0x32b:
        strFunc = "SetBattleLock(" + stack.pop()->getString() + ");";
        break;

    case 0x305:
        strFunc = "SetWaitFrames(" + stack.pop()->getString() + ");";
        break;

    case 0x33e:
        strFunc = "Unknown_AKAO(" + stack.pop()->getString() + ");";
        break;

    case 0x306:
        strFunc = "Wait();";
        break;

    case 0x350:
        strFunc = "SetMeteorTexture(" + stack.pop()->getString() + ");";
        break;

    case 0x34b:
    {
        std::string type = stack.pop()->getString();
        switch (std::stoi(type))
        {
        case 0:
            type = "yellow";
            break;

        case 1:
            type = "green";
            break;

        case 2:
            type = "blue";
            break;

        case 3:
            type = "black";
            break;

        case 4:
            type = "gold";
            break;
        }
        strFunc = "SetChocoboType(" + type + ");";
    }
        break;

    case 0x34c:
    {
        std::string type = stack.pop()->getString();
        switch (std::stoi(type))
        {
        case 0:
            type = "red";
            break;

        case 1:
            type = "blue";
            break;

        // Hack - not really supported, but works
        case -1:
            type = "gold";
            break;

        case -2:
            type = "black";
            break;
        }
        strFunc = "SetSubmarineColor(" + type + ");";
    }
    break;

    case 0x349:
    {
        std::string type = stack.pop()->getString();
        std::string comment = "// ";
        switch (std::stoi(type))
        {
        case 0:
            comment += "before temple of the ancients,";
            break;

        case 1:
            comment += "after temple of the ancients";
            break;

        case 2:
            comment += " after ultimate weapon appears,";
            break;

        case 3:
            comment += "after mideel";
            break;

        case 4:
            comment += "after ultimate weapon killed";
            break;

        default:
            comment += "??";
            break;
        }
        strFunc = "SetWorldProgress(" + type + "); "  +comment;
    }
        break;

    case 0x300:
    {
        std::string type = stack.pop()->getString();
        std::string comment = "// ";
        try
        {
            switch (std::stoi(type))
            {
            case 0: comment += "Cloud"; break;
            case 1: comment += "Tifa?"; break;
            case 2: comment += "Cid?"; break;
            case 3: comment += "Ultimate Weapon"; break;
            case 4: comment += "Unknown"; break;
            case 5: comment += "Unknown"; break;
            case 6: comment += "Unknown"; break;
            case 7: comment += "Unknown"; break;
            case 8: comment += "Unknown"; break;
            case 9: comment += "Unknown"; break;
            case 10: comment += "Unknown"; break;
            case 11: comment += "Highwind"; break;
            case 12: comment += "Unknown"; break;
            case 13: comment += "Submarine"; break;
            case 14: comment += "Unknown"; break;
            case 15: comment += "Unknown"; break;
            case 16: comment += "Unknown"; break;
            case 17: comment += "Unknown"; break;
            case 18: comment += "Unknown"; break;
            case 19: comment += "Chocobo"; break;
            case 20: comment += "Unknown"; break;
            case 21: comment += "Unknown"; break;
            case 22: comment += "Unknown"; break;
            case 23: comment += "Unknown"; break;
            case 24: comment += "Unknown"; break;
            case 25: comment += "Unknown"; break;

            default:
                comment += "??";
                break;
            }
        }
        catch (std::invalid_argument&)
        {

        }
        strFunc = "LoadModel(" + type + "); " + comment;
    }
        break;

    case 0x307:
        strFunc = "SetControlLock(" + stack.pop()->getString() + ");";
        break;

    case 0x30c:
        strFunc = "EnterVehicle();";
        break;

    default:
        strFunc = "kernel_unknown_" + AddressValue(_opcode).getString() + "();";
        break;
    }
    codeGen->addOutputLine(strFunc);
}

void FF7::FF7WorldNoOutputInstruction::processInst(Function&, ValueStack&, Engine*, CodeGenerator*)
{

}
