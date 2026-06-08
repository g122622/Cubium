#!/usr/bin/env bash
# ============================================================
# clang-tidy wrapper for MinecraftReborn
#
# Runs clang-tidy on project source files with the correct
# MSVC / Windows SDK / vcpkg include paths, since the project
# uses clang-cl in its compile database which causes segfaults
# with clang-tidy on Windows.
#
# Usage:
#   ./scripts/tidy.sh                              # Scan all src/ files
#   ./scripts/tidy.sh src/server/world/Foo.cpp      # Scan specific files
#   ./scripts/tidy.sh src/common/**/*.hpp           # Scan with glob
#   ./scripts/tidy.sh --fix src/server/world/Foo.cpp # Auto-fix issues
#   ./scripts/tidy.sh --checks 'bugprone-*' ...      # Override checks
#
# Environment variables:
#   CLANG_TIDY       - Path to clang-tidy (default: auto-detect)
#   MSVC_ROOT        - MSVC tools root (default: auto-detect)
#   WIN_SDK_ROOT     - Windows Kits/10 root (default: D:/Windows Kits)
#   WIN_SDK_VERSION  - SDK version (default: latest installed)
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# ---- Colors (disable if not a terminal) ----
if [[ -t 1 ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    RED='' GREEN='' YELLOW='' NC=''
fi

# ---- Detect clang-tidy ----
CLANG_TIDY="${CLANG_TIDY:-}"
if [[ -z "$CLANG_TIDY" ]]; then
    if command -v clang-tidy &>/dev/null; then
        CLANG_TIDY="$(command -v clang-tidy)"
    elif [[ -x "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-tidy.exe" ]]; then
        CLANG_TIDY="D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-tidy.exe"
    else
        echo -e "${RED}ERROR: clang-tidy not found. Set CLANG_TIDY environment variable.${NC}" >&2
        exit 1
    fi
fi

echo -e "${GREEN}Using clang-tidy: $CLANG_TIDY${NC}"
"$CLANG_TIDY" --version | head -1

# ---- Detect MSVC include path ----
MSVC_ROOT="${MSVC_ROOT:-}"
if [[ -z "$MSVC_ROOT" ]]; then
    # Try VS installation
    VS_BASE="D:/Program Files/Microsoft Visual Studio/18/Community"
    MSVC_TOOL_DIR="$VS_BASE/VC/Tools/MSVC"
    if [[ -d "$MSVC_TOOL_DIR" ]]; then
        MSVC_ROOT="$MSVC_TOOL_DIR/$(ls "$MSVC_TOOL_DIR" | sort -V | tail -1)"
    fi
fi
if [[ -z "$MSVC_ROOT" || ! -d "$MSVC_ROOT/include" ]]; then
    echo -e "${RED}ERROR: MSVC include directory not found. Set MSVC_ROOT environment variable.${NC}" >&2
    exit 1
fi
echo -e "${GREEN}MSVC: $MSVC_ROOT${NC}"

# ---- Detect Windows SDK ----
WIN_SDK_ROOT="${WIN_SDK_ROOT:-D:/Windows Kits}"
WIN_SDK_VERSION="${WIN_SDK_VERSION:-}"
if [[ -z "$WIN_SDK_VERSION" ]]; then
    SDK_INC_DIR="$WIN_SDK_ROOT/10/Include"
    if [[ -d "$SDK_INC_DIR" ]]; then
        WIN_SDK_VERSION="$(ls "$SDK_INC_DIR" | sort -V | tail -1)"
    fi
fi
if [[ -z "$WIN_SDK_VERSION" ]]; then
    echo -e "${RED}ERROR: Windows SDK not found. Set WIN_SDK_ROOT and WIN_SDK_VERSION.${NC}" >&2
    exit 1
fi
SDK_INC="$WIN_SDK_ROOT/10/Include/$WIN_SDK_VERSION"
echo -e "${GREEN}Windows SDK: $SDK_INC${NC}"

# ---- vcpkg includes ----
VCPKG_INC="$BUILD_DIR/vcpkg_installed/x64-windows/include"
if [[ ! -d "$VCPKG_INC" ]]; then
    echo -e "${YELLOW}WARNING: vcpkg include dir not found: $VCPKG_INC${NC}"
fi

# ---- Project includes (deduplicated) ----
PROJECT_INCLUDES=(
    "$PROJECT_ROOT/include"
    "$PROJECT_ROOT/src"
    "$PROJECT_ROOT/src/common"
    "$PROJECT_ROOT/src/common/mod/bedrock/addon"
    "$PROJECT_ROOT/tests"
)

# ---- Build extra-arg list ----
EXTRA_ARGS=(
    --extra-arg="--target=amd64-pc-windows-msvc"
    --extra-arg="-std=c++20"
    --extra-arg="-DNOMINMAX"
    --extra-arg="-DWIN32_LEAN_AND_MEAN"
    --extra-arg="-DWINVER=0x0A00"
    --extra-arg="-D_WIN32_WINNT=0x0A00"
    --extra-arg="-D_CRT_SECURE_NO_WARNINGS"
    --extra-arg="-D_GNU_SOURCE"
)

for inc in "${PROJECT_INCLUDES[@]}"; do
    EXTRA_ARGS+=(--extra-arg="-I$inc")
done
EXTRA_ARGS+=(--extra-arg="-I$MSVC_ROOT/include")
EXTRA_ARGS+=(--extra-arg="-I$SDK_INC/ucrt")
EXTRA_ARGS+=(--extra-arg="-I$SDK_INC/shared")
EXTRA_ARGS+=(--extra-arg="-I$SDK_INC/um")
EXTRA_ARGS+=(--extra-arg="-I$SDK_INC/ucrt")
if [[ -d "$VCPKG_INC" ]]; then
    EXTRA_ARGS+=(--extra-arg="-I$VCPKG_INC")
fi

# ---- Parse arguments ----
TIDY_ARGS=()
FILES=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        --fix|--checks=*|--checks|--header-filter=*|--header-filter)
            TIDY_ARGS+=("$1")
            if [[ "$1" == "--checks" || "$1" == "--header-filter" ]]; then
                shift
                TIDY_ARGS+=("$1")
            fi
            ;;
        --fix-errors|--format-style=*|--format-style|--line-filter=*|--line-filter|--quiet|--warnings-as-errors=*|--warnings-as-errors)
            TIDY_ARGS+=("$1")
            if [[ "$1" == "--format-style" || "$1" == "--line-filter" || "$1" == "--warnings-as-errors" ]]; then
                shift
                TIDY_ARGS+=("$1")
            fi
            ;;
        -h|--help)
            echo "Usage: $0 [clang-tidy-options] <file(s)>..."
            echo ""
            echo "If no files are specified, scans all .cpp files under src/."
            echo "Supports all standard clang-tidy options (--fix, --checks, etc.)."
            exit 0
            ;;
        -*)
            TIDY_ARGS+=("$1")
            ;;
        *)
            FILES+=("$1")
            ;;
    esac
    shift
