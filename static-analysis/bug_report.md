# Bug Report

## Warning 1

### Warning	
- This is a declaration shadowing another

```
(reader.cpp)
warning: declaration shadows a field of 'Reader' [-Wshadow]
   27 |     std::ifstream fileStream(filePath, std::ios::binary);
      |                   ^
(reader.hpp)
note: previous declaration is here
   10 |     std::ifstream fileStream;
      |                   ^
```

### Analysis	
- This is a declaration of ```std::ifstream fileStream(filePath, std::ios::binary)```in our reader.cpp file shadowing a previous declaration of ```std::ifstream fileStream``` in our reader.hpp file.

- Relevant execution path:
  - main
  - Reader reader
    - Initializes Reader::filestream
  - printReport
    - Reader::isValidELFFile
      - uses Reader::filestream AND creates a new one

### Confirmation	
- This is in fact a confirmed bug. Essentially, the constructor opens the file, then ```isValidELFFile()``` opens it again. In most cases, this won't cause much of an issue, but it could be a problem if the file path were to change during execution, for example.

### Root cause	
- This is a design and implementation issue where there is confusion about what was needed in the constructor and misusing it in ```isValidELFFile()```.

### Fix	
- We simply removed the shadow declaration on line 27 in reader.cpp. After running the script again, the warning is gone.


## Warning 2

### Warning	
- This is a implicit conversion issue between unsigned to a signed long
```
reader.cpp:102:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  102 |     fileStream.seekg(ehdr.e_phoff);
      |                      ~~~~~^~~~~~~
```

### Analysis	
- The issue here is that the e_phoff is defined as a uint64_t to match the expected struct for an ELF file, but the stream expects a signed long. Generally this won't cause many issues, but if there is a malformed file then it could lead to a runtime error.

- Relevant execution path:
  - Reader::getProgramHeader
    - Reader::getElf64_Ehdr
      - reader the ELF header
    - attempts to seek to e_phoff

### Confirmation	
- Confirmed bug. This could lead to a runtime error which would be a major issue for the program.

### Root cause	

- Generally, a bug wouldn't likely occur with something like this. But, if there were to be a malformed or malicious header that was too large, then it could lead to seeking issues. This is because seekg expects a streamoff object that is signed, so you have a conversion that may not work or lead to unexpected behaviors.

### Fix	

- The fix was simple. All we needed to do was have a check before casting to see if the header is too large and throw an error, then cast to a streamoff object for safe seeking. We then ran the script and got no more warnings.

``` 
lines 102-106
if (ehdr.e_phoff > std::numeric_limits<std::streamoff>::max())
    {
        throw std::runtime_error("Program header offset is too large");
    }
    fileStream.seekg(static_cast<std::streamoff>(ehdr.e_phoff));
```

## Warning 3

### Warning	
- This is another implicit conversion issue between unsigned to a signed long
```
/home/genial_libra/Code/CS5890/src/reader.cpp:125:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  125 |     fileStream.seekg(ehdr.e_shoff);
      |                      ~~~~~^~~~~~~
```

### Analysis	

- The issue here again is that the e_shoff is defined as a uint64_t to match the expected struct for an ELF file, but the stream expects a signed long. Generally this won't cause many issues, but if there is a malformed file then it could lead to a runtime error.
- Relevant execution path:
  - Reader::getProgramHeader
    - Reader::getElf64_Ehdr
      - reader the ELF header
    - attempts to seek to e_phoff

### Confirmation	

- Confirmed bug. This could lead to a runtime error which would be a major issue for the program.

### Root cause	
- Generally, a bug wouldn't likely occur with something like this. But, if there were to be a malformed or malicious header that was too large, then it could lead to seeking issues. This is because seekg expects a streamoff object that is signed, so you have a conversion that may not work or lead to unexpected behaviors. 

### Fix	

- The fix was simple. All we needed to do was have a check before casting to see if the header is too large and throw an error, then cast to a streamoff object for safe seeking. We then ran the script again and got no warnings.

``` 
lines 129-133
if (ehdr.e_shoff > std::numeric_limits<std::streamoff>::max())
    {
        throw std::runtime_error("Section header offset is too large");
    }
    fileStream.seekg(static_cast<std::streamoff>(ehdr.e_shoff));
```

## Warning 4

### Warning	

- This is a `Wsign-conversion`: an implicit conversion that changes the 'signedness' of `entryOffset`. Found in the `reader.cpp` file.

```
/home/genial_libra/Code/CS5890/src/reader.cpp:150:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  150 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```

### Analysis	

