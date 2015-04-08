#pragma once

#include <fstream> 
#include <sstream>
#include <iterator>
#include "unknown_opcode_exception.h"

class BinaryReader
{
public:
    static std::vector<unsigned char> ReadAll(std::string fileName)
    {
        std::ifstream file(fileName.c_str(), std::ios::binary | std::ios::ate);
        if (file.is_open())
        {
            size_t fileSizeInBytes = size_t(file.tellg());
            std::vector<unsigned char> fileContents(fileSizeInBytes);
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(fileContents.data()), fileContents.size());
            return fileContents;
        }
        else
        {
            throw std::runtime_error("Can't open file");
        }
    }

    BinaryReader(std::vector<unsigned char>&& data)
    {
        mSize = data.size();
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(mStream));
        mStream.seekg(std::ios::beg);
    }

    size_t Size() const
    {
        return mSize;
    }

    void Seek(unsigned int pos)
    {
        if (!mStream.seekg(pos))
        {
            throw InternalDecompilerError();
        }
    }

    unsigned int Position()
    {
        return static_cast<unsigned int>(mStream.tellg());
    }

    unsigned int ReadU32()
    {
        return InternalRead<unsigned int>();
    }

    signed int ReadS32()
    {
        return InternalRead<signed int>();
    }

    signed short int ReadS16()
    {
        return InternalRead<signed short int>();
    }

    unsigned short int ReadU16()
    {
        return InternalRead<unsigned short int>();
    }

    unsigned char ReadU8()
    {
        return InternalRead<unsigned char>();
    }

    signed char ReadS8()
    {
        return InternalRead<signed char>();
    }

private:
    template<class T>
    T InternalRead()
    {
        T r = {};
        if (!mStream.read(reinterpret_cast<char*>(&r), sizeof(r)))
        {
            throw InternalDecompilerError();
        }
        return r;
    }

    std::stringstream mStream;
    size_t mSize;
};
