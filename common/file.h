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

#ifndef COMMON_FILE_H
#define COMMON_FILE_H

#include "common/scummsys.h"
#include "common/noncopyable.h"

#include "tool_exception.h"


namespace Common {

/**
 * Something unexpected happened while reading / writing to a file.
 * Usually premature end, or that it could not be opened (write / read protected).
 */
class FileException : public ToolException {
public:
	FileException(std::string error, int retcode = -1) : ToolException(error, retcode) {}
};


/**
 * Possible modes for opening files
 */
enum FileMode {
	FILEMODE_READ = 1,
	FILEMODE_WRITE = 2,
	FILEMODE_BINARY = 4
};

/**
 * A basic wrapper around the FILE class.
 * Offers functionality to write words easily, and deallocates the FILE
 * automatically on destruction.
 */
class File : public NonCopyable {
public:
	/**
	 * Create an empty file, used for two-step construction.
	 */
	File();
	~File();

	/**
	 * Opens the given file path as an in/out stream, depending on the
	 * second argument.
	 *
	 * @param filename	file to open
	 * @param mode		mode to open the file in
	 */
	void open(const std::string& filename, const char *mode);

	/**
	 * Closes the file, if it's open.
	 */
	void close();

	/**
	 * Check whether the file is open.
	 */
	bool isOpen() const { return _file != 0; }

	/**
	 * Reads a single character (equivalent of fgetc).
	 */
	int readChar();
	/**
	 * Read a single unsigned byte.
	 * @throws FileException if file is not open / if read failed.
	 */
	uint8 readByte();
	/**
	 * Read a single 16-bit word, big endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	uint16 readUint16BE();
	/**
	 * Read a single 16-bit word, little endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	uint16 readUint16LE();
	/**
	 * Read a single 32-bit word, big endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	uint32 readUint32BE();
	/**
	 * Read a single 32-bit word, little endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	uint32 readUint32LE();

	/**
	 * Read a single 16-bit word, big endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	int16 readSint16BE();
	/**
	 * Read a single 16-bit word, little endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	int16 readSint16LE();
	/**
	 * Read a single 32-bit word, big endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	int32 readSint32BE();
	/**
	 * Read a single 32-bit word, little endian.
	 * @throws FileException if file is not open / if read failed.
	 */
	int32 readSint32LE();


	/**
	 * Works the same way as fread, but throws on error or if it could
	 * not read all elements.
	 *
	 * @param dataPtr	pointer to a buffer into which the data is read
	 * @param dataSize	number of bytes to be read
	 */
	void read_throwsOnError(void *dataPtr, size_t dataSize);

	/**
	 * Works the same way as fread, does NOT throw if it could not read all elements
	 * still throws if file is not open.
	 *
	 * @param dataPtr	pointer to a buffer into which the data is read
	 * @param dataSize	number of bytes to be read
	 * @return the number of bytes which were actually read.
	 */
	size_t read_noThrow(void *dataPtr, size_t dataSize);

	/**
	 * Reads a full string, until NULL or EOF.
	 * @throws FileException if file is not open / if read failed.
	 */
	std::string readString();

	/**
	 * Reads the queried amount of bytes and returns it as a string.
	 * @throws FileException if file is not open / if read failed.
	 *
	 * @param len How many bytes to read
	 * @return the data read
	 */
	std::string readString(size_t len);

	/**
	 * Reads a string, using until NULL or EOF or CR/LF.
	 * @throws FileException if file is not open / if read failed.
	 */
	void scanString(char *result);

	/**
	 * Writes a single character (equivalent of fputc).
	 */
	void writeChar(char c);
	/**
	 * Writes a single byte to the file.
	 * @throws FileException if file is not open / if write failed.
	 */
	void writeByte(uint8 b);
	/**
	 * Writes a single 16-bit word to the file, big endian.
	 * @throws FileException if file is not open / if write failed.
	 */
	void writeUint16BE(uint16 value);
	/**
	 * Writes a single 16-bit word to the file, little endian.
	 * @throws FileException if file is not open / if write failed.
	 */
	void writeUint16LE(uint16 value);
	/**
	 * Writes a single 32-bit word to the file, big endian.
	 * @throws FileException if file is not open / if write failed.
	 */
	void writeUint32BE(uint32 value);
	/**
	 * Writes a single 32-bit word to the file, little endian.
	 * @throws FileException if file is not open / if write failed.
	 */
	void writeUint32LE(uint32 value);

	/**
	 * Works the same way as fwrite, but throws on error or if
	 * it could not write all data.
	 *
	 * @param dataPtr	pointer to the data to be written
	 * @param dataSize	number of bytes to be written
	 * @return the number of bytes which were actually written.
	 */
	size_t write(const void *dataPtr, size_t dataSize);

	/**
	 * Seek to the specified position in the stream.
	 *
	 * @param offset how many bytes to jump
	 * @param origin SEEK_SET, SEEK_CUR or SEEK_END
	 */
	void seek(long offset, int origin);

	/**
	 * Resets the file pointer to the start of the file, in essence the same as re-opening it.
	 */
	void rewind();

	/**
	 * Returns current position of the file cursor.
	 */
	int pos() const;

	/**
	 * Check whether an error occurred.
	 */
	int err() const;

	void clearErr();

	/**
	 * True if there is nothing more to read from this file.
	 */
	bool eos() const;

	/**
	 * Returns the length of the file, in bytes, does not move the cursor.
	 */
	uint32 size() const;


protected:
	/** The mode the file was opened in. */
	FileMode _mode;
	/** Internal reference to the file. */
	FILE *_file;

    std::string _name;
};



} // End of namespace Common


#endif
