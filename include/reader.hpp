#include "elf_types.hpp"
#include <fstream>
#include <string>

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
    identification getIdentification();

};