done

# Default: scan all project source files
if [[ ${#FILES[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No files specified. Scanning all .cpp files under src/...${NC}"
    mapfile -t FILES < <(find "$PROJECT_ROOT/src" -name "*.cpp" -type f 2>/dev/null | sort)
fi

TOTAL=${#FILES[@]}
if [[ $TOTAL -eq 0 ]]; then
    echo -e "${RED}ERROR: No source files found.${NC}" >&2
    exit 1
fi

echo -e "${GREEN}Scanning $TOTAL file(s)...${NC}"
echo ""

# ---- Run clang-tidy ----
PASS=0
FAIL=0
SKIP=0
FAILED_FILES=()

for file in "${FILES[@]}"; do
    # Resolve to absolute path
    if [[ ! -f "$file" ]]; then
        abs_file="$PROJECT_ROOT/$file"
        if [[ ! -f "$abs_file" ]]; then
            echo -e "${YELLOW}SKIP: $file (not found)${NC}"
            ((SKIP++)) || true
            continue
        fi
        file="$abs_file"
    fi

    REL_PATH="${file#$PROJECT_ROOT/}"

    OUTPUT=$("$CLANG_TIDY" "${EXTRA_ARGS[@]}" "${TIDY_ARGS[@]}" "$file" 2>&1) && EXIT_CODE=0 || EXIT_CODE=$?

    # Filter: only show lines from project source (src/ or include/) or summary lines
    FILTERED=$(echo "$OUTPUT" | grep -E "^(E:\\\\dev|src[/\\\\]|Suppressed|[0-9]+ warning|[0-9]+ error)" || true)

    if [[ $EXIT_CODE -eq 0 ]]; then
        if [[ -n "$FILTERED" ]]; then
            echo -e "${YELLOW}WARN: $REL_PATH${NC}"
            echo "$FILTERED"
        else
            echo -e "${GREEN}  OK: $REL_PATH${NC}"
        fi
        ((PASS++)) || true
    elif [[ $EXIT_CODE -eq 139 ]]; then
        echo -e "${RED}CRASH: $REL_PATH (segfault)${NC}"
        ((SKIP++)) || true
    else
        echo -e "${RED}FAIL: $REL_PATH${NC}"
        echo "$FILTERED"
        ((FAIL++)) || true
        FAILED_FILES+=("$REL_PATH")
    fi
done

# ---- Summary ----
echo ""
echo "=========================================="
echo -e "  Total: $TOTAL  ${GREEN}Pass: $PASS${NC}  ${RED}Fail: $FAIL${NC}  ${YELLOW}Skip: $SKIP${NC}"
echo "=========================================="

if [[ ${#FAILED_FILES[@]} -gt 0 ]]; then
    echo ""
    echo -e "${RED}Failed files:${NC}"
    for f in "${FAILED_FILES[@]}"; do
        echo "  - $f"
    done
fi

exit $FAIL
