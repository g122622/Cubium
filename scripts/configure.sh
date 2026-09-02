#!/usr/bin/env bash
# ============================================================
# Cubium - Build wrapper
#
# 跨平台构建入口脚本，行为约定与 configure.bat / configure.ps1 一致：
#   ./configure.sh                          - Configure（默认 preset）
#   ./configure.sh build                    - Configure + Build
#   ./configure.sh --preset <preset-name>   - 透传给 cmake，使用指定 preset
#   ./configure.sh <cmake args...>          - 透传给 cmake
#
# 平台行为：
#   - Linux：直接调用 cmake preset（默认 linux-relwithdebinfo，服务端为主、不含客户端）
#   - macOS：直接调用 cmake preset（默认 macos-relwithdebinfo）
#   - Windows (Git Bash / MSYS2)：通过 cmd.exe 调用 configure.bat，
#     由其注入 Visual Studio 开发环境后再运行 CMake。
#
# 默认 preset 选取（Linux/macOS）：本仓库以服务端为主，客户端暂不维护，
# 故 Linux 默认 preset 为 linux-relwithdebinfo（MC_BUILD_CLIENT=OFF）。
# ============================================================

set -e

# ============================================================
# 平台检测
# ============================================================
detect_os() {
    case "$(uname -s)" in
        Linux*)  echo "linux" ;;
        Darwin*) echo "macos" ;;
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
        *) echo "unknown" ;;
    esac
}

OS="$(detect_os)"

# ============================================================
# 默认 preset：Linux 服务端为主，macOS 保留客户端
# ============================================================
default_preset_for() {
    case "$1" in
        linux) echo "linux-relwithdebinfo" ;;
        macos) echo "macos-relwithdebinfo" ;;
        *) echo "" ;;
    esac
}

# ============================================================
# Windows 分支：委托给 configure.bat（注入 VS 开发环境）
# ============================================================
run_windows() {
    local script_dir bat_path
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    if command -v cygpath >/dev/null 2>&1; then
        bat_path="$(cygpath -w "$script_dir/configure.bat")"
    else
        bat_path="$script_dir/configure.bat"
    fi

    if [ $# -eq 0 ]; then
        exec cmd //c "$bat_path"
    elif [ "$1" = "build" ]; then
        exec cmd //c "$bat_path" build
    else
        exec cmd //c "$bat_path" "$@"
    fi
}

# ============================================================
# Unix 分支（Linux/macOS）：直接走 cmake preset
# ============================================================
run_unix() {
    local preset
    preset="$(default_preset_for "$OS")"
    if [ -z "$preset" ]; then
        echo "ERROR: Unsupported OS for preset-based build: $(uname -s)" >&2
        exit 1
    fi

    # 无参：仅 configure 默认 preset
    if [ $# -eq 0 ]; then
        echo "Configuring with preset ${preset}..."
        cmake --preset "$preset"
        exit $?
    fi

    # "build" 快捷方式：configure + build
    if [ "$1" = "build" ]; then
        echo "Configuring with preset ${preset}..."
        cmake --preset "$preset" || exit $?
        echo ""
        echo "Building..."
        cmake --build --preset "$preset"
        exit $?
    fi

    # "test" 快捷方式：configure + build + ctest
    if [ "$1" = "test" ]; then
        echo "Configuring with preset ${preset}..."
        cmake --preset "$preset" || exit $?
        echo ""
        echo "Building..."
        cmake --build --preset "$preset" || exit $?
        echo ""
        echo "Running tests..."
        ctest --preset "$preset"
        exit $?
    fi

    # --preset <name>：使用指定 preset configure（不构建）
    if [ "$1" = "--preset" ]; then
        if [ -z "$2" ]; then
            echo "ERROR: --preset requires a preset name" >&2
            exit 1
        fi
        echo "Configuring with preset $2..."
        cmake --preset "$2"
        exit $?
    fi

    # 其他参数：透传给 cmake
    cmake "$@"
}

# ============================================================
# 分发
# ============================================================
case "$OS" in
    windows)
        run_windows "$@"
        ;;
    linux|macos)
        run_unix "$@"
        ;;
    *)
        echo "ERROR: Unsupported OS: $(uname -s)" >&2
        exit 1
        ;;
esac
