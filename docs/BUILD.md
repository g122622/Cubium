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
cmake --build --preset windows-clang-relwithdebinfo
```

#### 注意事项

- 即使在开发过程中，也要尽量使用 relwithdebinfo 构建，因为 Debug 运行非常慢，除非必要否则不要用。
- 构建命令除了编译 C++ 代码之外，还会编译着色器。
- 对于 macOS 等系统，默认只会启动一个核心构建，建议加上 `-j6`，并耐心等待 10 分钟左右以完成构建。
- Windows 不需要加 `-j` 后缀，系统会自动吃满全部核心。
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
cmake --build --preset windows-clang-relwithdebinfo
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
./build/bin/RelWithDebInfo/mc_benchmarks
```

增加新的着色器之后要在 `shaders/CMakeLists.txt` 中新增文件

## macOS 构建

项目支持在 macOS (Apple Silicon) 上构建。使用 `macos-relwithdebinfo` preset：

```bash
# 安装依赖
brew install cmake ninja pkg-config glslang molten-vk

# 克隆并配置 vcpkg
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh

# 配置环境变量
export VCPKG_ROOT=~/vcpkg

# 配置
cmake --preset macos-relwithdebinfo

# 构建（-j10 使用10核心并行）
cmake --build --preset macos-relwithdebinfo -- -j10

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
| `-fuse-ld=lld` 不可用 | macOS 没有 lld 链接器 | CMakeLists.txt 已添加平台判断，仅在 Windows/Linux 下使用 lld |
| `-Wl,-gc-sections` 不支持 | macOS 的 ld64 不支持 GNU 风格链接器选项 | CMakeLists.txt 已添加平台判断，仅在 Windows/Linux 下使用 |

> **注意**：macOS 构建需要着色器编译器。安装 `glslang`（提供 `glslangValidator`）或 Vulkan SDK（提供 `glslc`）。

## 着色器编译

项目使用 Vulkan SPIR-V 着色器。CMake 构建会自动编译着色器，无需手动操作。

如果需要手动编译着色器，确保已安装 [Vulkan SDK](https://vulkan.lunarg.com/) 并使用 `glslc`：

```powershell
glslc shaders/block.vert -o build/shaders/block.vert.spv
glslc shaders/block.frag -o build/shaders/block.frag.spv
```

### VS Code IntelliSense 配置

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

## 本地 Sanitizer 构建

```bash
# ASan + UBSan
cmake -B build-sanitize -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMC_BUILD_CLIENT=OFF -DMC_BUILD_SERVER=ON -DMC_BUILD_TESTS=ON \
  -DMC_ENABLE_SANITIZERS=ON \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all" \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"

# TSan
cmake -B build-tsan -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMC_BUILD_CLIENT=OFF -DMC_BUILD_SERVER=ON -DMC_BUILD_TESTS=ON \
  -DMC_ENABLE_SANITIZERS=ON \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-sanitize-recover=thread" \
  -DCMAKE_C_FLAGS="-fsanitize=thread -fno-sanitize-recover=thread" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread"
```
