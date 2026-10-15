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
#   ./scripts/tidy.sh --changed-lines=ranges.tsv ...  # Only fail on changed lines
#
# Environment variables:
#   CLANG_TIDY       - Path to clang-tidy (default: auto-detect)
#   MSVC_ROOT        - MSVC tools root (default: auto-detect)
#   WIN_SDK_ROOT     - Windows Kits/10 root (default: D:/Windows Kits)
#   WIN_SDK_VERSION  - SDK version (default: latest installed)
#   TIDY_JOBS        - Maximum parallel clang-tidy processes (default: logical CPU count)
#   TIDY_COMPILE_DB_DIR - Linux compilation database directory (default: build/)
#   TIDY_CHANGED_LINES  - Same as --changed-lines (CI passes it for pull requests)
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Native (OS-specific) form of the project root, without a trailing slash. clang-tidy
# prints absolute paths in the host format ("E:/dev/..." on Windows, "/home/..." on
# Linux), so changed-line matching needs this form rather than the MSYS "/e/dev/...".
# Falls back to the POSIX path when the host has no drive-letter mapping.
PROJECT_ROOT_ABS="$PROJECT_ROOT"
case "$(uname -s 2>/dev/null || echo unknown)" in
    MINGW*|MSYS*|CYGWIN*)
        if command -v cygpath >/dev/null 2>&1; then
            PROJECT_ROOT_ABS="$(cygpath -m "$PROJECT_ROOT")"
        fi
        ;;
esac
PROJECT_ROOT_ABS="${PROJECT_ROOT_ABS%/}"

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
        # 测试私有头以 tests/unit 为根表达（与 mc_tests 的 target_include_directories 一致），
        # 例如 tests/unit/common/BlockInteractionTestHelper.hpp 的 include 写法是
        # "common/BlockInteractionTestHelper.hpp"。
        "$PROJECT_ROOT/tests/unit"
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
    TIDY_COMPILE_DB_DIR="${TIDY_COMPILE_DB_DIR:-$BUILD_DIR}"
    if [[ ! -f "$TIDY_COMPILE_DB_DIR/compile_commands.json" ]]; then
        echo -e "${RED}ERROR: compile_commands.json not found at $TIDY_COMPILE_DB_DIR. Configure the project first.${NC}" >&2
        exit 1
    fi
    EXTRA_ARGS=(-p "$TIDY_COMPILE_DB_DIR")
fi

