#!/usr/bin/env bash
set -u  # Exit if unset variables are referenced

# Determine directories relative to script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TESTS_DIR="$SCRIPT_DIR/tests"
BIN_DIR="$SCRIPT_DIR/build"
TARGET="$BIN_DIR/elf_reader_asan"


# 1. Clean and prepare build output folder
rm -rf "$BIN_DIR"
mkdir -p "$BIN_DIR"

# 2. Compile with Clang ASan
echo "[*] Compiling project with Clang AddressSanitizer..."

clang++ -std=c++17 -O1 -g -fsanitize=address -fno-omit-frame-pointer \
    -I"$PROJECT_ROOT/include" \
    "$PROJECT_ROOT/src"/*.cpp \
    -o "$TARGET"


if [ ! -f "$TARGET" ]; then
    echo "[-] Build failed. Exiting."
    exit 1
fi
echo "[+] Build successful: $TARGET"
echo ""

# 3. Configure ASan Runtime Options
# detect_leaks=1 checks for memory leaks on exit
# color=always ensures stack traces are legible
# abort_on_error=0 allows the program to terminate via standard exit code (usually 1 or ASan default)
export ASAN_OPTIONS="detect_leaks=1:color=always:abort_on_error=0"

# 4. Run Test Cases
echo "=================================================="
echo "  Executing Test Cases from $TESTS_DIR"
echo "=================================================="

TOTAL_TESTS=0
ASAN_FAILURES=0

for test_file in "$TESTS_DIR"/*; do
    # Skip directories or the test generator scripts
    if [ -d "$test_file" ] || [[ "$test_file" == *.sh ]] || [[ "$test_file" == *.py ]]; then
        continue
    fi

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    test_name="$(basename "$test_file")"
    echo "--------------------------------------------------"
    echo "Running test [$TOTAL_TESTS]: $test_name"

    # Run target binary with the test file
    "$TARGET" "$test_file"
    EXIT_CODE=$?

    # ASan reports memory violations with exit status 1
    # Check if ASan caught an issue
    if [ $EXIT_CODE -gt 128 ] || [ $EXIT_CODE -eq 1 ]; then
        echo "[!] Program exited with code $EXIT_CODE (Check above for potential ASan crash/report)"
        ASAN_FAILURES=$((ASAN_FAILURES + 1))
        # Note: Non-zero exit code might just be clean rejection of invalid files
    else
        echo "[+] Clean run (exit code: $EXIT_CODE)"
    fi
done

echo ""
echo "=================================================="
echo "  Summary"
echo "=================================================="
echo "Total tests run: $TOTAL_TESTS"
echo "Tests with ASan failures: $ASAN_FAILURES"