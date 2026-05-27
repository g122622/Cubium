#!/usr/bin/env bash
# ============================================================
# Minecraft Reborn - Build wrapper for Git Bash / MSYS2 / CI
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

if [ $# -eq 0 ]; then
    exec cmd //c "$BAT_PATH"
elif [ "$1" = "build" ]; then
    exec cmd //c "$BAT_PATH" build
else
    # Pass all args through - need to forward them as a single string
    exec cmd //c "$BAT_PATH" "$@"
fi
