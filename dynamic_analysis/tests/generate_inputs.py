#!/usr/bin/env python3
"""Regenerate the ELF test inputs used by dyn_analysis.sh.

Every derived input is a byte-level patch or truncation of normal/test_base
(a copy of /bin/ls: ELF64, little-endian, x86-64, PIE). The generated files are
committed, so dyn_analysis.sh does not need to run this script; it exists so
the inputs can be reviewed and reproduced.

Usage: python3 dynamic_analysis/tests/generate_inputs.py
"""
import shutil
import struct
import subprocess
from pathlib import Path

TESTS = Path(__file__).resolve().parent
BASE = (TESTS / "normal" / "test_base").read_bytes()

# ELF64 header field offsets
E_PHOFF, E_SHOFF = 32, 40
E_PHENTSIZE, E_PHNUM = 54, 56
E_SHENTSIZE, E_SHNUM, E_SHSTRNDX = 58, 60, 62
SH_NAME, SH_OFFSET, SH_SIZE = 0, 24, 32
SHDR_SIZE, PHDR_SIZE = 64, 56

shoff = struct.unpack_from("<Q", BASE, E_SHOFF)[0]
shnum = struct.unpack_from("<H", BASE, E_SHNUM)[0]
shstrndx = struct.unpack_from("<H", BASE, E_SHSTRNDX)[0]
strtab_hdr = shoff + shstrndx * SHDR_SIZE
strtab_off, strtab_size = struct.unpack_from("<QQ", BASE, strtab_hdr + SH_OFFSET)
sec1_hdr = shoff + 1 * SHDR_SIZE


def u16(v): return struct.pack("<H", v)
def u32(v): return struct.pack("<I", v)
def u64(v): return struct.pack("<Q", v)


def patched(mods, data=BASE):
    out = bytearray(data)
    for offset, patch in mods:
        out[offset:offset + len(patch)] = patch
    return bytes(out)


def write(rel, data):
    path = TESTS / rel
    path.write_bytes(data)
    print(f"  {rel} ({len(data)} bytes)")


print("normal/")
src = TESTS / "normal" / "hello.c"
compiler = shutil.which("clang") or shutil.which("cc")
if compiler:
    # Relocatable object (ET_REL): has sections but no program headers.
    subprocess.run([compiler, "-c", "-O0", "-o", str(TESTS / "normal" / "test_relocatable.o"), str(src)],
                   check=True)
    print("  normal/test_relocatable.o (compiled from hello.c)")
else:
    print("  (no C compiler found; keeping existing normal/test_relocatable.o)")

print("boundary/")
write("boundary/test_empty.elf", b"")
# Header only, exactly 64 bytes, no tables: smallest file that produces a report.
write("boundary/test_header_only_64.elf",
      patched([(E_PHNUM, u16(0)), (E_SHNUM, u16(0)), (E_SHSTRNDX, u16(0))], BASE[:64]))
# One byte short of a complete ELF header.
write("boundary/test_header_63.elf", BASE[:63])
write("boundary/test_minimum_counts.elf",
      patched([(E_PHNUM, u16(0)), (E_SHNUM, u16(0)), (E_SHSTRNDX, u16(0))]))
write("boundary/test_maximum_counts.elf",
      patched([(E_PHNUM, u16(0xFFFF)), (E_SHNUM, u16(0xFFFF))]))
write("boundary/test_offsets_out_of_range.elf",
      patched([(E_PHOFF, u64(0xFFFFFFFFFFFFFFFF)), (E_SHOFF, u64(0xFFFFFFFFFFFFFFFF))]))
# e_shstrndx == e_shnum: first index past the valid range.
write("boundary/test_shstrndx_one_past.elf", patched([(E_SHSTRNDX, u16(shnum))]))
# e_shstrndx == SHN_UNDEF (0): valid ELF meaning "no section name table".
write("boundary/test_shstrndx_undef.elf", patched([(E_SHSTRNDX, u16(0))]))
# sh_name == sh_size - 1: the last byte of the string table (its final NUL).
write("boundary/test_name_last_byte.elf", patched([(sec1_hdr + SH_NAME, u32(strtab_size - 1))]))

print("malformed/")
write("malformed/test_bad_magic.elf", patched([(0, b"\x7fNOT")]))
write("malformed/test_class32.elf", patched([(4, b"\x01")]))       # EI_CLASS = ELFCLASS32
write("malformed/test_big_endian.elf", patched([(5, b"\x02")]))    # EI_DATA = ELFDATA2MSB
# Low byte of e_machine is 62 (x86-64) but the full 16-bit value is 0x013E.
write("malformed/test_machine_0x013e.elf", patched([(18, u16(0x013E))]))
write("malformed/test_trunc_ident.elf", BASE[:17])
write("malformed/test_trunc_phdr_table.elf", BASE[:64 + PHDR_SIZE * 3 + 10])
write("malformed/test_trunc_shdr_table.elf", BASE[:shoff + SHDR_SIZE * 5 + 7])
write("malformed/test_phentsize_zero.elf", patched([(E_PHENTSIZE, u16(0))]))
write("malformed/test_shentsize_mismatch.elf", patched([(E_SHENTSIZE, u16(40))]))
# Section 1's name starts at the last byte of .shstrtab, and that byte (the
# table's terminating NUL) is overwritten, so the name never terminates in-table.
write("malformed/test_name_unterminated.elf",
      patched([(sec1_hdr + SH_NAME, u32(strtab_size - 1)), (strtab_off + strtab_size - 1, b"X")]))
write("malformed/test_strtab_past_eof.elf", patched([(strtab_hdr + SH_OFFSET, u64(0x7FFFFFFF0000))]))
write("malformed/test_strtab_size_huge.elf", patched([(strtab_hdr + SH_SIZE, u64(0xFFFFFFFFFFFFFFFF))]))
