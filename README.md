# Minecraft Reborn

现代 Minecraft 克隆，使用 C++20 和 Vulkan 渲染，采用客户端-服务端架构。

一个idea：多world并行

## 项目总览

### 目录结构

```text
.
├── README.md
├── CMakeLists.txt
├── scripts/                # 构建 & 工具脚本
│   ├── configure.bat       # Windows 构建环境配置 (CMD)
│   ├── configure.sh        # Windows 构建环境配置 (Git Bash)
│   ├── configure.ps1       # Windows 构建环境配置 (PowerShell)
│   └── vsenv.bat           # VS 开发环境注入
├── shaders/                # Vulkan 着色器
├── resources/              # 原版资源与数据文件
├── src/
│   ├── client/             # 客户端
│   ├── common/             # 客户端/服务端共享代码
│   └── server/             # 服务端
└── tests/                  # 测试（不放 README）
```

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
- 构建可能出现 “cl: 命令行 error D8040: 创建子进程或与子进程通讯时出错” 这种错误，此时只需要重新跑一遍构建命令就行，不用清理构建目录、不用重新生成构建脚本。

#### vcpkg 构建失败恢复

如果遇到 vcpkg 构建失败，手动执行 CMake configure，**关闭 vcpkg manifest install**：

```powershell
cmake .. -DVCPKG_MANIFEST_INSTALL=OFF -G “Ninja Multi-Config”
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
./build/bin/RelWithDebInfo/mc_tests --gtest_filter=ServerWorkerPoolTest.* --gtest_brief=1

# 运行服务端
./build/bin/RelWithDebInfo/minecraft-server --help

# 运行客户端
./build/bin/RelWithDebInfo/minecraft-client

# 运行 benchmark
./build/bin/RelWithDebInfo/mc_benchmarks
```

增加新的着色器之后要在 `shaders/CMakeLists.txt` 中新增文件

## 着色器编译

项目使用 Vulkan SPIR-V 着色器。CMake 构建会自动编译着色器，无需手动操作。

如果需要手动编译着色器，确保已安装 [Vulkan SDK](https://vulkan.lunarg.com/) 并使用 `glslc`：

```powershell
glslc shaders/block.vert -o build/shaders/block.vert.spv
glslc shaders/block.frag -o build/shaders/block.frag.spv
```

## 依赖

见vcpkg.json

## CI / GitHub Actions

项目配置了 GitHub Actions 持续集成，位于 `.github/workflows/ci.yml`，包含以下 Job：

| Job | 平台 | 说明 |
|-----|------|------|
| **build-windows** | Windows / Clang | 客户端+服务端 RelWithDebInfo 构建（快速反馈） |
| **build-linux** | Linux / Clang | 服务端 RelWithDebInfo 构建并运行测试 |
| **asan-ubsan** | Linux / Clang | AddressSanitizer + UndefinedBehaviorSanitizer 测试 |
| **tsan** | Linux / Clang | ThreadSanitizer 测试 |
| **stack-protect** | Linux / Clang | 栈保护 + 硬化构建（`-fstack-protector-strong`、`-D_FORTIFY_SOURCE=2`、PIE、RELRO、不可执行栈），并验证二进制安全属性 |
| **format-check** | Linux | clang-format 格式检查（仅检查变更文件） |

### 关键设计

- **ASan+UBSan 与 TSan 分离**：两种 sanitizer 互斥，必须独立运行
- **Sanitizer 构建使用 Debug 模式**：`MC_ENABLE_SANITIZERS=ON` 时自动切换到 `-O1` 并禁用 `-march=native`、LTO、`-fno-stack-protector` 等优化选项
- **Linux Job 关闭客户端**：CI 无 GPU/Vulkan，所有 Linux Job 使用 `MC_BUILD_CLIENT=OFF`
- **vcpkg 缓存**：使用 `lukka/run-vcpkg@v11` 并锁定 baseline commit
- **并发控制**：同一分支/PR 的重复运行会自动取消

### 本地 Sanitizer 构建

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
