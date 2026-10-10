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
    unsigned char e_ident[16];
    char machineByte;

    std::ifstream fileStream(filePath, std::ios::binary);
    if (!fileStream.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    fileStream.seekg(0);
    fileStream.read(reinterpret_cast<char*>(e_ident), 16);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read ELF identification");
    }
    
    fileStream.seekg(18);
    fileStream.read(&machineByte, 1);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read ELF machine type");
    }


    // Check the magic number, class, data encoding, and machine type
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read ELF header");
    }

    if (!(e_ident[0] == 0x7f && e_ident[1] == 'E' && e_ident[2] == 'L' && e_ident[3] == 'F'))
    {
        return false;
    }
    if (e_ident[4] != 2) 
    {
        return false;
    }
    if (e_ident[5] != 1)
    {
        return false;
    }
    if (machineByte != 62)
    {
        return false;
    }
    
    return true;
}

Elf64_Ehdr Reader::getElf64_Ehdr()
{
    Elf64_Ehdr ehdr{};

    fileStream.clear();
    fileStream.seekg(0);

    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to ELF header");
    }

    fileStream.read(reinterpret_cast<char*>(&ehdr), sizeof(ehdr));

    if (!fileStream)
    {
        throw std::runtime_error("Failed to read ELF header");
    }

    return ehdr;
}

programHeader Reader::getProgramHeader()
{
    programHeader ph;
    Elf64_Ehdr ehdr = getElf64_Ehdr();

    fileStream.clear();
    fileStream.seekg(ehdr.e_phoff);

    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to program header");
    }

    fileStream.read(reinterpret_cast<char*>(&ph), sizeof(ph));

    if (!fileStream)
    {
        throw std::runtime_error("Failed to read program header");
    }

    return ph;
}

sectionHeader Reader::getSectionHeader()
{
    sectionHeader sh;
    Elf64_Ehdr ehdr = getElf64_Ehdr();

    fileStream.clear();
    fileStream.seekg(ehdr.e_shoff);

    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to section header");
    }

    fileStream.read(reinterpret_cast<char*>(&sh), sizeof(sh));

    if (!fileStream)
    {
        throw std::runtime_error("Failed to read section header");
    }

    return sh;
}

programHeader Reader::getProgramHeaderAt(uint16_t index)
{
    Elf64_Ehdr ehdr = getElf64_Ehdr();
    programHeader ph;

    uint64_t entryOffset = ehdr.e_phoff + static_cast<uint64_t>(index) * ehdr.e_phentsize;

    fileStream.clear();
    fileStream.seekg(entryOffset);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to program header " + std::to_string(index));
    }

    fileStream.read(reinterpret_cast<char*>(&ph), sizeof(ph));
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read program header " + std::to_string(index));
    }

    return ph;
}

sectionHeader Reader::getSectionHeaderAt(uint16_t index)
{
    Elf64_Ehdr ehdr = getElf64_Ehdr();
    sectionHeader sh;

    uint64_t entryOffset = ehdr.e_shoff + static_cast<uint64_t>(index) * ehdr.e_shentsize;

    fileStream.clear();
    fileStream.seekg(entryOffset);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to section header " + std::to_string(index));
    }

    fileStream.read(reinterpret_cast<char*>(&sh), sizeof(sh));
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read section header " + std::to_string(index));
    }

    return sh;
}

std::string Reader::readCString(uint64_t offset)
{
    fileStream.clear();
    fileStream.seekg(offset);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to string at offset " + std::to_string(offset));
    }

    std::string result;
    char c;
    while (fileStream.get(c) && c != '\0')
    {
        result.push_back(c);
    }
    return result;
}

std::string Reader::resolveSectionName(uint32_t nameIndex, uint16_t shstrndx)
{
    sectionHeader strtab = getSectionHeaderAt(shstrndx);
    return readCString(strtab.sh_offset + nameIndex);
}

std::vector<programHeader> Reader::getProgramHeaders()
{
    Elf64_Ehdr ehdr = getElf64_Ehdr();
    std::vector<programHeader> headers;
    headers.reserve(ehdr.e_phnum);

    for (uint16_t i = 0; i < ehdr.e_phnum; ++i)
    {
        headers.push_back(getProgramHeaderAt(i));
    }

    return headers;
}

std::vector<sectionHeader> Reader::getSectionHeaders()
{
    Elf64_Ehdr ehdr = getElf64_Ehdr();
    std::vector<sectionHeader> headers;
    headers.reserve(ehdr.e_shnum);

    for (uint16_t i = 0; i < ehdr.e_shnum; ++i)
    {
        headers.push_back(getSectionHeaderAt(i));
    }

    return headers;
}


