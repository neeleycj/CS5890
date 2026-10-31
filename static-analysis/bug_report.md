## Analyze and Confirm Reported Warnings
- Investigate each warning reported in your project code.

- Examine the reported source location and execution path.

- Determine whether the warning represents a real bug.

- Classify each warning as a confirmed bug, false positive, or unresolved issue, with supporting justification.

## Fix Confirmed Bugs
### For each confirmed bug:

- Analyze its root cause and potential security or correctness impact.

- Modify the source code to fix the underlying problem.

- Verify the fix through testing and/or rerunning static analysis



## found warnings

```
/home/genial_libra/Code/CS5890/src/reader.cpp:102:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  102 |     fileStream.seekg(ehdr.e_phoff);
      |                      ~~~~~^~~~~~~
```
```
/home/genial_libra/Code/CS5890/src/reader.cpp:125:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  125 |     fileStream.seekg(ehdr.e_shoff);
      |                      ~~~~~^~~~~~~
```

# Teague's bugs to document and fix

```
/home/genial_libra/Code/CS5890/src/reader.cpp:150:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  150 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```

Warning 	        Warning type and source-code location
Analysis 	        Explanation of the reported problem and relevant execution path
Confirmation 	    Confirmed bug, false positive, or unresolved, with justification
Root cause 	        Why the bug occurs (for confirmed bugs)
Fix 	            Changes made and verification results (for confirmed bugs)


```
/home/genial_libra/Code/CS5890/src/reader.cpp:173:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  173 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```

Warning 	        Warning type and source-code location
Analysis 	        Explanation of the reported problem and relevant execution path
Confirmation 	    Confirmed bug, false positive, or unresolved, with justification
Root cause 	        Why the bug occurs (for confirmed bugs)
Fix 	            Changes made and verification results (for confirmed bugs)

```
/home/genial_libra/Code/CS5890/src/reader.cpp:191:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  191 |     fileStream.seekg(offset);
      |                      ^~~~~~
```



# Bug Report

## Warning 1

### Warning	
- This is a declaraction shadowing another

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
- Explanation of the reported problem and relevant execution path
- This is a declaration of ```std::ifstream fileStream(filePath, std::ios::binary)```in our reader.cpp file shadowing a previous declaration of ```std::ifstream fileStream``` in our reader.hpp file.

- Relevant execution path:
  - main
  - Reader reader
    - Intializes Reader::filestream
  - printReport
    - Reader::isValidELFFile
      - uses Reader::filestream AND creates a new one
### Confirmation	
- Confirmed bug, false positive, or unresolved, with justification
- This is in fact a confirmed bug. Essentially, the constructor opens the file, then ```isValidELFFile()``` opens it again. In most cases, this won't cause much of an issue, but it could be a problem if the file path were to change during execution, for example.
### Root cause	
- Why the bug occurs (for confirmed bugs)
- This is a design and implementation issue where there is confusion about what was needed in the constructor and misusing it in ```isValidELFFile()```.
### Fix	
- Changes made and verification results (for confirmed bugs)
- We simply removed the shadow declaration on line 27 in reader.cpp. After running the script again, the warning is gone.

## Warning 2

### Warning	
- This is a implicit conversion issue betweeen unsigned to a signed long
```
reader.cpp:102:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  102 |     fileStream.seekg(ehdr.e_phoff);
      |                      ~~~~~^~~~~~~
```
### Analysis	
- Explanation of the reported problem and relevant execution path

- The issue here is that the e_phoff is defined as a uint64_t to match the expected struct for an ELF file, but the stream expects a signed long. Generally this won't cause many issues, but if there is a malformed file then it could lead to a runtime error.

- Relevant execution path:
  - Reader::getProgramHeader
    - Reader::getElf64_Ehdr
      - reader the ELF header
    - attempts to seek to e_phoff
### Confirmation	
- Confirmed bug. This could lead to a runtime error which would be a major issue for the program.
### Root cause	
- Why the bug occurs (for confirmed bugs)

- Generally, a bug wouldn't likely occur with something like this. But, if there were to be a malformed or malicious header that was too large, then it could lead to seeking issues. This is because seekg expects a streamoff object that is signed, so you have a conversion that may not work or lead to unexpected behaviors. 
### Fix	
- Changes made and verification results (for confirmed bugs)
- The fix was simple. All we needed to do was have a check before casting to see if the header is too large and throw an error, then cast to a streamoff object for safe seeking

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
- Warning type and source-code location
- This is another implicit conversion issue betweeen unsigned to a signed long
```
/home/genial_libra/Code/CS5890/src/reader.cpp:125:27: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  125 |     fileStream.seekg(ehdr.e_shoff);
      |                      ~~~~~^~~~~~~
```

### Analysis	
- Explanation of the reported problem and relevant execution path
- The issue here again is that the e_shoff is defined as a uint64_t to match the expected struct for an ELF file, but the stream expects a signed long. Generally this won't cause many issues, but if there is a malformed file then it could lead to a runtime error.
- Relevant execution path:
  - Reader::getProgramHeader
    - Reader::getElf64_Ehdr
      - reader the ELF header
    - attempts to seek to e_phoff
### Confirmation	
- Confirmed bug, false positive, or unresolved, with justification
- - Confirmed bug. This could lead to a runtime error which would be a major issue for the program.
### Root cause	
- Why the bug occurs (for confirmed bugs)
- Generally, a bug wouldn't likely occur with something like this. But, if there were to be a malformed or malicious header that was too large, then it could lead to seeking issues. This is because seekg expects a streamoff object that is signed, so you have a conversion that may not work or lead to unexpected behaviors. 
### Fix	
- Changes made and verification results (for confirmed bugs)
- The fix was simple. All we needed to do was have a check before casting to see if the header is too large and throw an error, then cast to a streamoff object for safe seeking

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
- Warning type and source-code location
  This is a `Wsign-conversion`: an implicit conversion that changes the 'signedness' of `entryOffset`. Found in the `reader.cpp` file.
```
/home/genial_libra/Code/CS5890/src/reader.cpp:150:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  150 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```
### Analysis	
- Explanation of the reported problem and relevant execution path
  `entryOffset` is a `uint64_t` (unsigned integer, from 0 to 2^{64} - 1). `seekg()` takes a `std::streamoff` (a signed long, from -2^{63} to 2^{63} - 1). The compiler converts one to the other silently, so any value of `entryOffset` that is greater than `INT64_MAX` will wrap around to become a negative number when being converted from int to long.
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
- Confirmed bug, false positive, or unresolved, with justification
### Root cause	
- Why the bug occurs (for confirmed bugs)
### Fix	
- Changes made and verification results (for confirmed bugs)
  `reader.cpp:102-106`


## Warning 5

### Warning	
- Warning type and source-code location
### Analysis	
- Explanation of the reported problem and relevant execution path
### Confirmation	
- Confirmed bug, false positive, or unresolved, with justification
### Root cause	
- Why the bug occurs (for confirmed bugs)
### Fix	
- Changes made and verification results (for confirmed bugs)

## Warning 6

### Warning	
- Warning type and source-code location
### Analysis	
- Explanation of the reported problem and relevant execution path
### Confirmation	
- Confirmed bug, false positive, or unresolved, with justification
### Root cause	
- Why the bug occurs (for confirmed bugs)
### Fix	
- Changes made and verification results (for confirmed bugs)

