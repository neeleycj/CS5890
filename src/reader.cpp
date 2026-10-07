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

Elf64_Ehdr Reader::getElf64_Ehdr()
{
    Elf64_Ehdr ehdr;
    unsigned char e_ident[16];
    fileStream.seekg(0);
    fileStream.read(reinterpret_cast<char*>(&e_ident), sizeof(e_ident));

    ehdr.e_ident[0] = e_ident[0];
    ehdr.e_ident[1] = e_ident[1];
    ehdr.e_ident[2] = e_ident[2];
    ehdr.e_ident[3] = e_ident[3];
    ehdr.e_ident[4] = e_ident[4];
    ehdr.e_ident[5] = e_ident[5];
    ehdr.e_ident[6] = e_ident[6];
    ehdr.e_ident[7] = e_ident[7];
    ehdr.e_ident[8] = e_ident[8];
    ehdr.e_ident[9] = e_ident[9];
    ehdr.e_ident[10] = e_ident[10];
    ehdr.e_ident[11] = e_ident[11];
    ehdr.e_ident[12] = e_ident[12];
    ehdr.e_ident[13] = e_ident[13];
    ehdr.e_ident[14] = e_ident[14];
    ehdr.e_ident[15] = e_ident[15];

    return ehdr;
}

programHeader Reader::getProgramHeader()
{
    programHeader ph;
    uint32_t phOffset;
    fileStream.seekg(28);
    fileStream.read(reinterpret_cast<char*>(&phOffset), sizeof(phOffset));
    ph.p_offset = phOffset;

    fileStream.seekg(ph.p_offset);
    fileStream.read(reinterpret_cast<char*>(&ph.p_type), sizeof(ph.p_type));
    fileStream.read(reinterpret_cast<char*>(&ph.p_flags), sizeof(ph.p_flags));
    fileStream.read(reinterpret_cast<char*>(&ph.p_offset), sizeof(ph.p_offset));
    fileStream.read(reinterpret_cast<char*>(&ph.p_vaddr), sizeof(ph.p_vaddr));
    fileStream.read(reinterpret_cast<char*>(&ph.p_paddr), sizeof(ph.p_paddr));
    fileStream.read(reinterpret_cast<char*>(&ph.p_filesz), sizeof(ph.p_filesz));
    fileStream.read(reinterpret_cast<char*>(&ph.p_memsz), sizeof(ph.p_memsz));
    fileStream.read(reinterpret_cast<char*>(&ph.p_align), sizeof(ph.p_align));

    return ph;


}

sectionHeader Reader::getSectionHeader()
{
    sectionHeader sh;
    uint32_t shOffset;
    fileStream.seekg(40);
    fileStream.read(reinterpret_cast<char*>(&shOffset), sizeof(shOffset));
    sh.sh_offset = shOffset;

    fileStream.seekg(sh.sh_offset);
    fileStream.read(reinterpret_cast<char*>(&sh.sh_name), sizeof(sh.sh_name));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_type), sizeof(sh.sh_type));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_flags), sizeof(sh.sh_flags));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_addr), sizeof(sh.sh_addr));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_offset), sizeof(sh.sh_offset));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_size), sizeof(sh.sh_size));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_link), sizeof(sh.sh_link));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_info), sizeof(sh.sh_info));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_addralign), sizeof(sh.sh_addralign));
    fileStream.read(reinterpret_cast<char*>(&sh.sh_entsize), sizeof(sh.sh_entsize));

    return sh;



}


