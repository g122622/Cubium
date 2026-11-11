# 构建指南

## 构建命令

### 环境配置

项目提供了自动注入 Visual Studio 开发环境的构建脚本（位于 `scripts/` 目录），无需手动打开 Developer Command Prompt：

| 脚本 | 适用环境 |
|------|---------|
| `scripts/configure.bat` | CMD / CI 管道 |
| `scripts/configure.sh` | Git Bash |
| `scripts/configure.ps1` | PowerShell |

#### 首次 Configure

```bash
# Git Bash / Claude Code
./scripts/configure.sh

# CMD / CI
scripts\configure.bat

# PowerShell
.\scripts\configure.ps1
```

#### Configure + Build 一步完成

```bash
# Git Bash
./scripts/configure.sh build

# CMD
scripts\configure.bat build

# PowerShell
.\scripts\configure.ps1 -Build
```

#### 增量构建（无需重新 configure）

```bash
cmake --build --preset windows-clang-relwithdebinfo-server
```

#### 构建非默认 preset（如 release-noprof）

`scripts/configure.{sh,bat,ps1}` 的无参模式与 `build` 快捷模式均**硬编码**使用 `windows-clang-relwithdebinfo-server` preset，无法直接切换到其他 preset。构建非默认 preset 时须使用脚本的 pass-through 模式，将 `--preset <name>` 透传给 cmake（前文 `--preset windows-clang-release` 示例即此用法）：

```bash
# Configure（注入 VS 开发环境 + 指定 preset，仅配置不构建）
./scripts/configure.sh --preset <preset-name>

# Build（用同名 build preset 增量构建，并发由核心数和可用内存共同限制）
cmake --build --preset <preset-name>
```

CMakePresets.json 按用途隔离构建目录：日常服务端使用 `build/`，含客户端的开发/发布配置使用 `build-client/`、`build-client-release/`，sanitizer 使用 `build-ci-tests/`，CI Release 使用 `build-ci-release/`，noprof 与 fuzz 也有独立目录。客户端已停止维护，脚本默认使用服务端专用 preset（`MC_BUILD_CLIENT=OFF`），需要客户端时显式传 `--preset windows-clang-relwithdebinfo`：

| preset | buildType | tests | binaryDir | 说明 |
|--------|-----------|-------|-----------|------|
| `windows-clang-relwithdebinfo-server` | RelWithDebInfo | ON | `build` | 日常开发（脚本默认）：Perfetto ON，Tracy/Memory OFF |
| `windows-clang-relwithdebinfo-noprof` | RelWithDebInfo | ON | `build-noprof` | 关闭 Perfetto/Tracy/memory 插桩 |
| `windows-clang-release-noprof` | Release | OFF | `build-release-noprof` | 可分发发布版：关插桩 + 关 Vulkan 校验层 + 不含测试/benchmark |

> **Tracy 与 `MC_ENABLE_MEMORY` 默认 OFF**（`CMakeLists.txt` 的 option 默认值）。原因是 Tracy 带来可观的常驻内存开销——Windows 上其符号解析线程会载入全部模块的调试符号（实测约 96 MB），自带的 rpmalloc 按 4 MiB 为单位 `VirtualAlloc`；且**不受 `--profiler-enabled=false` 影响**（该 flag 只门控 Perfetto 侧）。实测对比见 `docs/MEMORY.md`。需要 Tracy 实时火焰图时显式开启：
>
> ```bash
> cmake --preset windows-clang-relwithdebinfo-server -DMC_ENABLE_TRACY=ON -DMC_ENABLE_MEMORY=ON
> ```
>
> `MC_ENABLE_MEMORY` 依赖 `MC_ENABLE_TRACY`，两者需同时开启。排查期若不想重新构建，可临时设 `TRACY_SYMBOL_OFFLINE_RESOLVE=1` 环境变量剥离开销。

以 `windows-clang-release-noprof` 为例：

