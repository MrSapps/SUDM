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

SimpleDisassembler::SimpleDisassembler(InstVec &insts) : Disassembler(insts) {
}

void SimpleDisassembler::readParams(InstPtr inst, const char *typeString) {
	while (*typeString) {
		inst->_params.push_back(readParameter(inst, *typeString));
		typeString++;
	}
}

ValuePtr SimpleDisassembler::readParameter(InstPtr inst, char type) {
	ValuePtr retval = NULL;
	switch (type) {
	case 'b': // signed byte
        retval = new IntValue(mStream->ReadS8(), true);
        _address++;
		break;
	case 'B': // unsigned byte
        retval = new IntValue((uint32)mStream->ReadU8(), false);
		_address++;
		break;
	case 's': // 16-bit signed integer (short), little-endian
        retval = new IntValue(mStream->ReadS16(), true);
		_address += 2;
		break;
	case 'S': // 16-bit signed integer (short), big-endian
        abort();
        retval = new IntValue(mStream->ReadS16BE(), true);
		_address += 2;
		break;
	case 'w': // 16-bit unsigned integer (word), little-endian
        retval = new IntValue((uint32)mStream->ReadU16(), false);
		_address += 2;
		break;
	case 'W': // 16-bit unsigned integer (word), big-endian
        abort();
        retval = new IntValue((uint32)mStream->ReadU16BE(), false);
		_address += 2;
		break;
	case 'i': // 32-bit signed integer (int), little-endian
        retval = new IntValue(mStream->ReadS32(), true);
		_address += 4;
		break;
	case 'I': // 32-bit signed integer (int), big-endian
        abort();
        retval = new IntValue(mStream->ReadS32BE(), true);
		_address += 4;
		break;
	case 'd': // 32-bit unsigned integer (dword), little-endian
        retval = new IntValue(mStream->ReadU32(), false);
		_address += 4;
		break;
	case 'D': // 32-bit unsigned integer (dword), big-endian
        abort();
        retval = new IntValue(mStream->ReadU32BE(), false);
		_address += 4;
		break;

    default:
        std::cerr << "unknown parameter string" << std::endl;
        break;
	}
	return retval;
}