- `entryOffset` is a `uint64_t` (unsigned integer, from 0 to 2^{64} - 1). `seekg()` takes a `std::streamoff` (a signed long, from -2^{63} to 2^{63} - 1). The compiler converts one to the other silently, so any value of `entryOffset` that is greater than `INT64_MAX` will wrap around to become a negative number when being converted from `uint64_t` to a `long`.

- Relevant execution path:
  ```
  main.cpp:13           Reader reader(filepath)
  report.cpp:190        reader.getProgramHeaders()
  reader.cpp:217        for (i = 0; i < ehdr.e_phnum; ++i)
  reader.cpp:219        getProgramHeaderAt(i)
  reader.cpp:143        ehdr = getElf64_Ehdr()
  reader.cpp:146        entryOffset = e_phoff + i * e_phentsize
  reader.cpp:149        fileStream.seekg(entryOffset)
  reader.cpp:155        fileStream.read(&ph, sizeof(ph))
  ```

### Confirmation	

- Confirmed. If the signed result is negative, seekg sets failbit and we throw a misleading "Failed to seek" error. If the addition wraps to a small valid offset, seekg succeeds and read() fills ph with unrelated bytes, which the report then prints as a program header.

### Root cause	

- The code treats file-supplied ELF fields as trusted. It does arithmetic on them without checking for overflow, then passes the unsigned result to an API that expects a signed type, without checking the range.
### Fix	

- Before adding, check that `e_phoff + index * e_phentsize` fits in a `streamoff`. Since `index * e_phentsize` is at most 65535 * 65535 (< 2^32), checking `e_phoff > max - relOffset` also prevents the `uint64_t` addition from wrapping. Then cast explicitly for `seekg`.

```cpp
reader.cpp: getProgramHeaderAt()
const uint64_t relOffset = static_cast<uint64_t>(index) * ehdr.e_phentsize;
const auto maxOffset = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
if (ehdr.e_phoff > maxOffset - relOffset)
{
    throw std::runtime_error("Program header " + std::to_string(index) + " offset is out of range");
}
uint64_t entryOffset = ehdr.e_phoff + relOffset;

fileStream.clear();
fileStream.seekg(static_cast<std::streamoff>(entryOffset));
```

## Warning 5

### Warning	

- This is another `Wsign-conversion` on `entryOffset`, this time in `getSectionHeaderAt()` in the `reader.cpp` file.

```
/home/genial_libra/Code/CS5890/src/reader.cpp:173:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  173 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```

### Analysis	

- This is the same conversion as Warning 4. `entryOffset = e_shoff + index * e_shentsize` is a `uint64_t` built from ELF header fields and passed straight to `seekg()`, which takes a signed `streamoff`. Any value greater than `INT64_MAX` wraps around to a negative number.
- This function is reached by two paths. In the second one, the index is `e_shstrndx`, which is read straight from the file and never checked against `e_shnum`.

- Relevant execution path:
  ```
  main.cpp:13           Reader reader(filepath)
  report.cpp:191        reader.getSectionHeaders()
  reader.cpp:240        for (i = 0; i < ehdr.e_shnum; ++i)
  reader.cpp:242        getSectionHeaderAt(i)
  reader.cpp:175        ehdr = getElf64_Ehdr()
  reader.cpp:178        entryOffset = e_shoff + i * e_shentsize
  reader.cpp:181        fileStream.seekg(entryOffset)
  reader.cpp:187        fileStream.read(&sh, sizeof(sh))

  report.cpp:197        printSectionHeaders(reader, sectionHeaders, ehdr.e_shstrndx)
  report.cpp:165        reader.resolveSectionName(sh.sh_name, shstrndx)
  reader.cpp:216        getSectionHeaderAt(shstrndx)
  reader.cpp:181        fileStream.seekg(entryOffset)
  ```

### Confirmation	

- Confirmed. As in Warning 4, a negative result makes `seekg` set failbit, and we throw a misleading "Failed to seek" error. If the addition wraps to a small valid offset, `seekg` succeeds and `read()` fills `sh` with unrelated bytes.

### Root cause	

- This is the same root cause as Warning 4. File-supplied ELF fields are treated as trusted, added together without an overflow check, and passed to a signed API without a range check.

### Fix	

- Use the same range check and explicit cast as Warning 4 in `getSectionHeaderAt()`, and check `shstrndx` against `e_shnum` in `resolveSectionName()`.

```cpp
reader.cpp: getSectionHeaderAt()
const uint64_t relOffset = static_cast<uint64_t>(index) * ehdr.e_shentsize;
const auto maxOffset = static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max());
if (ehdr.e_shoff > maxOffset - relOffset)
{
    throw std::runtime_error("Section header " + std::to_string(index) + " offset is out of range");
}
uint64_t entryOffset = ehdr.e_shoff + relOffset;

fileStream.clear();
fileStream.seekg(static_cast<std::streamoff>(entryOffset));

reader.cpp: resolveSectionName()
Elf64_Ehdr ehdr = getElf64_Ehdr();
if (shstrndx >= ehdr.e_shnum)
{
    throw std::runtime_error("Section name string table index is out of range");
}
sectionHeader strtab = getSectionHeaderAt(shstrndx);
```

