/* ScummVM Tools
 *
 * ScummVM Tools is the legal property of its developers, whose
 * names are too numerous to list here. Please refer to the
 * COPYRIGHT file distributed with this source distribution.
 *
 * Additionally this file is based on the ScummVM source code.
 * Copyright information for the ScummVM source code is
 * available in the COPYRIGHT file of the ScummVM source
 * distribution.
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

#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include "common/scummsys.h"
#include <streambuf>
#include <ostream>

#ifdef MIN
#undef MIN
#endif

#ifdef MAX
#undef MAX
#endif

// Define an ostream which doesn't output anything to avoid clutter
// Source: http://groups.google.com/group/comp.lang.c++/msg/4a81a74500f9f4d3?hl=en
template<class cT, class traits = std::char_traits<cT> >
class basic_nullbuf : public std::basic_streambuf < cT, traits > {
    typename traits::int_type overflow(typename traits::int_type c)
    {
        return traits::not_eof(c);
    }
};

template<class cT, class traits = std::char_traits<cT> >
class basic_onullstream : public std::basic_ostream < cT, traits > {
public:
    basic_onullstream() :
        std::basic_ios<cT, traits>(&m_sbuf),
        std::basic_ostream<cT, traits>(&m_sbuf)
    {
        this->init(&m_sbuf);
    }

private:
    basic_nullbuf<cT, traits> m_sbuf;
};

typedef basic_onullstream<char> onullstream;

template<typename T> inline T ABS (T x)			{ return (x>=0) ? x : -x; }
template<typename T> inline T MIN (T a, T b)	{ return (a<b) ? a : b; }
template<typename T> inline T MAX (T a, T b)	{ return (a>b) ? a : b; }
template<typename T> inline T CLIP (T v, T amin, T amax)
		{ if (v < amin) return amin; else if (v > amax) return amax; else return v; }


/**
 * Template method which swaps the vaulues of its two parameters.
 */
template<typename T> inline void SWAP(T &a, T &b) { T tmp = a; a = b; b = tmp; }

/**
 * Macro which determines the number of entries in a fixed size array.
 */
#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

#endif
