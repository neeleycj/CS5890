#pragma once

#include "elf_types.hpp"
#include <cstdint>
#include <string>
#include <vector>

std::string decodeObjectType(uint16_t e_type);
std::string decodeObjectTypeDescription(uint16_t e_type);
std::string decodeMachine(uint16_t e_machine);
std::string decodeSegmentType(uint32_t p_type);
std::string decodeSectionType(uint32_t sh_type);
std::string decodeProgramFlags(uint32_t p_flags);
std::string decodeSectionFlags(uint64_t sh_flags);

class Reader;

void printElfHeader(const Elf64_Ehdr& ehdr);
void printProgramHeaders(const std::vector<programHeader>& headers);
void printSectionHeaders(Reader& reader,
                          const std::vector<sectionHeader>& sections,
                          uint16_t shstrndx);
void printReport(Reader& reader);
