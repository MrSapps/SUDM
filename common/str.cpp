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

#include "common/str.h"
#include "common/util.h"

#include <stdarg.h>

#if !defined(__SYMBIAN32__)
#include <new>
#endif


namespace Common {

String::String(const char *str)  {
	initWithCStr(str, strlen(str));
}

String::String(const char *str, uint32 len) {
	initWithCStr(str, len);
}

String::String(const char *beginP, const char *endP) {
	assert(endP >= beginP);
	initWithCStr(beginP, endP - beginP);
}

void String::initWithCStr(const char *str, uint32 len) {
	assert(str);

    _str = std::string(str, len);
}

String::String(const String &str)
{
    _str = str._str;
}

String::String(char c)
{
    _str += c;
}

String::~String() 
{

}

String& String::operator  =(const char *str) {
    _str = std::string(str);
	return *this;
}

String &String::operator  =(const String &str) {
    if (&str == this)
        return *this;

    _str = str._str;

    return *this;
}

String& String::operator  =(char c) {
    _str.clear();
    _str += c;
	return *this;
}

String &String::operator +=(const char *str) {
    _str += std::string(str);
	return *this;
}

String &String::operator +=(const String &str) {
	if (&str == this)
		return operator+=(Common::String(str));
    _str += str._str;
	return *this;
}

String &String::operator +=(char c) {
    _str += c;
	return *this;
}

bool String::hasPrefix(const String &x) const {
	return hasPrefix(x.c_str());
}

bool String::hasPrefix(const char *x) const {
	assert(x != 0);
	// Compare x with the start of _str.
	const char *y = c_str();
	while (*x && *x == *y) {
		++x;
		++y;
	}
	// It's a prefix, if and only if all letters in x are 'used up' before
	// _str ends.
	return *x == 0;
}

bool String::hasSuffix(const String &x) const {
	return hasSuffix(x.c_str());
}

bool String::hasSuffix(const char *x) const {
	assert(x != 0);
	// Compare x with the end of _str.
	const uint32 x_size = strlen(x);
	if (x_size > _str.size())
		return false;
    const char *y = c_str() + _str.size() - x_size;
	while (*x && *x == *y) {
		++x;
		++y;
	}
	// It's a suffix, if and only if all letters in x are 'used up' before
	// _str ends.
	return *x == 0;
}

bool String::contains(const String &x) const {
	return strstr(c_str(), x.c_str()) != NULL;
}

bool String::contains(const char *x) const {
	assert(x != 0);
	return strstr(c_str(), x) != NULL;
}

bool String::contains(char x) const {
	return strchr(c_str(), x) != NULL;
}

bool String::matchString(const char *pat, bool ignoreCase, bool pathMode) const {
	return Common::matchString(c_str(), pat, ignoreCase, pathMode);
}

bool String::matchString(const String &pat, bool ignoreCase, bool pathMode) const {
	return Common::matchString(c_str(), pat.c_str(), ignoreCase, pathMode);
}

void String::clear() {
    _str.clear();
}

void String::setChar(char c, uint32 p) {
    assert(p <= _str.size());
	_str[p] = c;
}

void String::insertChar(char c, uint32 p) {
    assert(p <= _str.size());

    _str += c;
	for (auto i = _str.size(); i > p; --i)
		_str[i] = _str[i-1];
	_str[p] = c;
}

void String::toLowercase() {
    for (auto i = 0u; i < _str.size(); ++i)
		_str[i] = static_cast<char>(tolower(_str[i]));
}

void String::toUppercase() {
	for (auto i = 0u; i < _str.size(); ++i)
		_str[i] = static_cast<char>(toupper(_str[i]));
}

bool String::operator ==(const String &x) const {
	return equals(x);
}

bool String::operator ==(const char *x) const {
	assert(x != 0);
	return equals(x);
}

bool String::operator !=(const String &x) const {
	return !equals(x);
}

bool String::operator !=(const char *x) const {
	assert(x != 0);
	return !equals(x);
}

bool String::operator < (const String &x) const {
	return compareTo(x) < 0;
}

bool String::operator <= (const String &x) const {
	return compareTo(x) <= 0;
}

bool String::operator > (const String &x) const {
	return compareTo(x) > 0;
}

bool String::operator >= (const String &x) const {
	return compareTo(x) >= 0;
}

bool operator == (const char* y, const String &x) {
	return (x == y);
}

bool operator != (const char* y, const String &x) {
	return x != y;
}

bool String::equals(const String &x) const {
	return (0 == compareTo(x));
}

bool String::equals(const char *x) const {
	assert(x != 0);
	return (0 == compareTo(x));
}

bool String::equalsIgnoreCase(const String &x) const {
	return (0 == compareToIgnoreCase(x));
}

bool String::equalsIgnoreCase(const char *x) const {
	assert(x != 0);
	return (0 == compareToIgnoreCase(x));
}

int String::compareTo(const String &x) const {
	return compareTo(x.c_str());
}

int String::compareTo(const char *x) const {
	assert(x != 0);
	return strcmp(c_str(), x);
}

int String::compareToIgnoreCase(const String &x) const {
	return compareToIgnoreCase(x.c_str());
}

int String::compareToIgnoreCase(const char *x) const {
	assert(x != 0);
	return scumm_stricmp(c_str(), x);
}

String operator +(const String &x, const String &y) {
	String temp(x);
	temp += y;
	return temp;
}

String operator +(const char *x, const String &y) {
	String temp(x);
	temp += y;
	return temp;
}

String operator +(const String &x, const char *y) {
	String temp(x);
	temp += y;
	return temp;
}

String operator +(char x, const String &y) {
	String temp(x);
	temp += y;
	return temp;
}

String operator +(const String &x, char y) {
	String temp(x);
	temp += y;
	return temp;
}

char *ltrim(char *t) {
	while (isspace(*t))
		t++;
	return t;
}

char *rtrim(char *t) {
	int l = strlen(t) - 1;
	while (l >= 0 && isspace(t[l]))
		t[l--] = 0;
	return t;
}

char *trim(char *t) {
	return rtrim(ltrim(t));
}

Common::String lastPathComponent(const Common::String &path, const char sep) {
	const char *str = path.c_str();
	const char *last = str + path.size();

	// Skip over trailing slashes
	while (last > str && *(last-1) == sep)
		--last;

	// Path consisted of only slashes -> return empty string
	if (last == str)
		return Common::String();

	// Now scan the whole component
	const char *first = last - 1;
	while (first >= str && *first != sep)
		--first;

	if (*first == sep)
		first++;

	return Common::String(first, last);
}

Common::String normalizePath(const Common::String &path, const char sep) {
	if (path.empty())
		return path;

	const char *cur = path.c_str();
	Common::String result;

	// If there is a leading slash, preserve that:
	if (*cur == sep) {
		result += sep;
		while (*cur == sep)
			++cur;
	}

	// Scan till the end of the String
	while (*cur != 0) {
		const char *start = cur;

		// Scan till the next path separator resp. the end of the string
		while (*cur != sep && *cur != 0)
			cur++;

		const Common::String component(start, cur);

		// Skip empty components and dot components, add all others
		if (!component.empty() && component != ".") {
			// Add a separator before the component, unless the result
			// string already ends with one (which happens only if the
			// path *starts* with a separator).
			if (!result.empty() && result.lastChar() != sep)
				result += sep;

			// Add the component
			result += component;
		}

		// Skip over separator chars
		while (*cur == sep)
			cur++;
	}

	return result;
}

bool matchString(const char *str, const char *pat, bool ignoreCase, bool pathMode) {
	assert(str);
	assert(pat);

	const char *p = 0;
	const char *q = 0;

	for (;;) {
		if (pathMode && *str == '/') {
			p = 0;
			q = 0;
			if (*pat == '?')
				return false;
		}

		switch (*pat) {
		case '*':
			// Record pattern / string position for backtracking
			p = ++pat;
			q = str;
			// If pattern ended with * -> match
			if (!*pat)
				return true;
			break;

		default:
			if ((!ignoreCase && *pat != *str) ||
				(ignoreCase && tolower(*pat) != tolower(*str))) {
				if (p) {
					// No match, oops -> try to backtrack
					pat = p;
					str = ++q;
					if (!*str)
						return !*pat;
					break;
				}
				else
					return false;
			}
			// fallthrough
		case '?':
			if (!*str)
				return !*pat;
			pat++;
			str++;
		}
	}
}

}	// End of namespace Common
