#include <cstdint>
#include <string>

struct programHeader
{
    uint64_t segmentType;
    uint64_t flags;
    uint64_t offset;
    uint64_t virtualAddress;
    uint64_t physicalAddress;
    uint64_t fileSize;
    uint64_t memorySize;
};
struct sectionHeader
{
    uint64_t name;
    uint64_t type;
    uint64_t flags;
    uint64_t offset;
    uint64_t size;
};
struct identification
{
    uint64_t type;
    uint64_t machine;
    uint64_t entryPoint;
    uint64_t programHeaderOffset;
    uint64_t programHeaderCount;
    uint64_t sectionHeaderOffset;
    uint64_t sectionHeaderCount;
};
