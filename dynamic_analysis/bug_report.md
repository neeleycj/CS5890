# Bug Report

## Normal Inputs

### test_base

#### Test Design

- this test was a simple base-case test testing normal inputs. 
- This is simply a copy of /bin/ls, an ELF file we've been using throughout the development process. 
- The targeted program behaviors were simply if the program would complete properly if given a normal, complete ELF file.

#### Analysis Results

- The program ran as expected, it was a clean run and exit with nothing to report from ASan.

#### Root Causes and Fixes


- As there were no issues with the run of this test, there was nothing to fix.

#### Limitations

- The limitation is that this test would inherently not cover as much code as it is only covering the expected, "best-case" scenario when running the program. It would be difficult to run into any issues when doing so.

## Boundary Inputs

### test_bad_magic

#### Test Design

- This test is a boundary input test.
- The input is a augmented ELF file (test_base) where the "magic number" is not valid
- This is testing to see if the program will properly disregard any files that are not ELF files.

#### Analysis Results

- The program properly ended with an "unsupported" error, and there were no reported ASan issues.

#### Root Causes and Fixes

- As there were no issues found and the program exited as expected, there is nothing to fix.

#### Limitations

- This does check if any of the other ELF checks we defined work or don't have memory issues, only the first check for the "magic number."

### test_empty

#### Test Design

- This is a boundary test
- The input is an augmented ELF file (test_base) that was emptied
- This is meant to target if the program is able to handle an empty file

#### Analysis Results

- The program properly exited with a "Failed to read ELF identification" error, and there were no reported ASan issues.

#### Root Causes and Fixes

- As there were no reported issues and the program behaved as expected, there was nothing to fix.

#### Limitations

- An important limitation is that this would check if the program functions properly, but wouldn't check memory issues as it would exit before any memory issues could happen.

### test_maximum_counts

#### Test Design

- This is a boundary test
- The input is an augmented ELF file (test_base) with the e_phnum and e_shnum values set to their maximum 
- This is meant to target if the program is able to handle large values for e_phnum and e_shnum

#### Analysis Results

- The program properly exited with a "Failed to read program header 2540" error, and there were no reported ASan issues.

#### Root Causes and Fixes

- As there were no reported issues and the program behaved as expected, there was nothing to fix.

#### Limitations

- A "limitation" to consider for this and several other tests is the error handling that was written during development catching these memory issues before they could happen. This also only tests the maximums for some values, but not many others.

### test_minimum_counts

#### Test Design

- This is a boundary test
- The input is an augmented ELF file (test_base) with the e_phnum, e_shnum and e_shstrndx values set to their minimum (0)
- This is meant to target if the program is able to handle minimum values for e_phnum, e_shnum and e_shstrndx

#### Analysis Results

- The program properly exited without errors, just not displaying any header information as there was nothing to seek to, and there were no reported ASan issues.

#### Root Causes and Fixes

- As there were no reported issues and the program behaved as expected, there was nothing to fix.

#### Limitations

- One limitation is that this only tests the minimums for some values, but not many others.

### test_offsets_out_of_range

#### Test Design

- This is a boundary test
- The input is an augmented ELF file (test_base) that has e_phoff and e_shoff offset values set far beyond the range of the file
- This is meant to test if out-of-bound reads are possible

#### Analysis Results

- The program properly exited with a "Program header 0 offset is out of range" error and closed cleanly with no reported ASan issues.

#### Root Causes and Fixes

- As there were no reported issues and everything worked as intended, there was nothing to fix.

#### Limitations

- A "limitation" to consider for this and several other tests is the error handling that was written during development catching these memory issues before they could happen.

## Malformed Inputs

#### Test Design
Describe the test categories, representative inputs, and targeted program behaviors.

#### Analysis Results
Summarize ASan findings and confirmation results, including relevant diagnostic evidence.

#### Root Causes and Fixes
Explain each confirmed defect, its root cause, the correction, and the verification result.

#### Limitations
Identify important behaviors or execution paths not exercised.

## Failure Inputs

#### Test Design
Describe the test categories, representative inputs, and targeted program behaviors.

#### Analysis Results
Summarize ASan findings and confirmation results, including relevant diagnostic evidence.

#### Root Causes and Fixes
Explain each confirmed defect, its root cause, the correction, and the verification result.

#### Limitations
Identify important behaviors or execution paths not exercised.