```bash
./scripts/configure.sh --preset windows-clang-release-noprof
cmake --build --preset windows-clang-release-noprof
```

构建产物位于 `build-release-noprof/bin/Release/`（如 `minecraft-client.exe`、`minecraft-server.exe`）。

> **注意**：`windows-clang-release-noprof` 与 `windows-clang-release` 的 `MC_BUILD_TESTS=OFF`，不会生成 `mc_tests`；回归测试须切回 `windows-clang-relwithdebinfo-server` preset 构建。

#### 调试信息等级（RelWithDebInfo）

`RelWithDebInfo` 不使用 CMake 默认的 `-O2 -g -DNDEBUG`，而是固定为：

```
-O2 -DNDEBUG -gline-tables-only -gno-pubnames
```

原因是本工程的调试信息规模异常：模板实例化 + 头文件密集（`std::variant` 承载上百种数据包、
codec 层层内联），默认 `-g` 会为每个实例化、每个内联副本生成完整的类型与变量 DIE。实测单个
`.cpp` 产出的 `.o` 达 188 MB，其中 `__debug_str`（类型名字符串表）142 MB，而代码段仅 0.9 MB；
整个 `build/` 曾膨胀到 74 GB。改为行号级调试信息后同一 `.o` 降到 41 MB（-78%），单 TU 编译
时间下降约 25%，崩溃栈仍能给出文件与行号。

需要观察变量值时使用日志输出（项目规范要求日志用英文），不依赖调试器。`-gz=zlib` 在 Mach-O 上
不生效，已实测排除。

#### 注意事项

- 本项目**只支持 `RelWithDebInfo` 与 `Release` 两种配置**，Debug（`-O0`）已从 preset、CI 和本文档中移除：区块生成与网络编解码在 `-O0` 下慢一个数量级，实测不具备可用性。调试需求一律通过日志解决。
- 构建命令除了编译 C++ 代码之外，还会编译着色器。
- 各平台默认并发按逻辑核心数减二与可用内存 / 1536 MiB 取较小值，链接串行执行。需要显式覆盖时 configure 传 `-DMC_COMPILE_JOBS=<正整数>`；不再使用固定的 16/10 并发。
- benchmark 默认关闭；需要时 configure 显式传 `-DMC_BUILD_BENCHMARKS=ON`，再构建 `mc_benchmark`。
- 客户端测试由 `MC_BUILD_CLIENT_TESTS=ON` 单独开启，产物为 `mc_client_tests` 和 `mc_trident_tests`；服务端主单测不编译客户端实现。
- 构建可能出现 "cl: 命令行 error D8040: 创建子进程或与子进程通讯时出错" 这种错误，此时只需要重新跑一遍构建命令就行，不用清理构建目录、不用重新生成构建脚本。

#### PowerShell 升级后链接阶段 "The system cannot find the path specified"

**现象**：链接成功后，POST_BUILD 步骤（复制运行时 DLL）失败，报错：

```
FAILED: bin/RelWithDebInfo/minecraft-client.exe
... pwsh.exe -noprofile -executionpolicy Bypass -file "applocal.ps1" ...
The system cannot find the path specified.
```

**根因**：vcpkg 在 CMake configure 时将 `pwsh.exe` 的绝对路径缓存到 `CMakeCache.txt` 中（`Z_VCPKG_POWERSHELL_PATH` 和 `Z_VCPKG_PWSH_PATH`），且写入 ninja 构建文件的 POST_BUILD 命令。当 PowerShell 通过 Microsoft Store 自动升级后（如 7.6.4.0 → 7.6.5.0），安装目录版本号变化，旧路径失效，但 CMake 缓存中仍是旧路径。

**解决**：先确认当前 PowerShell 实际安装路径（`which pwsh` 或 `where pwsh.exe`），得到新版本号，再替换 `build/CMakeCache.txt` 中的旧版本号，然后重新 configure 让 CMake 自动重生成其余 ninja/json 缓存文件（不必逐个手改、也不会漏）：

