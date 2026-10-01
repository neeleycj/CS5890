#include "reader.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdint>

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
    std::ifstream fileStream(filePath, std::ios::binary);

    char magic[4];
    char classByte;
    char dataByte;
    char machineByte;
    fileStream.read(magic, 4);
    fileStream.read(&classByte, 1);
    fileStream.read(&dataByte, 1);
    fileStream.seekg(18);
    fileStream.read(&machineByte, 1);

    if (!(magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F'))
    {
        return false;
    }
    if (classByte != 2) 
    {
        return false;
    }
    if (dataByte != 1)
    {
        return false;
    }
    if (machineByte != 62)
    {
        return false;
    }
    
    return true;
}
Reader::programHeader Reader::getProgramHeader()
{
    programHeader ph;
    uint32_t phOffset;
    fileStream.seekg(28);
    fileStream.read(reinterpret_cast<char*>(&phOffset), sizeof(phOffset));
    ph.offset = phOffset;

    fileStream.seekg(ph.offset);
    fileStream.read(reinterpret_cast<char*>(&ph.segmentType), sizeof(ph.segmentType));
    fileStream.read(reinterpret_cast<char*>(&ph.flags), sizeof(ph.flags));
    fileStream.read(reinterpret_cast<char*>(&ph.offset), sizeof(ph.offset));
    fileStream.read(reinterpret_cast<char*>(&ph.virtualAddress), sizeof(ph.virtualAddress));
    fileStream.read(reinterpret_cast<char*>(&ph.physicalAddress), sizeof(ph.physicalAddress));
    fileStream.read(reinterpret_cast<char*>(&ph.fileSize), sizeof(ph.fileSize));
    fileStream.read(reinterpret_cast<char*>(&ph.memorySize), sizeof(ph.memorySize));

    return ph;


}