## Warning 6

### Warning	

- This is a `Wsign-conversion` on `offset` in `readCString()` in the `reader.cpp` file.

```
/home/genial_libra/Code/CS5890/src/reader.cpp:191:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  191 |     fileStream.seekg(offset);
      |                      ^~~~~~
```

### Analysis	

- `readCString()` takes a `uint64_t offset` and passes it straight to `seekg()`, which takes a signed `streamoff`, so any value greater than `INT64_MAX` becomes negative. The offset is computed by `resolveSectionName()` as `strtab.sh_offset + nameIndex`. `sh_offset` comes from the string-table section header, which could be the garbage header from Warning 5, and `nameIndex` is each section's `sh_name`. Neither value is checked, and the sum can overflow.

- Relevant execution path:
  ```
  main.cpp:13           Reader reader(filepath)
  report.cpp:197        printSectionHeaders(reader, sectionHeaders, ehdr.e_shstrndx)
  report.cpp:165        reader.resolveSectionName(sh.sh_name, shstrndx)
  reader.cpp:216        strtab = getSectionHeaderAt(shstrndx)
  reader.cpp:217        readCString(strtab.sh_offset + nameIndex)
  reader.cpp:199        fileStream.seekg(offset)
  reader.cpp:207        while (fileStream.get(c) && c != '\0')
  ```

### Confirmation	

- Confirmed. A negative offset makes `seekg` fail, and we throw a misleading "Failed to seek to string" error. A wrapped or out-of-table offset seeks to the wrong place, and the loop then reads bytes until it hits a `'\0'` or EOF. The result is that arbitrary file contents get printed as section names. Because `nameIndex` is never compared to `strtab.sh_size`, this can happen even when the addition does not overflow.

### Root cause	

- Two file-controlled values are added without an overflow check, and the unsigned result is implicitly converted to a signed `streamoff`. `nameIndex` is also never bounds-checked against the size of the string table it indexes into.

### Fix	

- Do a range check and an explicit cast in `readCString()`. In `resolveSectionName()`, reject a `nameIndex` that is outside the string table and an addition that would overflow.

```cpp
reader.cpp: readCString()
if (offset > static_cast<uint64_t>(std::numeric_limits<std::streamoff>::max()))
{
    throw std::runtime_error("String offset is too large: " + std::to_string(offset));
}
fileStream.clear();
fileStream.seekg(static_cast<std::streamoff>(offset));

reader.cpp: resolveSectionName()
if (nameIndex >= strtab.sh_size ||
    strtab.sh_offset > std::numeric_limits<uint64_t>::max() - nameIndex)
{
    throw std::runtime_error("Section name index is out of range");
}
return readCString(strtab.sh_offset + nameIndex);
```

## Verification

All 6 of these bug are now fixed. When running the `st_analysis.sh` script we get the output:

```
scan-build: Using '/usr/bin/clang-22' for static analysis
-- The CXX compiler identification is Clang 22.1.8
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/lib/clang/c++-analyzer - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (2.6s)
-- Generating done (0.0s)
-- Build files have been written to: /home/winton/fall26/cs6890/CS5890/static-analysis/build-analysis
scan-build: Analysis run complete.
scan-build: Analysis results (plist files) deposited in '/home/winton/fall26/cs6890/CS5890/static-analysis/analysis-results/2026-10-31-125417-479970-1'
scan-build: Removing directory '/home/winton/fall26/cs6890/CS5890/static-analysis/analysis-results/2026-10-31-125417-479970-1' because it contains no reports.
scan-build: No bugs found.
scan-build: Using '/usr/bin/clang-22' for static analysis
[ 25%] Building CXX object CMakeFiles/main.dir/src/main.cpp.o
[ 75%] Building CXX object CMakeFiles/main.dir/src/reader.cpp.o
[ 75%] Building CXX object CMakeFiles/main.dir/src/report.cpp.o
[100%] Linking CXX executable main
[100%] Built target main
scan-build: Analysis run complete.
scan-build: Analysis results (plist files) deposited in '/home/winton/fall26/cs6890/CS5890/static-analysis/analysis-results/2026-10-31-125420-480033-1'
scan-build: Removing directory '/home/winton/fall26/cs6890/CS5890/static-analysis/analysis-results/2026-10-31-125420-480033-1' because it contains no reports.
scan-build: No bugs found.
Static analyzer found no bugs. Review compiler warnings printed above.
```

