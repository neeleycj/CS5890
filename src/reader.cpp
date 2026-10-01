#include "reader.hpp"

Reader::Reader(const std::string& filePath) : filePath(filePath), fileStream(filePath, std::ios::binary)
{
    if (!fileStream.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
}

Reader::~Reader()
{
    if (fileStream.is_open())
    {
        fileStream.close();
    }
}

bool Reader::isValidELFFile()
{
    char magic[4];
    char classByte;
    fileStream.read(magic, 4);
    fileStream.read(&classByte, 1);
    if (!(magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F'))
    {
        return false;
    }
    if (classByte != 2) 
    {
        return false;
    }
    return true;
}