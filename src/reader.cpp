#include "reader.hpp"
#include <stdexcept>
#include <iostream>
#include <cstdint>
#include <limits>

Reader::Reader(const std::string& path) : filePath(path), fileStream(path, std::ios::binary)
{
    if (!fileStream.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
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
    unsigned char machineBytes[2];

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
    
    // e_machine is a 16-bit little-endian field at offset 18. Both bytes must
    // be checked, otherwise values such as 0x013E pass as x86-64 (62).
    fileStream.seekg(18);
    fileStream.read(reinterpret_cast<char*>(machineBytes), 2);
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
    const uint16_t machine = static_cast<uint16_t>(machineBytes[0] | (machineBytes[1] << 8));
    if (machine != 62)
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
    if (ehdr.e_phoff > std::numeric_limits<std::streamoff>::max())
    {
        throw std::runtime_error("Program header offset is too large");
    }
    fileStream.seekg(static_cast<std::streamoff>(ehdr.e_phoff));

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
    if (ehdr.e_shoff > std::numeric_limits<std::streamoff>::max())
    {
        throw std::runtime_error("Section header offset is too large");
    }
    fileStream.seekg(static_cast<std::streamoff>(ehdr.e_shoff));

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

    const uint64_t relOffset = static_cast<uint64_t>(index) * ehdr.e_phentsize;
    const auto maxOffset = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
    if (ehdr.e_phoff > maxOffset - relOffset)
    {
        throw std::runtime_error("Program header " + std::to_string(index) + " offset is out of range");
    }
    uint64_t entryOffset = ehdr.e_phoff + relOffset;

    fileStream.clear();
    fileStream.seekg(static_cast<std::streamoff>(entryOffset));
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

    const uint64_t relOffset = static_cast<uint64_t>(index) * ehdr.e_shentsize;
    const auto maxOffset = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
    if (ehdr.e_shoff > maxOffset - relOffset)
    {
        throw std::runtime_error("Section header " + std::to_string(index) + " offset is out of range");
    }
    uint64_t entryOffset = ehdr.e_shoff + relOffset;

    fileStream.clear();
    fileStream.seekg(static_cast<std::streamoff>(entryOffset));
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

uint64_t Reader::fileSize()
{
    fileStream.clear();
    fileStream.seekg(0, std::ios::end);
    const std::streamoff end = fileStream.tellg();
    if (!fileStream || end < 0)
    {
        throw std::runtime_error("Failed to determine file size");
    }
    return static_cast<uint64_t>(end);
}

std::string Reader::readCString(uint64_t offset, uint64_t maxLen)
{
    if (offset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()))
    {
        throw std::runtime_error("String offset is too large: " + std::to_string(offset));
    }
    fileStream.clear();
    fileStream.seekg(static_cast<std::streamoff>(offset));
    if (!fileStream)
    {
        throw std::runtime_error("Failed to seek to string at offset " + std::to_string(offset));
    }

    // Never read more than maxLen bytes: a string must terminate inside the
    // table it belongs to, not somewhere later in the file.
    std::string result;
    char c;
    for (uint64_t i = 0; i < maxLen; ++i)
    {
        if (!fileStream.get(c))
        {
            throw std::runtime_error("Unexpected end of file in string at offset " + std::to_string(offset));
        }
        if (c == '\0')
        {
            return result;
        }
        result.push_back(c);
    }
    throw std::runtime_error("Unterminated string at offset " + std::to_string(offset));
}

std::string Reader::resolveSectionName(uint32_t nameIndex, uint16_t shstrndx)
{
    // SHN_UNDEF (0) means the file has no section name string table; this is
    // valid ELF, so report unnamed sections instead of failing.
    if (shstrndx == 0)
    {
        return "";
    }

    Elf64_Ehdr ehdr = getElf64_Ehdr();
    if (shstrndx >= ehdr.e_shnum)
    {
        throw std::runtime_error("Section name string table index is out of range");
    }
    sectionHeader strtab = getSectionHeaderAt(shstrndx);
    const uint64_t size = fileSize();
    if (strtab.sh_offset > size || strtab.sh_size > size - strtab.sh_offset)
    {
        throw std::runtime_error("Section name string table extends past end of file");
    }
    if (nameIndex >= strtab.sh_size)
    {
        throw std::runtime_error("Section name index is out of range");
    }
    return readCString(strtab.sh_offset + nameIndex, strtab.sh_size - nameIndex);
}

std::vector<programHeader> Reader::getProgramHeaders()
{
    Elf64_Ehdr ehdr = getElf64_Ehdr();
    std::vector<programHeader> headers;
    // Entries are read as fixed-size structs, so any other entry size would
    // produce overlapping or misaligned records.
    if (ehdr.e_phnum > 0 && ehdr.e_phentsize != sizeof(programHeader))
    {
        throw std::runtime_error("Unsupported program header entry size: " + std::to_string(ehdr.e_phentsize) +
                                 " (expected " + std::to_string(sizeof(programHeader)) + ")");
    }
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
    if (ehdr.e_shnum > 0 && ehdr.e_shentsize != sizeof(sectionHeader))
    {
        throw std::runtime_error("Unsupported section header entry size: " + std::to_string(ehdr.e_shentsize) +
                                 " (expected " + std::to_string(sizeof(sectionHeader)) + ")");
    }
    headers.reserve(ehdr.e_shnum);

    for (uint16_t i = 0; i < ehdr.e_shnum; ++i)
    {
        headers.push_back(getSectionHeaderAt(i));
    }

    return headers;
}

