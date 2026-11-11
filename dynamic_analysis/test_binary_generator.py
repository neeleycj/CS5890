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

# Test 2: Impossible Section Header Offset (e_shoff = 0xFFFFFFFFFFFFFFFF)
create_corrupt_file("test_shoff_overflow.elf", [
    (40, struct.pack("<Q", 0xFFFFFFFFFFFFFFFF))
])

# Test 3: Truncated File (e_phnum says 10 headers, but file cuts off at 64 bytes)
create_corrupt_file("test_truncated.elf", [
    (56, struct.pack("<H", 10))  # e_phnum = 10
])
# Crop file to only 64 bytes
with open("test_truncated.elf", "wb") as f:
    f.write(clean_bytes[:64])