# Dynamic Analysis Bug Report — ELF Binary Inspector

## How to Reproduce

```bash
./dynamic_analysis/dyn_analysis.sh            # build with ASan, run all 31 cases
```

Requirements: `clang++` with the compiler-rt ASan runtime, `bash`, and coreutils (`timeout`, `head`). The script
builds straight from `src/` and `include/` (no CMake step), so it works from a clean checkout. It exits non-zero if any
case fails. Per-case logs are written to `dynamic_analysis/results/` (git-ignored).

To rerun the same suite against the code *before* the fixes:

```bash
git worktree add /tmp/prefix fb72937
./dynamic_analysis/dyn_analysis.sh --src /tmp/prefix
git worktree remove /tmp/prefix
```

`dynamic_analysis/tests/generate_inputs.py` regenerates every derived input from `tests/normal/test_base`. The
generated files are committed, so the script does not need Python.

---

## 1. Test Design

### ASan configuration

Following the [Clang AddressSanitizer documentation](https://clang.llvm.org/docs/AddressSanitizer.html):

| Setting | Purpose |
|---|---|
| `-fsanitize=address -fsanitize-address-use-after-scope` | Instrument heap, stack, and global accesses, including use-after-scope |
| `-O1 -g -fno-omit-frame-pointer` | Reasonable speed with readable, symbolized stack traces |
| `detect_leaks=1` | LeakSanitizer at exit (checks resource cleanup on every error path) |
| `detect_stack_use_after_return=1` | Catch dangling references to returned stack frames |
| `check_initialization_order=1`, `strict_string_checks=1` | Extra checks for static init order and C-string arguments |
| `halt_on_error=1`, `exitcode=86` | Stop at the first report and use an exit code that cannot be confused with the program's own codes |

**Sanity check.** Before the real tests run, the script builds and runs `tests/asan_canary.cpp`, which contains a
deliberate one-element heap overflow. The script aborts if ASan does not report it. This shows that "0 ASan reports"
from the real tests means the instrumentation was active, not that it was missing:

```
==350625==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x7c1437de0020 at pc 0x5586389742bb ...
READ of size 4 at 0x7c1437de0020 thread T0
    #0 0x5586389742ba in main .../dynamic_analysis/tests/asan_canary.cpp:8:13
SUMMARY: AddressSanitizer: heap-buffer-overflow .../dynamic_analysis/tests/asan_canary.cpp:8:13 in main
```

### Oracle

Every case in `tests/cases.txt` lists an expected exit code and an expected string in the output. A case **passes** only
if (a) no `ERROR: AddressSanitizer` / `ERROR: LeakSanitizer` report appears, (b) the exit code matches, and (c) the
expected text is present. The old script counted any exit code of 1 as an "ASan failure", which mixed up clean rejections
with real reports. It also could never flag anything, because the program always exited with 0.

### Test categories

Derived inputs are byte-level patches or truncations of `test_base` (a copy of `/bin/ls`: ELF64, LE, x86-64, PIE;
31 section headers, 13 program headers). Each case targets a separate branch in `Reader`/`printReport`, not just more
of the same input.

#### Normal inputs — expected functionality

| Case | Input | Behavior targeted |
|---|---|---|
| `base_pie_executable` | `normal/test_base` | Full happy path: header, 13 program headers, 31 named sections |
| `relocatable_object` | `normal/test_relocatable.o` (from `hello.c`) | `ET_REL` file with **no** program headers but real sections |
| `interactive_prompt` | path fed on stdin | The `std::cin` prompt path in `main` |

#### Boundary inputs — minimum, maximum, empty, off-by-one

| Case | Input | Behavior targeted |
|---|---|---|
| `empty_file` | 0 bytes | First read fails |
| `header_only_64_bytes` | exactly 64 bytes, counts = 0 | Smallest file that yields a full report |
| `header_63_bytes` | 63 bytes | One byte short of `sizeof(Elf64_Ehdr)` |
| `minimum_counts` | `e_phnum = e_shnum = e_shstrndx = 0` | Empty table loops, "(none)" output |
| `maximum_counts` | `e_phnum = e_shnum = 0xFFFF` | Largest `reserve()`; table loop runs into EOF |
| `offsets_out_of_range` | `e_phoff = e_shoff = 2^64-1` | Overflow guard on `offset + index*entsize` |
| `shstrndx_one_past` | `e_shstrndx = e_shnum` | Off-by-one on the string-table index check |
| `shstrndx_undef` | `e_shstrndx = 0` (`SHN_UNDEF`) | Valid "no string table" special value |
| `name_last_byte` | `sh_name = sh_size - 1` | Last valid byte of `.shstrtab` (empty name) |

#### Malformed inputs — invalid formats, truncation, inconsistent fields

| Case | Input | Behavior targeted |
|---|---|---|
| `bad_magic` | `\x7fNOT` | Magic number rejection |
| `class32` / `big_endian` | `EI_CLASS=1` / `EI_DATA=2` | Unsupported class / encoding |
| `machine_0x013e` | `e_machine = 0x013E` | Full 16-bit machine check |
| `trunc_ident` | 17 bytes | Truncated before `e_machine` |
| `trunc_phdr_table` | cut inside program header 3 | Short read mid-table |
| `trunc_shdr_table` | cut inside section header 5 | Short read mid-table |
| `phentsize_zero` | `e_phentsize = 0` | Entry size inconsistent with the struct being read |
| `shentsize_mismatch` | `e_shentsize = 40` | Same, for sections |
| `name_unterminated` | name starts at `.shstrtab`'s last byte, NUL overwritten | String read must stay inside its table |
| `strtab_past_eof` | `.shstrtab` `sh_offset` far past EOF | String table location validation |
| `strtab_size_huge` | `.shstrtab` `sh_size = 2^64-1` | String table size validation |

#### Failure scenarios — error handling, interrupted operations, cleanup

| Case | Scenario | Behavior targeted |
|---|---|---|
| `missing_file` | nonexistent path | Constructor throws, exception path cleanup |
| `directory_input` | path is a directory | `open` succeeds but `read` fails (`EISDIR`) |
| `unreadable_file` | copy with mode `000` | Permission failure (skipped if run as root) |
| `non_seekable_pipe` | `cat file \| main /dev/stdin` | `seekg` fails on a non-seekable stream |
| `interrupted_pipe` | `head -c 100 file \| main /dev/stdin` | Writer stops early (interrupted input) |
| `stdin_closed` | prompt mode, stdin = `/dev/null` | `std::cin` hits EOF before a path is given |
| `sigint_at_prompt` | `SIGINT` while blocked at the prompt | Interrupted process; no ASan/LSan output on teardown |

LeakSanitizer runs at exit in every case, so all error paths that end in a thrown `std::runtime_error` are also checked
for leaked memory.

---

## 2. Analysis Results

### ASan findings

| Build | Cases | ASan / LSan reports | Expectation failures |
|---|---|---|---|
| Before fixes (`fb72937`) | 31 | **0** | **24** |
| After fixes | 31 | **0** | **0** |

**ASan reported no memory-safety errors and no leaks, before or after the fixes.** The canary shows the
instrumentation was active. The result fits the design: every file access goes through `std::ifstream::read` into a
fixed-size stack struct (`Elf64_Ehdr`, `programHeader`, `sectionHeader`) or through `get()` into a `std::string`. No
raw buffer is ever indexed with a value taken from the file, so a hostile offset can only cause a failed seek or read,
never an out-of-bounds memory access. The offset-overflow checks added during static analysis (`maxOffset - relOffset`)
were exercised by `offsets_out_of_range` and held.

### Defects found by the dynamic tests

The same test runs did find **six confirmed defects** that ASan cannot detect, because they are not invalid memory
accesses. They are wrong results produced from in-bounds memory. Evidence from the pre-fix build:

| ID | Case(s) | Pre-fix observed behavior |
|---|---|---|
| D1 | all 24 error/unsupported cases | Exit status `0` even when printing `Error: ...` or `unsupported` |
| D2 | `phentsize_zero`, `shentsize_mismatch` | Accepted; printed 13 identical `PHDR R-- 0x40 0x40 728 728` rows |
| D3 | `name_unterminated` | Printed section name `X` built from bytes past the end of `.shstrtab` |
| D4 | `strtab_past_eof`, `strtab_size_huge` | Printed every section with a blank name, no error |
| D5 | `machine_0x013e` | Accepted as supported, then printed `Machine: Unknown` |
| D6 | `shstrndx_undef` | Valid file aborted with `Error: Section name index is out of range` |

Pre-fix output for D2 (`e_phentsize = 0`):

```
Program Headers:
  Type           Flags  Offset      VirtAddr               FileSize   MemSize
  PHDR           R--    0x40        0x40                        728       728
  PHDR           R--    0x40        0x40                        728       728
  PHDR           R--    0x40        0x40                        728       728
  ...
```

Pre-fix output for D3 and D4:

```
  Name              Type       Flags  Offset          Size          (name_unterminated)
                    NULL              0x0                0
  X                 PROGBITS   A      0x318             28

  Name              Type       Flags  Offset          Size          (strtab_past_eof)
                    NULL              0x0                0
                    PROGBITS   A      0x318             28
```

The other 7 cases (normal inputs, two zero-count boundaries, `name_last_byte`, `sigint_at_prompt`) passed before and
after the fixes.

---

## 3. Root Causes and Fixes

### D1 — Errors reported with a success exit status (`src/main.cpp`)

- **Root cause:** `main` caught every exception, printed it, and fell through to `return 0`. `printReport` returned
  `void`, so "unsupported" could not be told apart from success. Scripts or callers could not detect a failure.
  Prompt mode also ignored a failed `std::cin >> filePath`, then tried to open an empty path.
- **Fix:** `printReport` now returns `bool`. `main` returns **0** for a report, **1** for any error, and **2** for an
  unsupported file. A failed prompt read prints `Error: No file path provided` and returns 1.
- **Verified by:** all 24 error/unsupported cases, plus `stdin_closed`.

### D2 — Entry sizes from the header ignored (`Reader::getProgramHeaders` / `getSectionHeaders`)

- **Root cause:** Entry *i* is located at `e_phoff + i * e_phentsize`, but `sizeof(programHeader)` (56) bytes are
  always read. With `e_phentsize = 0` every entry is read from the same offset. With other values the records overlap
  or skip bytes. Each read is in-bounds, but the output is wrong. In a parser that copied `e_phentsize` bytes into a
  fixed struct, the same mismatch would be a stack overflow.
- **Fix:** If the table is non-empty and `e_phentsize != sizeof(programHeader)` (or `e_shentsize != sizeof(sectionHeader)`),
  throw `Unsupported program/section header entry size: N (expected M)`.
- **Verified by:** `phentsize_zero`, `shentsize_mismatch` (now exit 1 with that message). All normal cases still pass.

### D3 — Section-name read not bounded by the string table (`Reader::readCString`)

- **Root cause:** `readCString(offset)` read until a NUL **or EOF**, with no limit. `resolveSectionName` checked that
  the *start* index was inside `sh_size` but not the *end*. An unterminated name therefore kept reading into whatever
  followed `.shstrtab` (here, the section header table). This is an out-of-bounds read at the logic level. ASan stays
  silent only because the bytes come from the file stream, not from a memory buffer. Loading `.shstrtab` into a
  `std::vector` and scanning for NUL the same way would be a heap-buffer-overflow.
- **Fix:** `readCString(offset, maxLen)` reads at most `maxLen` bytes and throws `Unterminated string at offset N` if no
  NUL is found. It also throws on EOF instead of returning a partial string. `resolveSectionName` passes
  `sh_size - nameIndex`.
- **Verified by:** `name_unterminated` (now exit 1, `Unterminated string`). `name_last_byte` checks that the last valid
  byte still resolves (empty name, exit 0).

### D4 — String table location not validated against the file (`Reader::resolveSectionName`)

- **Root cause:** `.shstrtab`'s `sh_offset`/`sh_size` were trusted. When the offset was past EOF, the unbounded read
  hit EOF at once and returned `""`, so the error was hidden as blank names. A huge `sh_size` made the index check
  meaningless.
- **Fix:** Added `Reader::fileSize()`. `resolveSectionName` throws `Section name string table extends past end of file`
  unless `sh_offset <= size && sh_size <= size - sh_offset`. The check is written to avoid overflow. Together with D3's
  EOF check, a short read can no longer turn into an empty name without an error.
- **Verified by:** `strtab_past_eof`, `strtab_size_huge` (now exit 1 with that message).

### D5 — Only the low byte of `e_machine` checked (`Reader::isValidELFFile`)

- **Root cause:** Validation read one `char` at offset 18 and compared it with 62. `e_machine` is a 16-bit field, so any
  value whose low byte is `0x3E` (e.g. `0x013E`) passed as x86-64. The report then contradicted itself
  (`Machine: Unknown`).
- **Fix:** Read both bytes and compose the little-endian `uint16_t` before comparing.
- **Verified by:** `machine_0x013e` (now exit 2, `unsupported`).

### D6 — `SHN_UNDEF` string table index treated as an error (`Reader::resolveSectionName`)

- **Root cause:** `e_shstrndx = 0` is the ELF spec's way of saying "no section name table". The code treated section 0
  (the null section, `sh_size = 0`) as the string table, so the first name lookup threw and the whole section listing
  was lost.
- **Fix:** Return an empty name when `shstrndx == 0`, so the sections are still listed.
- **Verified by:** `shstrndx_undef` (now exit 0 with all 31 sections listed). `shstrndx_one_past` checks that real
  out-of-range indices are still rejected.

### Verification

After the fixes, `dyn_analysis.sh` reports `Total: 31  Passed: 31  Failed: 0  Skipped: 0  ASan reports: 0`. The regular
CMake build (`-Wall -Wextra -Wpedantic -Wconversion -Wshadow`) also compiles with no warnings.

---

## 4. Limitations

- **No ASan reports does not prove memory safety.** ASan only checks the paths that actually ran, and only for the
  error classes it instruments. It does not detect reads of uninitialized memory (MemorySanitizer), integer overflow or
  other undefined behavior (UBSan), or data races (TSan). None of those were run.
- **The defects above were found by the test oracle, not by ASan.** The stream-based design makes memory errors
  unlikely, but it also means ASan has little to check. A future change to `mmap` or read-into-buffer parsing would
  turn D2/D3-style mistakes into real overflows, and these tests would then be the ones to catch them.
- **One seed binary.** Every derived input comes from one PIE executable (`/bin/ls`), plus one relocatable object. Core
  files, static non-PIE executables, stripped files, and files with `e_shnum >= SHN_LORESERVE` (extended section
  numbering via section 0's `sh_size`) were not tested. The extended-numbering case is not supported by the code.
- **Fields that are read but never interpreted are not tested.** Program header `p_offset`/`p_filesz` and section
  `sh_offset`/`sh_size` (other than `.shstrtab`) are printed but never followed, so corrupting them only changes
  printed numbers. `e_ehsize`, `e_version`, and `EI_VERSION` are not validated at all.
- **Partial output before errors.** When a later table is malformed, the ELF header summary has already been printed
  to stdout before the error appears on stderr. The tests accept this; they do not check for it.
- **Interrupted operations are only partly covered.** `sigint_at_prompt` interrupts the process only while it waits for
  input. Signals during file parsing, and a file changing size between `fileSize()` and the reads (a TOCTOU race),
  were not tested. The program installs no signal handlers. Cleanup on a signal relies on the OS closing the file
  descriptor.
- **No fuzzing.** The inputs were designed by hand to reach specific branches. A coverage-guided fuzzer (e.g.
  libFuzzer with `-fsanitize=address,fuzzer`) would explore combinations of corrupt fields that these tests do not.
- **`unreadable_file` is skipped when run as root**, because root ignores file permissions.
