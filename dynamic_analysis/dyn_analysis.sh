#!/usr/bin/env bash
# Build the ELF reader with Clang AddressSanitizer and run every case listed in
# tests/cases.txt. A case passes when the exit code and output match what is
# expected AND no AddressSanitizer / LeakSanitizer report was produced.
#
# Usage: dynamic_analysis/dyn_analysis.sh [--src DIR]
#   --src DIR   build the sources in DIR/src and DIR/include instead of this
#               checkout (e.g. a `git worktree` of an older commit, to reproduce
#               pre-fix results against the same test suite).
#
# Requirements: clang++ (with compiler-rt ASan runtime), bash, coreutils.
# Per-case logs are written to dynamic_analysis/results/.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TESTS_DIR="$SCRIPT_DIR/tests"
BIN_DIR="$SCRIPT_DIR/build"
RESULTS_DIR="$SCRIPT_DIR/results"
TARGET="$BIN_DIR/elf_reader_asan"
CASES="$TESTS_DIR/cases.txt"

if [ "${1:-}" = "--src" ] && [ -n "${2:-}" ]; then
    SRC_ROOT="$(cd "$2" && pwd)"
fi

# 1. Build with ASan (flags from the Clang AddressSanitizer documentation:
#    -fsanitize=address, -O1 for reasonable speed, -g and frame pointers for
#    readable stack traces).
rm -rf "$BIN_DIR" "$RESULTS_DIR"
mkdir -p "$BIN_DIR" "$RESULTS_DIR"

echo "[*] Building $SRC_ROOT with Clang AddressSanitizer..."
if ! clang++ -std=c++17 -O1 -g -fsanitize=address -fno-omit-frame-pointer \
        -fsanitize-address-use-after-scope \
        -I"$SRC_ROOT/include" "$SRC_ROOT"/src/*.cpp -o "$TARGET"; then
    echo "[-] Build failed."
    exit 1
fi
echo "[+] Built $TARGET"

# 2. ASan runtime options.
#    detect_leaks=1                  LeakSanitizer on exit (resource cleanup)
#    detect_stack_use_after_return=1 catch dangling stack references
#    check_initialization_order=1    catch static init order bugs
#    strict_string_checks=1          check full extent of C string arguments
#    exitcode=86                     distinguish ASan aborts from the program's own codes
export ASAN_OPTIONS="detect_leaks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_string_checks=1:halt_on_error=1:exitcode=86:color=never"

# 3. Confirm ASan is active by running a program with a known heap overflow.
echo "[*] Sanity check: running ASan canary (expects a heap-buffer-overflow report)..."
clang++ -O1 -g -fsanitize=address -fno-omit-frame-pointer \
    "$TESTS_DIR/asan_canary.cpp" -o "$BIN_DIR/asan_canary" 2>/dev/null
"$BIN_DIR/asan_canary" >"$RESULTS_DIR/asan_canary.log" 2>&1
if grep -q 'ERROR: AddressSanitizer: heap-buffer-overflow' "$RESULTS_DIR/asan_canary.log"; then
    echo "[+] ASan detected the seeded overflow; instrumentation is working."
else
    echo "[-] ASan did NOT detect the seeded overflow; results below are not trustworthy."
    exit 1
fi
echo

# 4. Run the cases.
TMP_DIR="$(mktemp -d)"
trap 'chmod -R u+rw "$TMP_DIR" 2>/dev/null; rm -rf "$TMP_DIR"' EXIT

trim() { local s="$1"; s="${s#"${s%%[![:space:]]*}"}"; printf '%s' "${s%"${s##*[![:space:]]}"}"; }

total=0; passed=0; failed=0; skipped=0; asan_reports=0
printf '%-10s %-24s %-6s %-6s %s\n' CATEGORY CASE RC RESULT NOTE

while IFS='|' read -r category name mode arg expected_rc expected_text; do
    category="$(trim "$category")"
    [ -z "$category" ] || [ "${category:0:1}" = "#" ] && continue
    name="$(trim "$name")"; mode="$(trim "$mode")"; arg="$(trim "$arg")"
    expected_rc="$(trim "$expected_rc")"; expected_text="$(trim "$expected_text")"

    log="$RESULTS_DIR/${category}_${name}.log"
    input="$TESTS_DIR/$arg"
    total=$((total + 1))

    case "$mode" in
        file)       "$TARGET" "$input" >"$log" 2>&1 ;;
        prompt)     printf '%s\n' "$input" | "$TARGET" >"$log" 2>&1 ;;
        stdin_eof)  "$TARGET" </dev/null >"$log" 2>&1 ;;
        missing)    "$TARGET" "$TMP_DIR/does_not_exist.elf" >"$log" 2>&1 ;;
        dir)        "$TARGET" "$input" >"$log" 2>&1 ;;
        unreadable)
            if [ "$(id -u)" -eq 0 ]; then
                printf '%-10s %-24s %-6s %-6s %s\n' "$category" "$name" "-" SKIP "root ignores file permissions"
                skipped=$((skipped + 1)); continue
            fi
            cp "$input" "$TMP_DIR/unreadable.elf"; chmod 000 "$TMP_DIR/unreadable.elf"
            "$TARGET" "$TMP_DIR/unreadable.elf" >"$log" 2>&1 ;;
        pipe)       cat "$input" | "$TARGET" /dev/stdin >"$log" 2>&1 ;;
        pipe_trunc) head -c 100 "$input" | "$TARGET" /dev/stdin >"$log" 2>&1 ;;
        sigint)     sleep 5 | timeout -s INT 1 "$TARGET" >"$log" 2>&1 ;;
        *)          echo "Unknown mode '$mode' for case $name" >"$log"; false ;;
    esac
    rc=$?

    note=""
    if grep -qE 'ERROR: (AddressSanitizer|LeakSanitizer)' "$log"; then
        asan_reports=$((asan_reports + 1))
        note="ASan report: $(grep -m1 -oE 'ERROR: [A-Za-z]+Sanitizer: [^ ]+' "$log")"
    elif [ "$rc" != "$expected_rc" ]; then
        note="expected exit $expected_rc"
    elif [ "$expected_text" != "-" ] && ! grep -qF -- "$expected_text" "$log"; then
        note="missing output: \"$expected_text\""
    fi

    if [ -z "$note" ]; then
        result=PASS; passed=$((passed + 1))
    else
        result=FAIL; failed=$((failed + 1))
    fi
    printf '%-10s %-24s %-6s %-6s %s\n' "$category" "$name" "$rc" "$result" "$note"
done <"$CASES"

echo
echo "Total: $total  Passed: $passed  Failed: $failed  Skipped: $skipped  ASan reports: $asan_reports"
echo "Logs: $RESULTS_DIR"

[ "$failed" -eq 0 ]
