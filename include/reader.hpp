#include "elf_types.hpp"
#include <fstream>
#include <string>
#include <vector>

class Reader
{
private:
    std::string filePath;
    std::ifstream fileStream;
public:
    Reader(const std::string& filePath);


    ~Reader();

    bool isValidELFFile();
    programHeader getProgramHeader();
    sectionHeader getSectionHeader();
    Elf64_Ehdr getElf64_Ehdr();

    programHeader getProgramHeaderAt(uint16_t index);
    sectionHeader getSectionHeaderAt(uint16_t index);
    std::string readCString(uint64_t offset);
    std::string resolveSectionName(uint32_t nameIndex, uint16_t shstrndx);

    std::vector<programHeader> getProgramHeaders();
    std::vector<sectionHeader> getSectionHeaders();
};

