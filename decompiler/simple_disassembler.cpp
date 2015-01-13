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

#include "simple_disassembler.h"

SimpleDisassembler::SimpleDisassembler(InstVec &insts) 
    : Disassembler(insts) 
{

}

static inline unsigned int Nib2(unsigned int v)
{
    return (v & 0xF);
}

static inline unsigned int Nib1(unsigned int v)
{
    return (v >> 4) & 0xF;
}


void SimpleDisassembler::readParams(InstPtr inst, const char *typeString)
{
    // Handle [] blocks as working on an individual element (i.e a BYTE,WORD etc)
    // this syntax allows picking of nibbles and bit fields into their own parameters.

    while (*typeString)
    {
        boost::string_ref typeStr = boost::string_ref(typeString, 1);
        if (typeStr == "N") // Read nibbles 
        {
            const uint8 byte = mStream->ReadU8();

            inst->_params.push_back(new IntValue(Nib1(byte), false));
            inst->_params.push_back(new IntValue(Nib2(byte), false));
            _address++;
        }
        else
        {
            inst->_params.push_back(readParameter(inst, typeStr));
        }
        typeString++;
    }
}

ValuePtr SimpleDisassembler::readParameter(InstPtr inst, boost::string_ref type) {
    ValuePtr retval = NULL;

    if (type == "b") // signed byte
    {
        retval = new IntValue(mStream->ReadS8(), true);
        _address++;
    }
    else if (type == "B") // unsigned byte
    {
        retval = new IntValue((uint32)mStream->ReadU8(), false);
        _address++;
    }
    else if (type == "s") // 16-bit signed integer (short), little-endian
    {
        retval = new IntValue(mStream->ReadS16(), true);
        _address += 2;
    }
    else if (type == "S") // 16-bit signed integer (short), big-endian
    {
        abort();
        retval = new IntValue(mStream->ReadS16BE(), true);
        _address += 2;
    }
    else if (type == "w") // 16-bit unsigned integer (word), little-endian
    {
        retval = new IntValue((uint32)mStream->ReadU16(), false);
        _address += 2;
    }
    else if (type == "W") // 16-bit unsigned integer (word), big-endian
    {
        abort();
        retval = new IntValue((uint32)mStream->ReadU16BE(), false);
        _address += 2;
    }
    else if (type == "i") // 32-bit signed integer (int), little-endian
    {
        retval = new IntValue(mStream->ReadS32(), true);
        _address += 4;
    }
    else if (type == "I") // 32-bit signed integer (int), big-endian
    {
        abort();
        retval = new IntValue(mStream->ReadS32BE(), true);
        _address += 4;
    }
    else if (type == "d") // 32-bit unsigned integer (dword), little-endian
    {
        retval = new IntValue(mStream->ReadU32(), false);
        _address += 4;
    }
    else if (type == "D") // 32-bit unsigned integer (dword), big-endian
    {
        abort();
        retval = new IntValue(mStream->ReadU32BE(), false);
        _address += 4;
    }
    else
    {
        throw UnknownOpcodeParameterException(type);
    }
    return retval;
}

