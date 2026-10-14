#!/usr/bin/env bash
# ============================================================
# clang-tidy wrapper for MinecraftReborn
#
# Runs clang-tidy with the platform-specific compile settings:
# Windows injects MSVC / Windows SDK / vcpkg include paths because
# clang-tidy can segfault with the project's clang-cl database; Linux
# uses the CMake compile_commands.json directly.
#
# Usage:
#   ./scripts/tidy.sh                              # Scan all src/ files
#   ./scripts/tidy.sh src/server/world/Foo.cpp      # Scan specific files
#   ./scripts/tidy.sh --jobs 8 src/server/world       # Scan a directory with 8 workers
#   ./scripts/tidy.sh src/common/**/*.hpp           # Scan with glob
#   ./scripts/tidy.sh --fix src/server/world/Foo.cpp # Auto-fix issues
#   ./scripts/tidy.sh --checks 'bugprone-*' ...      # Override checks
#
# Environment variables:
#   CLANG_TIDY       - Path to clang-tidy (default: auto-detect)
#   MSVC_ROOT        - MSVC tools root (default: auto-detect)
#   WIN_SDK_ROOT     - Windows Kits/10 root (default: D:/Windows Kits)
#   WIN_SDK_VERSION  - SDK version (default: latest installed)
#   TIDY_JOBS        - Maximum parallel clang-tidy processes (default: logical CPU count)
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

WINDOWS_TIDY=0
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) WINDOWS_TIDY=1 ;;
esac

if (( WINDOWS_TIDY )); then
    # ---- Detect MSVC include path ----
    MSVC_ROOT="${MSVC_ROOT:-}"
    if [[ -z "$MSVC_ROOT" ]]; then
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

    VCPKG_INC="$BUILD_DIR/vcpkg_installed/x64-windows/include"
    if [[ ! -d "$VCPKG_INC" ]]; then
        echo -e "${YELLOW}WARNING: vcpkg include dir not found: $VCPKG_INC${NC}"
    fi

    PROJECT_INCLUDES=(
        "$PROJECT_ROOT/include"
        "$PROJECT_ROOT/src"
        "$PROJECT_ROOT/src/common"
        "$PROJECT_ROOT/src/common/mod/bedrock/addon"
        "$PROJECT_ROOT/tests"
    )
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
    if [[ -d "$VCPKG_INC" ]]; then
        EXTRA_ARGS+=(--extra-arg="-I$VCPKG_INC")
    fi
else
    if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
        echo -e "${RED}ERROR: compile_commands.json not found at $BUILD_DIR. Configure the project first.${NC}" >&2
        exit 1
    fi
    EXTRA_ARGS=(-p "$BUILD_DIR")
fi

# ---- Parse arguments ----
TIDY_ARGS=()
FILES=()
JOBS="${TIDY_JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1)}"
if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo -e "${RED}ERROR: TIDY_JOBS must be a positive integer.${NC}" >&2
    exit 1
