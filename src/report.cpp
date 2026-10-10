#include "report.hpp"
#include "reader.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace {

const std::unordered_map<uint16_t, std::string> kObjectTypes = {
    {0, "NONE"}, {1, "REL"}, {2, "EXEC"}, {3, "DYN"}, {4, "CORE"},
};

const std::unordered_map<uint16_t, std::string> kObjectTypeDescriptions = {
    {0, "No file type"},       {1, "Relocatable file"},
    {2, "Executable file"},    {3, "Shared object file"},
    {4, "Core file"},
};

// Non-exhaustive; extend as needed. Anything not listed falls back to "Unknown".
const std::unordered_map<uint16_t, std::string> kMachines = {
    {3, "Intel 80386"}, {8, "MIPS"}, {62, "x86-64"}, {183, "AArch64"},
};

const std::unordered_map<uint32_t, std::string> kSegmentTypes = {
    {0, "NULL"},     {1, "LOAD"},    {2, "DYNAMIC"},      {3, "INTERP"},
    {4, "NOTE"},     {5, "SHLIB"},   {6, "PHDR"},         {7, "TLS"},
    {0x6474e550, "GNU_EH_FRAME"},
    {0x6474e551, "GNU_STACK"},
    {0x6474e552, "GNU_RELRO"},
};

const std::unordered_map<uint32_t, std::string> kSectionTypes = {
    {0, "NULL"},    {1, "PROGBITS"}, {2, "SYMTAB"}, {3, "STRTAB"},
    {4, "RELA"},    {5, "HASH"},     {6, "DYNAMIC"}, {7, "NOTE"},
    {8, "NOBITS"},  {9, "REL"},      {10, "SHLIB"},  {11, "DYNSYM"},
};

template <typename Key>
std::string lookup(const std::unordered_map<Key, std::string>& table, Key key)
{
    auto it = table.find(key);
    return it == table.end() ? "Unknown" : it->second;
}

std::string hex(uint64_t value)
{
    std::ostringstream oss;
    oss << "0x" << std::hex << value;
    return oss.str();
}

// Left-pads to width like std::setw(width) << std::left, but always keeps at
// least one separating space even when the field itself is >= width wide, so
// long values (e.g. section names) never run into the next column.
std::string padLeft(const std::string& value, std::size_t width)
{
    if (value.size() >= width)
    {
        return value + " ";
    }
    return value + std::string(width - value.size(), ' ');
}

} // namespace

std::string decodeObjectType(uint16_t e_type) { return lookup(kObjectTypes, e_type); }

std::string decodeObjectTypeDescription(uint16_t e_type)
{
    auto it = kObjectTypeDescriptions.find(e_type);
    return it == kObjectTypeDescriptions.end() ? "" : it->second;
}

std::string decodeMachine(uint16_t e_machine) { return lookup(kMachines, e_machine); }
std::string decodeSegmentType(uint32_t p_type) { return lookup(kSegmentTypes, p_type); }
std::string decodeSectionType(uint32_t sh_type) { return lookup(kSectionTypes, sh_type); }

std::string decodeProgramFlags(uint32_t p_flags)
{
    std::string flags;
    flags += (p_flags & 0x4) ? 'R' : '-'; // PF_R
    flags += (p_flags & 0x2) ? 'W' : '-'; // PF_W
    flags += (p_flags & 0x1) ? 'E' : '-'; // PF_X
    return flags;
}

std::string decodeSectionFlags(uint64_t sh_flags)
{
    std::string flags;
    if (sh_flags & 0x1) flags += 'W'; // SHF_WRITE
    if (sh_flags & 0x2) flags += 'A'; // SHF_ALLOC
    if (sh_flags & 0x4) flags += 'X'; // SHF_EXECINSTR
    return flags;
}

void printElfHeader(const Elf64_Ehdr& ehdr)
{
    std::string description = decodeObjectTypeDescription(ehdr.e_type);

    std::cout << "Type:                   " << decodeObjectType(ehdr.e_type);
    if (!description.empty())
    {
        std::cout << " (" << description << ")";
    }
    std::cout << "\n";

    std::cout << "Machine:                " << decodeMachine(ehdr.e_machine) << "\n";
    std::cout << "Entry point address:    " << hex(ehdr.e_entry) << "\n";
    std::cout << "Program headers:        offset " << hex(ehdr.e_phoff)
               << ", " << ehdr.e_phnum << " entries\n";
    std::cout << "Section headers:        offset " << hex(ehdr.e_shoff)
               << ", " << ehdr.e_shnum << " entries\n";
}

void printProgramHeaders(const std::vector<programHeader>& headers)
{
    std::cout << "Program Headers:\n";
    std::cout << "  " << padLeft("Type", 15)
               << padLeft("Flags", 7)
               << padLeft("Offset", 12)
               << padLeft("VirtAddr", 21)
               << std::right
               << std::setw(10) << "FileSize"
               << std::setw(10) << "MemSize" << "\n";

    if (headers.empty())
    {
        std::cout << "  (none)\n";
        return;
    }

    for (const auto& ph : headers)
    {
        std::cout << "  " << padLeft(decodeSegmentType(ph.p_type), 15)
                   << padLeft(decodeProgramFlags(ph.p_flags), 7)
                   << padLeft(hex(ph.p_offset), 12)
                   << padLeft(hex(ph.p_vaddr), 21)
                   << std::right
                   << std::setw(10) << ph.p_filesz
                   << std::setw(10) << ph.p_memsz << "\n";
    }
}

void printSectionHeaders(Reader& reader,
                          const std::vector<sectionHeader>& sections,
                          uint16_t shstrndx)
{
    std::cout << "Section Headers:\n";
    std::cout << "  " << padLeft("Name", 18)
               << padLeft("Type", 11)
               << padLeft("Flags", 7)
               << padLeft("Offset", 12)
               << std::right
               << std::setw(8)  << "Size" << "\n";

    if (sections.empty())
    {
        std::cout << "  (none)\n";
        return;
    }

    for (const auto& sh : sections)
    {
        std::string name = reader.resolveSectionName(sh.sh_name, shstrndx);
        std::cout << "  " << padLeft(name, 18)
                   << padLeft(decodeSectionType(sh.sh_type), 11)
                   << padLeft(decodeSectionFlags(sh.sh_flags), 7)
                   << padLeft(hex(sh.sh_offset), 12)
                   << std::right
                   << std::setw(8)  << sh.sh_size << "\n";
    }
}

void printReport(Reader& reader)
{
    if (!reader.isValidELFFile())
    {
        std::cout << "unsupported\n";
        return;
    }

    Elf64_Ehdr ehdr = reader.getElf64_Ehdr();
    printElfHeader(ehdr);

    // TODO: these two calls assume enumeration (owned elsewhere) exposes
    // vector-returning getters, e.g. getProgramHeaders() / getSectionHeaders().
    // Until that lands, this won't compile against the current Reader, which
    // only has single-entry getProgramHeader()/getSectionHeader().
    std::vector<programHeader> programHeaders = reader.getProgramHeaders();
    std::vector<sectionHeader> sectionHeaders = reader.getSectionHeaders();

    std::cout << "\n";
    printProgramHeaders(programHeaders);

    std::cout << "\n";
    printSectionHeaders(reader, sectionHeaders, ehdr.e_shstrndx);
}
