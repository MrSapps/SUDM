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

#ifndef DEC_UNKNOWN_OPCODE_H
#define DEC_UNKNOWN_OPCODE_H

#include <exception>
#include <string>
#include "common/scummsys.h"
#include <boost/utility/string_ref.hpp>

class InternalDecompilerError : public std::exception
{
public:

};

class UnknownOpcodeParameterException : public InternalDecompilerError
{
public:
    UnknownOpcodeParameterException(boost::string_ref param)
    {
        mWhat = "unknown opcode parameter string: " + param.to_string();
    }

    virtual const char *what() const throw() override
    {
        return mWhat.c_str();
    }

private:
    std::string mWhat;
};


/**
 * Exception representing an unknown opcode.
 */
class UnknownOpcodeException : public InternalDecompilerError
{
	uint32 _address; ///< Address where the invalid opcode was found.
	uint32 _opcode;   ///< The value of the invalid opcode.
	mutable char _buf[255];  ///< Buffer for formatting the error message.

public:
	/**
	 * Constructor for UnknownOpcodeException.
	 *
	 * @param address Address where the invalid opcode was found.
	 * @param opcode  The value of the invalid opcode.
	 */
    UnknownOpcodeException(uint32 address, uint32 opcode);

	/**
	 * Description of the exception.
	 */
    virtual const char *what() const throw() override;

private:
    virtual const char* Type() const { return "Opcode"; }
};

class UnknownSubOpcodeException : public UnknownOpcodeException
{
public:
    UnknownSubOpcodeException(uint32 address, uint32 opcode)
        : UnknownOpcodeException(address, opcode)
    {

    }
private:
    virtual const char* Type() const override { return "SubOpcode"; }

};

#endif