```bash
# 示例：PowerShell 从 7.6.4.0 升级到 7.6.5.0
sed -i 's|Microsoft.PowerShell_7.6.4.0_x64__8wekyb3d8bbwe|Microsoft.PowerShell_7.6.5.0_x64__8wekyb3d8bbwe|g' \
  build/CMakeCache.txt
./scripts/configure.sh build   # 重新 configure 会重写 impl-*.ninja 与 cache json，再增量链接
```

> 缓存变量名为 `Z_VCPKG_POWERSHELL_PATH`（INTERNAL）与 `Z_VCPKG_PWSH_PATH`（FILEPATH），均在 `build/CMakeCache.txt` 中。若不想重跑 configure，亦可手动同步替换 `build/CMakeFiles/impl-*.ninja` 与 `build/.cmake/api/v1/reply/cache-v2-*.json` 后直接增量构建，但任一文件漏改都会使 POST_BUILD 仍指向旧路径。本陷阱每次 PowerShell 小版本升级都会复现，属 vcpkg 缓存绝对路径的固有问题。

#### vcpkg 构建失败恢复

如果遇到 vcpkg 构建失败，手动执行 CMake configure，**关闭 vcpkg manifest install**：

```powershell
cmake .. -DVCPKG_MANIFEST_INSTALL=OFF -G "Ninja Multi-Config"
```

这跳过了 vcpkg install 步骤，CMake 成功完成配置并重新生成了 ninja 构建文件。之后正常构建即可：

```powershell
cmake --build --preset windows-clang-relwithdebinfo-server
```

### 运行

```bash
# 运行测试
# 强烈建议只运行特定测试并设置 brief，运行全部测试会更慢（测试用例有几千个）
# 建议只在全部编码工作完成之后运行回归测试的时候才运行全部测试，且也要启用 brief
./build/bin/RelWithDebInfo/mc_tests --gtest_filter=UniversalWorkerPoolTest.* --gtest_brief=1

# 运行服务端
./build/bin/RelWithDebInfo/minecraft-server --help

# 运行客户端
./build/bin/RelWithDebInfo/minecraft-client

# 运行 benchmark
./build/bin/RelWithDebInfo/mc_benchmark
```

增加新的着色器之后要在 `shaders/CMakeLists.txt` 中新增文件

## macOS 构建

项目支持在 macOS (Apple Silicon) 上构建。使用 `macos-relwithdebinfo-server` preset：

```bash
# 安装依赖
brew install cmake ninja pkg-config glslang molten-vk lld

# 克隆并配置 vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

# 配置环境变量
export VCPKG_ROOT=~/vcpkg

# 配置
cmake --preset macos-relwithdebinfo-server

# 构建（-j10 使用16核心并行）
cmake --build --preset macos-relwithdebinfo-server -- -j16

# 运行
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
  build/bin/RelWithDebInfo/minecraft-client
```

### macOS 已知问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `-mavx2` 编译错误 | AVX2 是 x86 专用指令集，arm64 不支持 | CMakeLists.txt 已添加 `CMAKE_SYSTEM_PROCESSOR` 判断，仅在 x86_64 下启用 `-mavx2` |
| `BYTE_SIZE` 宏冲突 | macOS `<mach/arm/vm_param.h>` 定义 `#define BYTE_SIZE 8`，与 `NibbleArray::BYTE_SIZE` 冲突 | 在受影响的文件中使用 `#pragma push_macro`/`#undef` 屏蔽系统宏，并在所有 include 之后再次 `#undef` |
| `TRUE`/`FALSE` 宏冲突 | macOS `<mach/boolean.h>` 定义 `#define TRUE 1` / `#define FALSE 0`，与 `BooleanOp::TRUE()`/`FALSE()` 冲突 | 在 `BooleanOp.hpp` 中使用 `#pragma push_macro`/`#undef`/`#pragma pop_macro` 屏蔽 |
| `resident_size_max` 不存在 | macOS `task_vm_info` 结构体中字段名为 `resident_size_peak` | 使用 `resident_size_peak` 替代 `resident_size_max` |
| `noexcept` override 不匹配 | Apple Clang 21 严格要求 override 函数的异常规范与基类一致 | 在 override 方法声明中添加 `noexcept` |
| `ld64.lld` 缺失 | Homebrew 自 LLVM 19 起把 lld 拆成独立 formula，`brew install llvm` 不再附带 | 执行 `brew install lld`；CMakeLists.txt 统一为全平台使用 lld，找不到时在配置阶段直接失败 |
| `-Wl,-gc-sections` 不支持 | macOS 的 ld64 不支持 GNU 风格链接器选项 | CMakeLists.txt 已添加平台判断，仅在 Linux 下使用 |

