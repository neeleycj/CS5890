#!/usr/bin/env bash
set -euo pipefail

# Static analysis: configure + build under clang's scan-build in a
# separate build dir, writing HTML/plist reports to analysis-results/.
# Compiler warnings are enabled by the target's CMake configuration and are
# intentionally printed during the build rather than treated as analyzer bugs.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT/build-analysis"
OUT_DIR="$ROOT/analysis-results"

command -v scan-build >/dev/null || { echo "scan-build not found (install clang-tools)"; exit 1; }

# Fresh dir so CMake caches scan-build's compiler wrappers, not the old compiler
rm -rf "$BUILD_DIR"
mkdir -p "$OUT_DIR"

SB_ARGS=(
    -o "$OUT_DIR"
    -plist-html
    --use-cc=clang
    --use-c++=clang++
    -analyze-headers
    -enable-checker security
    -enable-checker optin.cplusplus.UninitializedObject
    -enable-checker optin.cplusplus.VirtualCall
    --status-bugs
)

scan-build "${SB_ARGS[@]}" cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug

status=0
scan-build "${SB_ARGS[@]}" cmake --build "$BUILD_DIR" -j"$(nproc)" || status=$?

latest="$(ls -td "$OUT_DIR"/*/ 2>/dev/null | head -n1 || true)"
if [[ $status -ne 0 ]]; then
    if [[ -n "$latest" ]]; then
        echo "Static analyzer bugs found. Report: ${latest}index.html  (or: scan-view $latest)"
    else
        echo "Analysis build failed; no static analyzer report was generated." >&2
    fi
else
    echo "Static analyzer found no bugs. Review compiler warnings printed above."
fi
exit $status
