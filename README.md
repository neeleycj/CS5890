# ELF Binary Reader

**Team Member Names:** Chandler Neeley, Teague Beauchamp

**A-Numbers:** A02429680, A02376169

**Email Addresses:** a02429680@aggies.usu.edu, a02376169@usu.edu

**Team Leader:** Chandler Neeley

**Project:** ELF Binary Inspector 

## To build:
```
./build.sh
```

## To run:
```
./build/main
```

## Basic Usage
Once the tool is running, you will be prompted to enter a file path. If the file exists, it will be validated and the header information will be reported.

## Supported Scope
This tool supports ELF64, little-endian, x86-64 ELF files.

## Known Limitations
There is only one type of file supported, no other types are allowed to to be used by the tool and will be marked as invalid.

## Design Summary

### Major processing stages
The general flow for this tool is receiving the ELF file, reading the bytes, validating the format of the file, interpreting the structures of the file, and reporting on the contents.

### Important design decisions

One important decision that had to be made was what how we would validate the file. We decided that we would use critical information found in the ELF Header. This included validating the contents and length of the "e_ident" array (especially the prescence of the "magic number"), and checking to see if it was an allowed format. 

Another decision that had to be made was how to acutally parse the information so we could eventually print it. We decided to define some structs that could contain the parsed information, and then created a Reader class that would help us to parse and validate the file. 

We also needed to decide how to handle errors. For reading errors, we decided it would be easiest to try and read and entire header and then report on a failure, rather than trying to report after reading each line (too verbose). 