> **注意**：macOS 构建需要着色器编译器。安装 `glslang`（提供 `glslangValidator`）或 Vulkan SDK（提供 `glslc`）。

## Linux 构建

项目支持在 Linux（x86_64）上构建服务端。使用 `linux-relwithdebinfo` preset：

```bash
# 安装依赖（Ubuntu/Debian 示例）
sudo apt-get install clang clang++ cmake ninja-build ccache pkg-config git

# 配置 vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# 配置（默认 linux-relwithdebinfo，MC_BUILD_CLIENT=OFF）
cmake --preset linux-relwithdebinfo

# 构建
cmake --build --preset linux-relwithdebinfo

# 运行服务端
./build/bin/RelWithDebInfo/minecraft-server --help

# 运行测试
./build/bin/RelWithDebInfo/mc_tests --gtest_filter="*Math*" --gtest_brief=1
```

### 编译缓存（Windows / Linux / macOS）

`MC_ENABLE_CCACHE=ON` 时通过 ccache 包装 C/C++ 编译。`cmake/CompilerCache.cmake` 自动传入
PCH 所需的 sloppiness 与基于编译器内容的身份检查；缓存目录、容量、远端存储和
`CCACHE_PREFIX` 继续使用 ccache 的正常配置。找不到可执行文件时 configure 会明确警告。

