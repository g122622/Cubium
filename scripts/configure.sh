#!/usr/bin/env bash
# ============================================================
# Cubium - Build wrapper for Git Bash / MSYS2 / CI
#
# This script invokes configure.bat through cmd.exe, which
# automatically sets up the Visual Studio developer environment
# before running CMake. This avoids needing to manually open
# Developer Command Prompt.
#
# Usage:
#   ./configure.sh                          - Configure (relwithdebinfo)
#   ./configure.sh build                    - Configure + build
#   ./configure.sh --preset windows-clang-debug  - Custom cmake args
#
# For CI pipelines, call configure.bat directly:
#   cmd /c configure.bat
#   cmd /c configure.bat build
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BAT_PATH="$(cygpath -w "$SCRIPT_DIR/configure.bat")"

# Helper: format seconds as mm:ss
format_duration() {
    local total=$1
    local mins=$((total / 60))
    local secs=$((total % 60))
    printf "%02d:%02d" "$mins" "$secs"
}

if [ $# -eq 0 ]; then
    exec cmd //c "$BAT_PATH"
elif [ "$1" = "build" ]; then
    START=$(date +%s)
    cmd //c "$BAT_PATH" build
    EXIT_CODE=$?
    END=$(date +%s)
    ELAPSED=$((END - START))
    echo ""
    echo "=== Build Summary ==="
    echo "  Duration:  $(format_duration $ELAPSED)"
    echo "  Exit code: $EXIT_CODE"
    exit $EXIT_CODE
else
    # Pass all args through - need to forward them as a single string
    exec cmd //c "$BAT_PATH" "$@"
fi
