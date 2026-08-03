#!/usr/bin/env bash
# ============================================================
# clang-include-cleaner wrapper for Cubium
#
# 离线分析 #include 冗余/缺失，基于 compile_commands.json。
# 不挂钩编译，不影响日常构建。
#
# 与 tidy.sh 的关键差异：
#   - 走 -p build/ 消费 compile_commands.json（取 -D 宏、项目 -I、vcpkg -isystem）
#   - 只用 --extra-arg 补 MSVC/SDK/clang-builtin 系统头（项目 -I/vcpkg 已在 database）
#   - 用 --ignore-headers 排除第三方头噪音
#
# Usage:
#   ./scripts/iwyu/iwyu.sh                              # 扫试点 src/common/core
#   ./scripts/iwyu/iwyu.sh src/common/core/Result.cpp   # 扫指定文件
#   ./scripts/iwyu/iwyu.sh src/common/core              # 扫目录下所有 .cpp
#   ./scripts/iwyu/iwyu.sh --edit src/common/core/      # 应用建议（危险！先 git status 干净）
#   ./scripts/iwyu/iwyu.sh --remove src/common/core/    # 只建议移除头
#   ./scripts/iwyu/iwyu.sh --insert src/common/core/    # 只建议插入头
#   ./scripts/iwyu/iwyu.sh --print src/common/core/     # 打印最终代码（默认 --print=changes）
#   ./scripts/iwyu/iwyu.sh --html=report.html ...       # 输出 HTML 报告
#   ./scripts/iwyu/iwyu.sh --only-headers='common/.*'   # 只分析匹配后缀的头
#   ./scripts/iwyu/iwyu.sh --ignore-headers='...'       # 覆盖默认第三方头排除
#
# 环境变量:
#   CLANG_INCLUDE_CLEANER  - 工具路径（默认自动检测）
#   MSVC_ROOT              - MSVC 工具链根目录（默认自动检测最新版本）
#   WIN_SDK_ROOT           - Windows Kits/10 根目录（默认 D:/Windows Kits）
#   WIN_SDK_VERSION        - SDK 版本号（默认自动检测最新版本）
#   IWYU_BUILD_DIR         - compile_commands.json 目录（默认 $PROJECT_ROOT/build）
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${IWYU_BUILD_DIR:-$PROJECT_ROOT/build}"
PILOT_DIR="src/common/core"

# 默认排除的第三方头（单一大正则，匹配"头的后缀"）
# 覆盖 vcpkg / vendored tracy / perfetto / OffsetAllocator / quickjs-ng / FetchContent _deps
IGNORE_HEADERS_DEFAULT="build/vcpkg_installed|third_party/|perfetto|tracy|_deps/|OffsetAllocator|quickjs"