Windows 可从 [ccache 官方发布页](https://github.com/ccache/ccache/releases) 下载 x86_64 压缩包，
将 `ccache.exe` 放入 PATH 或 `%LOCALAPPDATA%/Cubium/tools/ccache/`；本机已使用 4.14.3。
Linux 安装 `ccache` 包，macOS 使用 `brew install ccache`。

用 `ccache --show-stats --verbose` 查看命中与不可缓存原因。更换编译器、优化参数或调试信息等级
会产生正常的冷缓存；先验证同配置下的暖缓存，再调整容量和并发。

### Sanitizer 与依赖功能

`linux-clang-ci-tests` 使用 `build-ci-tests/`，全局 C/C++ 及链接插桩由
`cmake/Sanitizers.cmake` 管理，关闭 `MC_ENABLE_SANITIZERS` 即移除这些使用要求。
不要将插桩参数额外写进通用 `CMAKE_*_FLAGS`。预设显式清空通用 flags，迁移旧构建缓存时
也不会残留历史 sanitizer 参数；自定义 flags 应在 configure 命令中显式传入。

vcpkg 的 `tests`、`benchmarks`、`client`、`server` 功能由 CMake 根据目标开关选择。
关闭客户端和客户端测试后不安装 GLFW/Vulkan/OpenAL；关闭测试或 benchmark 后不安装对应框架。

测试预设均指定 `RelWithDebInfo`，可使用 `ctest --preset <同名开发预设> -R '<测试名>'`。
`configure.sh test` 也接受后续的 CTest 筛选参数。

### 异常开关

`MC_ENABLE_NO_EXCEPTIONS` 默认 ON：`mc_common` / `mc_gen` / `mc_server_shared` /
`mc_server_storage` / `mc_bedrock_addon` / `minecraft-server` 以 `-fno-exceptions` 编译，
`src/` 下出现 `throw` / `try` / `catch` 会直接编译失败。`mc_tests` 与 `mc_benchmark`
故意保留异常。`-DMC_ENABLE_NO_EXCEPTIONS=OFF` 可临时回到带异常构建；
`-DMC_NO_EXCEPTIONS_TARGETS=mc_common` 可只对子集生效。PCH 的异常模式必须与使用方一致，
所以测试目标在开关开启时不再 `REUSE_FROM mc_common`，由 `mc_configure_unit_target()` 统一处理。

### Linux preset 一览

CMakePresets.json 中预置的 Linux preset（均设 `MC_BUILD_CLIENT=OFF`，服务端为主）：

| preset | buildType | tests | binaryDir | 说明 |
|--------|-----------|-------|-----------|------|
| `linux-relwithdebinfo` | RelWithDebInfo | ON | `build` | 日常开发（脚本默认） |
| `linux-release-noprof` | Release | OFF | `build-release-noprof` | 可分发发布版：关插桩 + 不含测试/benchmark |

> **注意**：日常构建、CI sanitizer、CI Release 分别使用 `build/`、`build-ci-tests/`、`build-ci-release/`，可并存。

### Linux 已知问题及解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `'__msvc_chrono.hpp' file not found` | MSVC STL 内部头文件，libstdc++ 不需要 | `SpecialDates.hpp` 已用 `#ifdef _MSC_VER` 守卫，仅 MSVC 下 include |
| `CHAR_WIDTH` 宏冲突 | glibc `<limits.h>` 定义 `#define CHAR_WIDTH 8`（C23 字符位宽宏） | 已将 `CHAR_WIDTH` 常量重命名为 `DEFAULT_CHAR_WIDTH` |
| 头文件缺 `<memory>` 等 include | 依赖传递性 include，clang/Linux 下不传递 | 已逐个补齐显式 include |

## 着色器编译

项目使用 Vulkan SPIR-V 着色器。CMake 构建会自动编译着色器，无需手动操作。

如果需要手动编译着色器，确保已安装 [Vulkan SDK](https://vulkan.lunarg.com/) 并使用 `glslc`：

```powershell
glslc shaders/block.vert -o build/shaders/block.vert.spv
glslc shaders/block.frag -o build/shaders/block.frag.spv
```

### VS Code IntelliSense 配置（Windows / cpptools）

项目使用 clang-cl 编译，系统头文件路径（MSVC、Windows SDK）由 `VsDevCmd.bat` 通过 `INCLUDE` 环境变量隐式注入，不会写入 `compile_commands.json`。这会导致 VS Code cpptools 扩展无法找到 C++ 标准库头文件（如 `<string>`、`<cmath>`、`<unordered_set>`），出现红色波浪线报错。

#### 根因

1. `compile_commands.json` 中编译器路径为 8.3 短名格式（`PROGRA~2`）且指向 `clang.exe`（C 编译器），而非 `clang++.exe`
2. 系统头文件路径未记录在 `compile_commands.json` 中，cpptools 无法自动发现
3. 全局 VS Code 设置中 `C_Cpp.default.compilerPath` 可能指向错误或不存在的编译器路径

#### 解决方案

项目已在 `.vscode/c_cpp_properties.json` 和 `.vscode/settings.json` 中预配置了正确的路径。如果你的环境不同，需检查以下两项：

**1. `.vscode/c_cpp_properties.json`** — 确保 `includePath` 包含所有系统头文件目录，且 `compilerPath` 指向 `clang++.exe`：

```json
{
    "configurations": [{
        "name": "Win32",
        "includePath": [
            "${workspaceFolder}/include",
            "${workspaceFolder}/src",
            "${workspaceFolder}/src/common",
            "${workspaceFolder}/build/vcpkg_installed/x64-windows/include",
            "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/include",
            "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231/ATLMFC/include",
            "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/VS/include",
            "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/lib/clang/20/include",
            "D:/Windows Kits/10/Include/10.0.26100.0/ucrt",
            "D:/Windows Kits/10/Include/10.0.26100.0/um",
            "D:/Windows Kits/10/Include/10.0.26100.0/shared",
            "D:/Windows Kits/10/Include/10.0.26100.0/winrt",
            "D:/Windows Kits/10/Include/10.0.26100.0/cppwinrt"
        ],
        "compilerPath": "D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/clang++.exe",
        "cStandard": "c17",
        "cppStandard": "c++20",
        "intelliSenseMode": "windows-clang-x64"
    }],
    "version": 4
}
```

> **注意**：不要同时设置 `compileCommands`，否则 cpptools 会优先使用 `compile_commands.json` 中不完整的路径信息，覆盖 `includePath` 的效果。

**2. 全局 VS Code 设置** — 检查 `C_Cpp.default.compilerPath` 是否指向正确路径（`Ctrl+,` 搜索 `compilerPath`），确保不是指向不存在的 VS 2022 或旧版 MSVC 路径。

修改后执行 `Ctrl+Shift+P` → **C/C++: Reset IntelliSense Database** 重建索引。

### clangd 配置（macOS / Linux）

clangd 不读 `.vscode/c_cpp_properties.json`，它依赖 CMake 生成的 `compile_commands.json`。仓库已备好两项前提：

1. **生成编译数据库**：`CMAKE_EXPORT_COMPILE_COMMANDS=ON` 定义在公共 `base` 预设中，所有平台的
   `cmake --preset <name>` 都会在 `build/compile_commands.json` 生成数据库。
2. **让 clangd 找到它**：仓库根的 `.clangd` 指定 `CompilationDatabase: build`。clangd 默认只沿源文件
   的祖先目录向上查找该文件，而它位于 `build/` 而非仓库根，不显式指定就会找不到。

**缺少任一项的症状**：clangd 拿不到 `-std=c++20`、include 路径与 vcpkg 的 `-isystem`，回退到猜测模式，
对整份文件报出成片的假错误（`未定义标识符`、`Result 不是模板`、`i32 不是类型名`、`应输入";"` 等）。
这类级联报错的根因通常只是文件开头的第一个未解析类型，**不要逐个去改代码**——先确认数据库是否就位。

验证：

```bash
cmake --preset macos-relwithdebinfo-server     # 重新 configure 才会生成/刷新数据库
ls build/compile_commands.json          # 应存在
python3 -c "import json; print(len(json.load(open('build/compile_commands.json'))))"
```

改动 `CMakePresets.json` 的 cacheVariables 或切换构建目录后，需重新 configure 才能刷新数据库。
`build/` 已被 `.gitignore` 忽略，该文件不入版本控制。

## 本地 Sanitizer 构建

nightly 使用两个独立的 Linux preset：`linux-clang-ci-tests` 保留 RelWithDebInfo 并对 C/C++ 全局启用
ASan/UBSan，产出服务端与单测；`linux-clang-ci-release` 在独立目录产出无调试信息、无 profiler、
无 sanitizer 的 Release 服务端与 benchmark。两者均关闭 native-arch，原开发和 fuzz preset 保持原有配置。
CI 发布前还检查 ELF，并移除静态依赖库可能带入的调试节；仅设置 Release 不足以保证最终包没有调试信息。
项目保留 `-Wno-pass-failed`，避免显式向量化请求被编译器启发式拒绝时升级为错误；性能差异由 benchmark 检查。

```bash
# ASan + UBSan：使用独立目录，开关自动添加 C/C++ 与链接插桩
cmake --preset linux-clang-ci-tests
cmake --build --preset linux-clang-ci-tests
ctest --preset linux-clang-ci-tests -R '<相关测试名>'
```

`MC_ENABLE_SANITIZERS` 专用于 ASan/UBSan，不要再叠加另一种 sanitizer 到这个预设。
