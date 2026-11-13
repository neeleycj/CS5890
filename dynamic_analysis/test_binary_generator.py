#!/usr/bin/env python3
import struct

# Read a valid base ELF
with open("test_base", "rb") as f:
    clean_bytes = bytearray(f.read())

def create_corrupt_file(filename, modifications):
    data = bytearray(clean_bytes)
    for offset, patch in modifications:
        data[offset:offset+len(patch)] = patch
    with open(filename, "wb") as f:
        f.write(data)

# Test 1: Corrupted Magic Number
create_corrupt_file("test_bad_magic.elf", [
    (0, b"\x7fNOT")
])

# Test 2: Minimum representable header counts (zero program and section headers)
create_corrupt_file("test_minimum_counts.elf", [
    (56, struct.pack("<H", 0)),  # e_phnum = 0
    (60, struct.pack("<H", 0)),  # e_shnum = 0
    (62, struct.pack("<H", 0)),  # e_shstrndx = 0
])

# Test 3: Maximum representable header counts
create_corrupt_file("test_maximum_counts.elf", [
    (56, struct.pack("<H", 0xFFFF)),  # e_phnum = UINT16_MAX
    (60, struct.pack("<H", 0xFFFF)),  # e_shnum = UINT16_MAX
])

# Test 4: Empty input file
with open("test_empty.elf", "wb") as f:
    f.write(b"")

# Test 5: Out-of-range program and section header offsets
create_corrupt_file("test_offsets_out_of_range.elf", [
    (32, struct.pack("<Q", 0xFFFFFFFFFFFFFFFF)),  # e_phoff
    (40, struct.pack("<Q", 0xFFFFFFFFFFFFFFFF)),  # e_shoff
])