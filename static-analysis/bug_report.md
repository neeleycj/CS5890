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
/home/genial_libra/Code/CS5890/src/reader.cpp:27:19: warning: declaration shadows a field of 'Reader' [-Wshadow]
   27 |     std::ifstream fileStream(filePath, std::ios::binary);
      |                   ^
/home/genial_libra/Code/CS5890/include/reader.hpp:10:19: note: previous declaration is here
   10 |     std::ifstream fileStream;
      |                   ^
```
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
```
/home/genial_libra/Code/CS5890/src/reader.cpp:150:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  150 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```
```
/home/genial_libra/Code/CS5890/src/reader.cpp:173:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  173 |     fileStream.seekg(entryOffset);
      |                      ^~~~~~~~~~~
```
```
/home/genial_libra/Code/CS5890/src/reader.cpp:191:22: warning: implicit conversion changes signedness: 'uint64_t' (aka 'unsigned long') to 'streamoff' (aka 'long') [-Wsign-conversion]
  191 |     fileStream.seekg(offset);
      |                      ^~~~~~
6 warnings generated.
```

# Bug Report

## Warning 1

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


## Warning 2

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

## Warning 3

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

## Warning 4

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