# ---- Parse arguments ----
TIDY_ARGS=()
FILES=()
CHANGED_LINES_FILE="${TIDY_CHANGED_LINES:-}"
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
        --changed-lines)
            shift
            if [[ $# -eq 0 ]]; then
                echo -e "${RED}ERROR: --changed-lines requires a file path.${NC}" >&2
                exit 1
            fi
            CHANGED_LINES_FILE="$1"
            ;;
        --changed-lines=*)
            CHANGED_LINES_FILE="${1#*=}"
            ;;
        --fix|--checks=*|--checks|--header-filter=*|--header-filter)
            TIDY_ARGS+=("$1")
            if [[ "$1" == "--checks" || "$1" == "--header-filter" ]]; then
                if [[ $# -lt 2 ]]; then
                    echo -e "${RED}ERROR: $1 requires a value.${NC}" >&2
                    exit 1
                fi
                shift
                TIDY_ARGS+=("$1")
            fi
            ;;
        --fix-errors|--format-style=*|--format-style|--line-filter=*|--line-filter|--quiet|--warnings-as-errors=*|--warnings-as-errors)
            TIDY_ARGS+=("$1")
            if [[ "$1" == "--format-style" || "$1" == "--line-filter" || "$1" == "--warnings-as-errors" ]]; then
                if [[ $# -lt 2 ]]; then
                    echo -e "${RED}ERROR: $1 requires a value.${NC}" >&2
                    exit 1
                fi
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
            echo ""
            echo "--changed-lines <tsv>  Only fail on diagnostics inside the given line ranges."
            echo "                       The TSV is '<path>\\t<first>\\t<last>' per record, produced by"
            echo "                       scripts/ci/changed_lines.py. Without it every diagnostic counts."
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

# ---- Changed-line filtering (pull-request gate) ----
# clang-tidy's own --line-filter matches on the bare file name only, so two files
# sharing a name (the repo has several) would share line ranges. We therefore run
# clang-tidy unfiltered and drop out-of-range diagnostics ourselves, keyed on the
# repo-relative path plus line number.
CHANGED_LINES_ACTIVE=0
declare -A CHANGED_LINE_RANGES=()
if [[ -n "$CHANGED_LINES_FILE" ]]; then
    if [[ ! -f "$CHANGED_LINES_FILE" ]]; then
        echo -e "${RED}ERROR: changed-lines file not found: $CHANGED_LINES_FILE${NC}" >&2
        exit 1
    fi
    while IFS=$'\t' read -r changed_path changed_first changed_last; do
        [[ -z "$changed_path" || -z "$changed_first" || -z "$changed_last" ]] && continue
        # Strip a stray CR: the TSV may have been produced on Windows with CRLF endings,
        # which would otherwise land inside the numeric field and break the comparison.
        changed_first="${changed_first//$'\r'/}"
        changed_last="${changed_last//$'\r'/}"
        CHANGED_LINE_RANGES["$changed_path"]+="${changed_first} ${changed_last}"$'\n'
    done < "$CHANGED_LINES_FILE"
    CHANGED_LINES_ACTIVE=1
fi

# Whether a diagnostic at <path>:<line> is inside the changed ranges. When no ranges
# are known for the file, the whole file counts as unchanged.
is_changed_line() {
    local path="$1" line="$2" range first last
    [[ $CHANGED_LINES_ACTIVE -eq 1 ]] || return 0
    local ranges="${CHANGED_LINE_RANGES[$path]:-}"
    [[ -z "$ranges" ]] && return 1
    while read -r first last; do
        [[ -z "$first" ]] && continue
        if (( line >= first && line <= last )); then
            return 0
        fi
    done <<< "$ranges"
    return 1
}

# Rewrite a raw diagnostic line as "<repo-relative-path>\t<line>\t<kind>\t<message>",
# or return non-zero when it does not match the clang-tidy diagnostic shape.
#
# Two patterns are needed because the column is optional in clang-tidy's output. Both
# use a greedy path group: POSIX ERE has no non-greedy quantifier, and a greedy group
# backtracks to the right split as long as the trailing ":(line):(col)?" is mandatory
# in the pattern — which is why the column-less form is matched separately rather than
# folded in with "?".
normalize_diagnostic() {
    local raw="$1" abs_path rel_path line kind message
    if [[ "$raw" =~ ^(.*):([0-9]+):([0-9]+):[[:space:]]*(warning|error):[[:space:]]*(.*)$ ]]; then
        abs_path="${BASH_REMATCH[1]}"
        line="${BASH_REMATCH[2]}"
        kind="${BASH_REMATCH[4]}"
        message="${BASH_REMATCH[5]}"
    elif [[ "$raw" =~ ^(.*):([0-9]+):[[:space:]]*(warning|error):[[:space:]]*(.*)$ ]]; then
        abs_path="${BASH_REMATCH[1]}"
        line="${BASH_REMATCH[2]}"
        kind="${BASH_REMATCH[3]}"
        message="${BASH_REMATCH[4]}"
    else
        return 1
    fi
    # Normalise backslashes so a Windows path matches the git-relative TSV key.
    abs_path="${abs_path//\\//}"
    rel_path="${abs_path#"$PROJECT_ROOT_ABS"/}"
    printf '%s\t%s\t%s\t%s\n' "$rel_path" "$line" "$kind" "$message"
}

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
trap 'rm -rf "$TMP_DIR"' EXIT
declare -A RUNNING_FILES=()
declare -A RUNNING_OUTPUTS=()
declare -A RUNNING_META=()

finish_worker() {
    local pid="$1" file="$2" output_file="$3" meta_file="$4"
    local exit_code elapsed rel_path raw normalized diag_path diag_line diag_kind diag_message
    local -a in_scope=()
    local out_of_scope_count=0
    wait "$pid" || true
    read -r exit_code elapsed < "$meta_file"
    rel_path="${file#"$PROJECT_ROOT"/}"
    TOTAL_FILE_SECONDS=$(awk -v total="$TOTAL_FILE_SECONDS" -v value="$elapsed" 'BEGIN { printf "%.6f", total + value }')

    # Collect the diagnostics that survive changed-line filtering. Each entry is the
    # original clang-tidy line, so the report keeps its native formatting. Diagnostics
    # dropped by the filter are counted so a nonzero exit can be told apart from a
    # genuine invocation failure (see the branch below).
    while IFS= read -r raw; do
        [[ -z "$raw" ]] && continue
        normalized="$(normalize_diagnostic "$raw")" || { in_scope+=("$raw"); continue; }
        IFS=$'\t' read -r diag_path diag_line diag_kind diag_message <<< "$normalized"
        if is_changed_line "$diag_path" "$diag_line"; then
            in_scope+=("$raw")
        else
            ((out_of_scope_count++)) || true
        fi
    done < <(grep -E ':[[:space:]]*(warning|error):' "$output_file" || true)

    # Count the checks that survived, so the summary reflects what actually failed.
    for raw in "${in_scope[@]}"; do
        if [[ "$raw" =~ \[([A-Za-z0-9_.-]+)(,-warnings-as-errors)?\] ]]; then
            DIAGNOSTIC_COUNTS+=("${BASH_REMATCH[1]}")
        fi
    done

    if [[ $exit_code -eq 139 ]]; then
        # A segfault carries no usable diagnostics, so changed-line filtering cannot
        # apply: report it regardless of where it happened.
        echo -e "${RED}CRASH: $rel_path (segfault)${NC}"
        ((FAIL++)) || true
        FAILED_FILES+=("$rel_path")
    elif [[ $exit_code -ne 0 && ${#in_scope[@]} -eq 0 && $out_of_scope_count -gt 0 ]]; then
        # Nonzero exit, but every diagnostic it emitted sits outside the changed lines:
        # the only failures are pre-existing backlog, so the file passes the gate.
        echo -e "${GREEN}  OK: $rel_path${NC} ${YELLOW}(diagnostics outside changed lines)${NC}"
        ((PASS++)) || true
    elif [[ $exit_code -ne 0 && ${#in_scope[@]} -eq 0 ]]; then
        # Nonzero exit with no diagnostic at all: clang-tidy itself failed (bad
        # invocation, missing compile-database entry, unparsable output). Reporting OK
        # here would let the gate pass without having checked the file, so fail loudly.
        echo -e "${RED}FAIL: $rel_path${NC} ${YELLOW}(clang-tidy exited $exit_code without diagnostics)${NC}"
        cat "$output_file"
        ((FAIL++)) || true
        FAILED_FILES+=("$rel_path")
    elif [[ $exit_code -ne 0 ]]; then
        echo -e "${RED}FAIL: $rel_path${NC}"
        printf '%s\n' "${in_scope[@]}"
        ((FAIL++)) || true
        FAILED_FILES+=("$rel_path")
    elif [[ ${#in_scope[@]} -gt 0 ]]; then
        echo -e "${YELLOW}WARN: $rel_path${NC}"
        printf '%s\n' "${in_scope[@]}"
        ((PASS++)) || true
    else
        echo -e "${GREEN}  OK: $rel_path${NC}"
        ((PASS++)) || true
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