fi
while [[ $# -gt 0 ]]; do
    case "$1" in
        --jobs)
            shift
            if [[ $# -eq 0 || ! "$1" =~ ^[1-9][0-9]*$ ]]; then
                echo -e "${RED}ERROR: --jobs requires a positive integer.${NC}" >&2
                exit 1
            fi
            JOBS="$1"
            ;;
        --jobs=*)
            JOBS="${1#*=}"
            if ! [[ "$JOBS" =~ ^[1-9][0-9]*$ ]]; then
                echo -e "${RED}ERROR: --jobs requires a positive integer.${NC}" >&2
                exit 1
            fi
            ;;
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
            echo "Directories are scanned recursively for C++ source and header files."
            echo "Parallel workers default to the logical processor count; override with --jobs N or TIDY_JOBS."
            echo "Supports clang-tidy options such as --fix, --checks, and --warnings-as-errors."
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

for arg in "${TIDY_ARGS[@]}"; do
    if [[ "$arg" == "--fix" || "$arg" == "--fix-errors" ]]; then
        JOBS=1
        echo -e "${YELLOW}Auto-fix mode uses one worker to avoid concurrent edits.${NC}"
        break
    fi
done

# Default: scan all project source files
if [[ ${#FILES[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No files specified. Scanning all .cpp files under src/...${NC}"
    mapfile -t FILES < <(find "$PROJECT_ROOT/src" -name "*.cpp" -type f 2>/dev/null | sort)
fi

EXPANDED_FILES=()
for file in "${FILES[@]}"; do
    if [[ -d "$file" ]]; then
        while IFS= read -r -d '' found; do
            EXPANDED_FILES+=("$found")
        done < <(find "$file" -type f \( -iname '*.cpp' -o -iname '*.cc' -o -iname '*.cxx' -o -iname '*.hpp' -o -iname '*.h' \) -print0 | sort -z)
    else
        EXPANDED_FILES+=("$file")
    fi
done
FILES=("${EXPANDED_FILES[@]}")

TOTAL=${#FILES[@]}
if [[ $TOTAL -eq 0 ]]; then
    echo -e "${RED}ERROR: No source files found.${NC}" >&2
    exit 1
fi

echo -e "${GREEN}Scanning $TOTAL file(s)...${NC}"
if (( JOBS > TOTAL )); then JOBS=$TOTAL; fi
echo -e "${GREEN}Parallel workers: $JOBS (logical processors: $(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 1))${NC}"
echo ""

# ---- Run clang-tidy ----
PASS=0
FAIL=0
SKIP=0
FAILED_FILES=()
DIAGNOSTIC_COUNTS=()
TOTAL_FILE_SECONDS=0
WALL_START=$SECONDS
TMP_DIR="$(mktemp -d)"
declare -A RUNNING_FILES=()
declare -A RUNNING_OUTPUTS=()
declare -A RUNNING_META=()

finish_worker() {
    local pid="$1" file="$2" output_file="$3" meta_file="$4"
    local exit_code elapsed rel_path filtered diagnostics check
    wait "$pid" || true
    read -r exit_code elapsed < "$meta_file"
    rel_path="${file#"$PROJECT_ROOT"/}"
    TOTAL_FILE_SECONDS=$(awk -v total="$TOTAL_FILE_SECONDS" -v value="$elapsed" 'BEGIN { printf "%.6f", total + value }')
    filtered=$(grep -E '^([A-Za-z]:[\\/]|/|src[/\\]|Suppressed|[0-9]+ warning|[0-9]+ error)' "$output_file" || true)
    diagnostics=$(grep -E ':[[:space:]]*(warning|error):' "$output_file" || true)

    while IFS= read -r line; do
        if [[ "$line" =~ :([0-9]+):([0-9]+:)?[[:space:]](warning|error):.*\[([A-Za-z0-9_.-]+) ]]; then
            check="${BASH_REMATCH[4]}"
            DIAGNOSTIC_COUNTS+=("$check")
        fi
    done < "$output_file"

    if [[ $exit_code -eq 0 ]]; then
        if [[ -n "$diagnostics" ]]; then
            echo -e "${YELLOW}WARN: $rel_path${NC}"
            printf '%s\n' "$filtered" | grep -Ev '^(Suppressed|[0-9]+ warnings? generated)' || true
        else
            echo -e "${GREEN}  OK: $rel_path${NC}"
        fi
        ((PASS++)) || true
    elif [[ $exit_code -eq 139 ]]; then
        echo -e "${RED}CRASH: $rel_path (segfault)${NC}"
        ((SKIP++)) || true
    else
        echo -e "${RED}FAIL: $rel_path${NC}"
        if [[ -n "$diagnostics" ]]; then
            details=$(printf '%s\n' "$filtered" | grep -Ev '^(Suppressed|[0-9]+ warnings? generated)' || true)
            if [[ -n "$details" ]]; then
                printf '%s\n' "$details"
            else
                cat "$output_file"
            fi
        else
            cat "$output_file"
        fi
        ((FAIL++)) || true
        FAILED_FILES+=("$rel_path")
    fi
    echo -e "${NC}  Time: ${elapsed}s"
    rm -f "$output_file" "$meta_file"
}

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

    abs_file="$file"
    if [[ ! -f "$abs_file" ]]; then abs_file="$PROJECT_ROOT/$file"; fi
    output_file="$TMP_DIR/$((PASS + FAIL + SKIP + ${#RUNNING_FILES[@]})).out"
    meta_file="$output_file.meta"
    (
        started=$(date +%s.%N)
        if "$CLANG_TIDY" "${EXTRA_ARGS[@]}" "${TIDY_ARGS[@]}" "$abs_file" > "$output_file" 2>&1; then
            exit_code=0
        else
            exit_code=$?
        fi
        ended=$(date +%s.%N)
        elapsed=$(awk -v start="$started" -v end="$ended" 'BEGIN { printf "%.2f", end - start }')
        printf '%s %s\n' "$exit_code" "$elapsed" > "$meta_file"
    ) &
    pid=$!
    RUNNING_FILES[$pid]="$abs_file"
    RUNNING_OUTPUTS[$pid]="$output_file"
    RUNNING_META[$pid]="$meta_file"

    while (( ${#RUNNING_FILES[@]} >= JOBS )); do
        completed_pid=""
        for running_pid in "${!RUNNING_FILES[@]}"; do
            if ! kill -0 "$running_pid" 2>/dev/null; then
                completed_pid="$running_pid"
                break
            fi
        done
        if [[ -n "$completed_pid" ]]; then
            finish_worker "$completed_pid" "${RUNNING_FILES[$completed_pid]}" "${RUNNING_OUTPUTS[$completed_pid]}" "${RUNNING_META[$completed_pid]}"
            unset 'RUNNING_FILES[$completed_pid]' 'RUNNING_OUTPUTS[$completed_pid]' 'RUNNING_META[$completed_pid]'
        else
            sleep 0.1
        fi
    done
done

while (( ${#RUNNING_FILES[@]} > 0 )); do
    completed_pid=""
    for running_pid in "${!RUNNING_FILES[@]}"; do
        if ! kill -0 "$running_pid" 2>/dev/null; then
            completed_pid="$running_pid"
            break
        fi
    done
    if [[ -n "$completed_pid" ]]; then
        finish_worker "$completed_pid" "${RUNNING_FILES[$completed_pid]}" "${RUNNING_OUTPUTS[$completed_pid]}" "${RUNNING_META[$completed_pid]}"
        unset 'RUNNING_FILES[$completed_pid]' 'RUNNING_OUTPUTS[$completed_pid]' 'RUNNING_META[$completed_pid]'
    else
        sleep 0.1
    fi
done
WALL_SECONDS=$((SECONDS - WALL_START))

# ---- Summary ----
echo ""
echo "=========================================="
echo -e "  Total: $TOTAL  ${GREEN}Pass: $PASS${NC}  ${RED}Fail: $FAIL${NC}  ${YELLOW}Skip: $SKIP${NC}"
if (( PASS + FAIL > 0 )); then
    average=$(awk -v total="$TOTAL_FILE_SECONDS" -v count="$((PASS + FAIL))" 'BEGIN { printf "%.2f", total / count }')
    echo "  Wall time: ${WALL_SECONDS}s  Mean per-file time: ${average}s"
fi
echo "=========================================="

if [[ ${#DIAGNOSTIC_COUNTS[@]} -gt 0 ]]; then
    echo ""
    echo "Diagnostics by check:"
    printf '%s\n' "${DIAGNOSTIC_COUNTS[@]}" | sort | uniq -c | sort -nr | awk '{ printf "  %s: %s\n", $2, $1 }'
fi

if [[ ${#FAILED_FILES[@]} -gt 0 ]]; then
    echo ""
    echo -e "${RED}Failed files:${NC}"
    for f in "${FAILED_FILES[@]}"; do
        echo "  - $f"
    done
fi

exit $FAIL