# ---- Colors (disable if not a terminal) ----
if [[ -t 1 ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    CYAN='\033[0;36m'
    NC='\033[0m'
else
    RED='' GREEN='' YELLOW='' CYAN='' NC=''
fi

# ---- Detect clang-include-cleaner ----
CLANG_IC="${CLANG_INCLUDE_CLEANER:-}"
if [[ -z "$CLANG_IC" ]]; then
    if command -v clang-include-cleaner &>/dev/null; then
        CLANG_IC="$(command -v clang-include-cleaner)"
    elif [[ -x "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-include-cleaner.exe" ]]; then
        CLANG_IC="D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang-include-cleaner.exe"
    else
        echo -e "${RED}ERROR: clang-include-cleaner not found. Set CLANG_INCLUDE_CLEANER environment variable.${NC}" >&2
        exit 1
    fi
fi

echo -e "${GREEN}Using clang-include-cleaner: $CLANG_IC${NC}"
"$CLANG_IC" --version | head -1

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

# ---- Detect clang builtin headers ----
# 从 --version 动态提取 LLVM 主版本号，拼 lib/clang/<ver>/include（含 stddef.h/stdint.h 等）
LLVM_VERSION="$("$CLANG_IC" --version | grep -oE 'LLVM version [0-9]+' | grep -oE '[0-9]+$' || true)"
CLANG_LIB_INCLUDE=""
if [[ -n "$LLVM_VERSION" ]]; then
    CLANG_BIN_DIR="$(cd "$(dirname "$CLANG_IC")" && pwd)"
    CANDIDATE="$CLANG_BIN_DIR/../lib/clang/$LLVM_VERSION/include"
    if [[ -d "$CANDIDATE" ]]; then
        CLANG_LIB_INCLUDE="$CANDIDATE"
        echo -e "${GREEN}Clang builtin: $CLANG_LIB_INCLUDE${NC}"
    fi
fi

# ---- Build extra-arg list（只补系统头，项目 -I/vcpkg 已在 compile_commands.json） ----
EXTRA_ARGS=(
    --extra-arg="-DNOMINMAX"
    --extra-arg="-DWIN32_LEAN_AND_MEAN"
    --extra-arg="-D_CRT_SECURE_NO_WARNINGS"
    --extra-arg="-I$MSVC_ROOT/include"
    --extra-arg="-I$SDK_INC/ucrt"
    --extra-arg="-I$SDK_INC/shared"
    --extra-arg="-I$SDK_INC/um"
)
if [[ -n "$CLANG_LIB_INCLUDE" ]]; then
    EXTRA_ARGS+=(--extra-arg="-I$CLANG_LIB_INCLUDE")
fi

# ---- Parse arguments ----
IWYU_ARGS=()
FILES=()
EDIT_MODE=false
PRINT_MODE_SET=false
IGNORE_SET=false

print_usage() {
    cat <<'EOF'
Usage: iwyu.sh [options] <file(s) or dir(s)>...

If no files/dirs specified, scans the pilot directory src/common/core.
Directories are expanded to all .cpp under them.

Options (passed through to clang-include-cleaner):
  --print[=changes]   Print suggestions (changes=symbol-level, empty=final code). Default: --print=changes
  --remove            Allow header removal suggestions
  --insert            Allow header insertion suggestions
  --edit              Apply edits to source files in place (DANGEROUS, requires clean git tree)
  --html=<file>       Write HTML report to <file>
  --only-headers=<r>  Only analyze headers whose suffix matches regex <r>
  --ignore-headers=<r>Override default third-party header exclusion regex

Wrapper options:
  --build-dir=<dir>   Path to compile_commands.json directory (default: $PROJECT_ROOT/build)
  -h, --help          Show this help

Environment:
  CLANG_INCLUDE_CLEANER, MSVC_ROOT, WIN_SDK_ROOT, WIN_SDK_VERSION, IWYU_BUILD_DIR
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --edit)
            EDIT_MODE=true
            IWYU_ARGS+=("--edit")
            ;;
        --remove|--insert)
            IWYU_ARGS+=("$1")
            ;;
        --print|--print=*)
            IWYU_ARGS+=("$1")
            PRINT_MODE_SET=true
            ;;
        --html=*|--only-headers=*)
            IWYU_ARGS+=("$1")
            ;;
        --ignore-headers=*)
            IWYU_ARGS+=("$1")
            IGNORE_SET=true
            ;;
        --build-dir=*)
            BUILD_DIR="${1#*=}"
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        -*)
            IWYU_ARGS+=("$1")
            ;;
        *)
            FILES+=("$1")
            ;;
    esac
    shift
done

# 默认 --print=changes（若用户未指定 print/edit/html 任一输出模式）
if ! $PRINT_MODE_SET && ! $EDIT_MODE; then
    has_html=false
    for a in "${IWYU_ARGS[@]}"; do
        [[ "$a" == --html=* ]] && has_html=true
    done
    if ! $has_html; then
        IWYU_ARGS+=("--print=changes")
    fi
fi

# 默认 --ignore-headers（若用户未覆盖）
if ! $IGNORE_SET; then
    IWYU_ARGS+=("--ignore-headers=$IGNORE_HEADERS_DEFAULT")
fi

# ---- compile_commands.json 存在性检查 ----
if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
    echo -e "${RED}ERROR: $BUILD_DIR/compile_commands.json not found.${NC}" >&2
    echo "Run './scripts/configure.sh build' first (CMakePresets now exports compile_commands.json)." >&2
    exit 1
fi

