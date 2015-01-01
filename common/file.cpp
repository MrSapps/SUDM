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

#include "file.h"
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <deque>
#include <algorithm>
#include <sys/stat.h>   // for stat()
#include <sys/types.h>


namespace Common {

// File interface
// While this does massive duplication of the code above, it's required to make sure that
// unconverted tools are backwards-compatible
File::File() {
	_file = NULL;
	_mode = FILEMODE_READ;
}

File::~File() {
	close();
}

void File::open(const std::string& filepath, const char *mode) {

	// Clean up previously opened file
	close();

	_file = fopen(filepath.c_str(), mode);
    _name = filepath;

	FileMode m = FILEMODE_READ;
	do {
		switch(*mode) {
		case 'w': m = FILEMODE_WRITE; break;
		case 'r': m = FILEMODE_READ; break;
		case 'b': m = FileMode(m | FILEMODE_BINARY); break;
		case '+': m = FileMode(m | FILEMODE_READ | FILEMODE_WRITE); break;
		default: throw FileException(std::string("Unsupported FileMode ") + mode);
		}
	} while (*++mode);
	_mode = m;

	if (!_file)
        throw FileException("Could not open file " + filepath);
}

void File::close() {
	if (_file)
		fclose(_file);
	_file = NULL;
}


int File::readChar() {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_READ) == 0)
		throw FileException("Tried to read from file opened in write mode (" + _name + ")");

	int u8 = fgetc(_file);
	if (u8 == EOF)
		throw FileException("Read beyond the end of file (" + _name + ")");
	return u8;
}

uint8 File::readByte() {
	int u8 = readChar();
	return (uint8)u8;
}

uint16 File::readUint16BE() {
	uint16 ret = 0;
	ret |= uint16(readByte() << 8ul);
	ret |= uint16(readByte());
	return ret;
}

uint16 File::readUint16LE() {
	uint16 ret = 0;
	ret |= uint16(readByte());
	ret |= uint16(readByte() << 8ul);
	return ret;
}

uint32 File::readUint32BE() {
	uint32 ret = 0;
	ret |= uint32(readByte() << 24);
	ret |= uint32(readByte() << 16);
	ret |= uint32(readByte() << 8);
	ret |= uint32(readByte());
	return ret;
}

uint32 File::readUint32LE() {
	uint32 ret = 0;
	ret |= uint32(readByte());
	ret |= uint32(readByte() << 8);
	ret |= uint32(readByte() << 16);
	ret |= uint32(readByte() << 24);
	return ret;
}

int16 File::readSint16BE() {
	int16 ret = 0;
	ret |= int16(readByte() << 8ul);
	ret |= int16(readByte());
	return ret;
}

int16 File::readSint16LE() {
	int16 ret = 0;
	ret |= int16(readByte());
	ret |= int16(readByte() << 8ul);
	return ret;
}

int32 File::readSint32BE() {
	int32 ret = 0;
	ret |= int32(readByte() << 24);
	ret |= int32(readByte() << 16);
	ret |= int32(readByte() << 8);
	ret |= int32(readByte());
	return ret;
}

int32 File::readSint32LE() {
	int32 ret = 0;
	ret |= int32(readByte());
	ret |= int32(readByte() << 8);
	ret |= int32(readByte() << 16);
	ret |= int32(readByte() << 24);
	return ret;
}

void File::read_throwsOnError(void *dataPtr, size_t dataSize) {
	size_t data_read = read_noThrow(dataPtr, dataSize);
	if (data_read != dataSize)
		throw FileException("Read beyond the end of file (" + _name + ")");
}

size_t File::read_noThrow(void *dataPtr, size_t dataSize) {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_READ) == 0)
		throw FileException("Tried to read from file opened in write mode (" + _name + ")");

	return fread(dataPtr, 1, dataSize, _file);
}

std::string File::readString() {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_READ) == 0)
		throw FileException("Tried to read from file opened in write mode (" + _name + ")");

	std::string s;
	try {
		char c;
		while ((c = readByte())) {
			s += c;
		}
	} catch (FileException &) {
		// pass, we reached EOF
	}

	return s;
}

std::string File::readString(size_t len) {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_READ) == 0)
		throw FileException("Tried to read from file opened in write mode (" + _name + ")");

	std::string s('\0', len);
	std::string::iterator is = s.begin();

	char c;
	while ((c = readByte())) {
		*is = c;
	}

	return s;
}

void File::scanString(char *result) {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_READ) == 0)
		throw FileException("Tried to write to file opened in read mode (" + _name + ")");

	fscanf(_file, "%s", result);
}

void File::writeChar(char i) {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_WRITE) == 0)
		throw FileException("Tried to write to a file opened in read mode (" + _name + ")");

	if (fwrite(&i, 1, 1, _file) != 1)
		throw FileException("Could not write to file (" + _name + ")");
}

void File::writeByte(uint8 b) {
	writeChar(b);
}

void File::writeUint16BE(uint16 value) {
	writeByte((uint8)(value >> 8));
	writeByte((uint8)(value));
}

void File::writeUint16LE(uint16 value) {
	writeByte((uint8)(value));
	writeByte((uint8)(value >> 8));
}

void File::writeUint32BE(uint32 value) {
	writeByte((uint8)(value >> 24));
	writeByte((uint8)(value >> 16));
	writeByte((uint8)(value >> 8));
	writeByte((uint8)(value));
}

void File::writeUint32LE(uint32 value) {
	writeByte((uint8)(value));
	writeByte((uint8)(value >> 8));
	writeByte((uint8)(value >> 16));
	writeByte((uint8)(value >> 24));
}

size_t File::write(const void *dataPtr, size_t dataSize) {
	if (!_file)
		throw FileException("File is not open");
	if ((_mode & FILEMODE_WRITE) == 0)
		throw FileException("Tried to write to file opened in read mode (" + _name + ")");

	size_t data_read = fwrite(dataPtr, 1, dataSize, _file);
	if (data_read != dataSize)
		throw FileException("Could not write to file (" + _name + ")");

	return data_read;
}

void File::seek(long offset, int origin) {
	if (!_file)
		throw FileException("File is not open");

	if (fseek(_file, offset, origin) != 0)
		throw FileException("Could not seek in file (" + _name + ")");
}

void File::rewind() {
	return ::rewind(_file);
}

int File::pos() const {
	return ftell(_file);
}

int File::err() const {
	return ferror(_file);
}

void File::clearErr() {
	clearerr(_file);
}

bool File::eos() const {
	return feof(_file) != 0;
}

uint32 File::size() const {
	uint32 sz;
	uint32 p = ftell(_file);
	fseek(_file, 0, SEEK_END);
	sz = ftell(_file);
	fseek(_file, p, SEEK_SET);
	return sz;
}


} // End of namespace Common