# ---- 默认扫试点子目录 ----
if [[ ${#FILES[@]} -eq 0 ]]; then
    echo -e "${YELLOW}No files specified. Scanning pilot dir $PILOT_DIR ...${NC}"
    FILES=("$PROJECT_ROOT/$PILOT_DIR")
fi

# ---- 展开目录为 .cpp 列表 ----
EXPANDED=()
for f in "${FILES[@]}"; do
    if [[ -d "$f" ]]; then
        while IFS= read -r cpp; do
            EXPANDED+=("$cpp")
        done < <(find "$f" -name "*.cpp" -type f 2>/dev/null | sort)
    else
        EXPANDED+=("$f")
    fi
done
FILES=("${EXPANDED[@]}")

TOTAL=${#FILES[@]}
if [[ $TOTAL -eq 0 ]]; then
    echo -e "${RED}ERROR: No .cpp source files found.${NC}" >&2
    exit 1
fi

# ---- --edit 安全闸：工作树脏则交互确认 ----
if $EDIT_MODE; then
    if ! git -C "$PROJECT_ROOT" diff --quiet 2>/dev/null; then
        echo -e "${RED}WARNING: Working tree has uncommitted changes.${NC}" >&2
        echo "  --edit will modify source files in place. Continue? [y/N]" >&2
        read -r ans
        if [[ ! "$ans" =~ ^[Yy]$ ]]; then
            echo "Aborted."
            exit 1
        fi
    fi
    echo -e "${RED}EDIT MODE: will modify source files in place.${NC}"
fi

echo -e "${GREEN}Scanning $TOTAL file(s)...${NC}"
echo ""

# ---- 去重 clang-include-cleaner --print=changes 的重复输出 ----
# LLVM 20 的 include-cleaner 对单个翻译单元会输出 N 份完全相同的建议块
# （N 不固定，与文件复杂度相关：试点见过 3 次、12 次等；非多配置导致，是工具固有行为）。
# 用"保留首次出现顺序"的去重：整段重复时每段内行相同，去重后只剩唯一建议行。
dedupe_changes_output() {
    # awk '!seen[$0]++'：首次出现的行输出，重复行跳过，保留原始顺序
    printf '%s\n' "$1" | awk '!seen[$0]++'
}

# 是否需要对 --print=changes 输出去重（仅默认建议模式；--edit/--html/--print 最终代码模式不去重）
SHOULD_DEDUPE=true
if $EDIT_MODE; then
    SHOULD_DEDUPE=false
fi
for a in "${IWYU_ARGS[@]}"; do
    # --print (无 =changes) 是最终代码模式；--html 是 HTML 报告；这两种不去重
    if [[ "$a" == "--print" || "$a" == --html=* ]]; then
        SHOULD_DEDUPE=false
    fi
done

# ---- Run clang-include-cleaner ----
PASS=0   # CLEAN
SUGGEST=0
FAIL=0
SKIP=0
FAILED_FILES=()

for file in "${FILES[@]}"; do
    # 解析为绝对路径
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

    # clang-include-cleaner 调用：-p <build> + extra-args + iwyu-args + 单源文件
    OUTPUT=$("$CLANG_IC" -p "$BUILD_DIR" "${EXTRA_ARGS[@]}" "${IWYU_ARGS[@]}" "$file" 2>&1) && EXIT_CODE=0 || EXIT_CODE=$?

    if $SHOULD_DEDUPE && [[ $EXIT_CODE -eq 0 && -n "$OUTPUT" ]]; then
        OUTPUT=$(dedupe_changes_output "$OUTPUT")
    fi

    if [[ $EXIT_CODE -eq 0 ]]; then
        if [[ -n "$OUTPUT" ]]; then
            echo -e "${YELLOW}SUGGEST: $REL_PATH${NC}"
            echo "$OUTPUT"
            ((SUGGEST++)) || true
        else
            echo -e "${GREEN}  CLEAN: $REL_PATH${NC}"
            ((PASS++)) || true
        fi
    elif [[ $EXIT_CODE -eq 139 || $EXIT_CODE -lt 0 ]]; then
        echo -e "${RED}CRASH: $REL_PATH (exit $EXIT_CODE)${NC}"
        ((SKIP++)) || true
    else
        echo -e "${RED}ERROR: $REL_PATH (exit $EXIT_CODE)${NC}"
        echo "$OUTPUT" | head -20
        ((FAIL++)) || true
        FAILED_FILES+=("$REL_PATH")
    fi
done

# ---- Summary ----
echo ""
echo "=========================================="
echo -e "  Total: $TOTAL  ${GREEN}Clean: $PASS${NC}  ${YELLOW}Suggest: $SUGGEST${NC}  ${RED}Fail: $FAIL${NC}  ${YELLOW}Skip: $SKIP${NC}"
echo "=========================================="

if [[ ${#FAILED_FILES[@]} -gt 0 ]]; then
    echo ""
    echo -e "${RED}Failed files:${NC}"
    for f in "${FAILED_FILES[@]}"; do
        echo "  - $f"
    done
fi

exit $FAIL
