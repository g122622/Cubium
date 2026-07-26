# Cubium

![](./docs/logo.png)

![Lines of Code](https://raw.githubusercontent.com/g122622/Cubium/image-data/badge.svg)

![](./docs/screenshot1.png)
![](./docs/screenshot2.png)

现代 Minecraft 第三方完整实现，使用 C++20 和 Vulkan 渲染，采用客户端-服务端架构。

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
│   ├── tidy/               # clang-tidy 静态分析
│   │   ├── tidy.sh         #   Bash 版本
│   │   └── tidy.ps1        #   PowerShell 版本
│   └── vsenv.bat           # VS 开发环境注入
├── shaders/                # Vulkan 着色器
├── resources/              # 原版资源与数据文件
├── src/
│   ├── client/             # 客户端
│   ├── common/             # 客户端/服务端共享代码
│   └── server/             # 服务端
└── tests/                  # 测试（不放 README）
```

详细的构建指南见 [docs/BUILD.md](docs/BUILD.md)，包含环境配置、构建命令、运行方式、着色器编译、VS Code IntelliSense 配置、本地 Sanitizer 构建等。

## 运行客户端

### Windows / Linux

直接运行构建产物：

```bash
./build/bin/RelWithDebInfo/minecraft-client
```

### macOS

macOS 上 Vulkan 加载器（vcpkg 打包的 `libvulkan`）默认不会发现 Homebrew 安装的 MoltenVK 驱动，必须通过 `VK_ICD_FILENAMES` 指向 ICD 清单，否则 `vkCreateInstance` 会返回 `-9`（`VK_ERROR_INCOMPATIBLE_DRIVER`）。启动命令：

```bash
# 方式 A：作为命令前缀（仅对本次启动生效）
VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json build/bin/RelWithDebInfo/minecraft-client

# 方式 B：先 export 再运行（当前 shell 后续都生效）
export VK_ICD_FILENAMES=/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json
build/bin/RelWithDebInfo/minecraft-client
```

> 注意：`VAR=value;` 单独一行（带分号、另起一行再运行二进制）只会设置 shell 变量而**不会**导出给子进程，进程仍然读不到该变量，会再次报 `-9`。请用上面两种写法之一。
>
> MoltenVK 经 Homebrew 安装时，ICD 清单路径为 `/opt/homebrew/etc/vulkan/icd.d/MoltenVK_icd.json`；若改用 Vulkan SDK 安装，路径见 SDK 目录下的 `share/vulkan/icd.d/`。

## 测试

项目测试基于 GoogleTest，通过 CTest 编排运行，支持单用例限时（默认 300 秒）、并行、按名筛选。完整指南见 [docs/TEST.md](docs/TEST.md)。

```bash
cd build
# Windows（多配置生成器，须带 --build-config）
ctest --build-config RelWithDebInfo --output-on-failure -j8
# 按用例名筛选
ctest --build-config RelWithDebInfo -R 'ServerChunkManagerTest' --output-on-failure
```

测试 target 共 5 个（`mc_tests` / `mc_resource_tests` / `mc_trident_tests` / `mc_command_tests` / `mc_village_tests`），均注册到 CTest。单用例超时由 `MC_TEST_TIMEOUT`（`tests/CMakeLists.txt`）控制，改值需重新 configure。

## 测试崩溃时查看完整调用栈

`mc_tests` 在 `main` 中安装了 `mc::assert::CrashHandler`（参考 `src/client/main.cpp`、`src/server/main.cpp`），崩溃（SEH 访问违例、除零、栈溢出、纯虚调用、`std::terminate`、`MC_ASSERT_RELEASE` 触发的 `abort`）时输出调用栈和寄存器到 stderr。

但 GoogleTest 默认安装自己的 SEH 处理器（`--gtest_catch_exceptions=1`），会抢先捕获 SEH 并打印 `unknown file: error: SEH exception with code 0x... thrown in the test body.` **不带栈**，CrashHandler 拿不到崩溃现场。要看到完整栈，必须禁用 gtest 的 SEH 捕获，让 CrashHandler 接管：

```bash
# 禁用 gtest SEH 捕获，崩溃时由 CrashHandler 输出完整调用栈
./build/bin/RelWithDebInfo/mc_tests --gtest_filter='ServerChunkManagerTest.GetChunkSync_MultipleChunks' --gtest_catch_exceptions=0
```

- `--gtest_catch_exceptions=0`：关闭 gtest 的 SEH 捕获，SEH 直接传给 CrashHandler。
- 崩溃时输出 `Reason: ACCESS_VIOLATION - Read/Write access at address 0x...`、寄存器转储、`Stack trace:`（帧序号 + 函数名 + 文件:行号）。
- 仅 Windows 需要 `--gtest_catch_exceptions=0`（Linux/macOS 用信号处理，gtest 默认不捕获 SIGSEGV）。
- 配合 `--gtest_filter` 定位到单个用例，栈最干净。`--gtest_break_on_failure` 会让 gtest 抛 `BREAKPOINT`（被 CrashHandler 当作断点，不是原始崩溃点），调试时不要混用。

## clang-tidy 静态分析

项目配置了 `.clang-tidy` 文件，启用 `bugprone-*`、`clang-analyzer-*`、`concurrency-*`、`performance-*` 等检查规则。

由于项目使用 clang-cl 编译，`compile_commands.json` 会导致 clang-tidy segfault，因此需要使用项目提供的脚本，自动注入 MSVC / Windows SDK / vcpkg 头文件路径。

### 用法

```bash
# Bash (Git Bash)
./scripts/tidy/tidy.sh src/server/world/ServerChunkManager.cpp    # 扫描单个文件
./scripts/tidy/tidy.sh src/common/world/*.cpp                      # 扫描多个文件
./scripts/tidy/tidy.sh                                             # 不指定文件则扫描全部 src/

# PowerShell
.\scripts\tidy\tidy.ps1 src\server\world\ServerChunkManager.cpp
.\scripts\tidy\tidy.ps1                                            # 扫描全部 src/
```

### 常用选项

```bash
# 自动修复（谨慎使用，先不加 --fix 确认告警）
./scripts/tidy/tidy.sh --fix src/server/world/Foo.cpp

# 只跑特定检查
./scripts/tidy/tidy.sh --checks 'bugprone-use-after-move' src/common/core/Result.hpp

# 覆盖告警等级（不视为错误）
./scripts/tidy/tidy.sh --warnings-as-errors='' src/server/world/Foo.cpp
```

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `CLANG_TIDY` | clang-tidy 可执行文件路径 | 自动检测 PATH / VS 内置 |
| `MSVC_ROOT` | MSVC 工具链根目录 | 自动检测最新版本 |
| `WIN_SDK_ROOT` | Windows Kits 根目录 | `D:/Windows Kits` |
| `WIN_SDK_VERSION` | SDK 版本号 | 自动检测最新版本 |

### 输出示例

```
Using clang-tidy: /d/Program Files/.../clang-tidy
MSVC: D:/Program Files/.../MSVC/14.51.36231
Windows SDK: D:/Windows Kits/10/Include/10.0.26100.0
Scanning 1 file(s)...

FAIL: src/server/world/ServerChunkManager.cpp
src/server/world/ServerChunkManager.cpp:47:70: error: the parameter 'waiters'
  is copied for each invocation but only used as a const reference
  [performance-unnecessary-value-param,-warnings-as-errors]

==========================================
  Total: 1  Pass: 0  Fail: 1  Skip: 0
==========================================
```

## 依赖

见vcpkg.json

## CI / GitHub Actions

项目配置了 GitHub Actions 持续集成，包含主CI工作流和自愈工作流。

### 主CI工作流 (`ci.yml`)

位于 `.github/workflows/ci.yml`，包含以下 Job：

| Job | 平台 | 说明 |
|-----|------|------|
| **build-windows** | Windows / Clang | 客户端+服务端 RelWithDebInfo 构建（快速反馈） |
| **build-linux** | Linux / Clang | 服务端 RelWithDebInfo 构建并运行测试 |
| **asan-ubsan** | Linux / Clang | AddressSanitizer + UndefinedBehaviorSanitizer 测试 |
| **tsan** | Linux / Clang | ThreadSanitizer 测试 |
| **stack-protect** | Linux / Clang | 栈保护 + 硬化构建（`-fstack-protector-strong`、`-D_FORTIFY_SOURCE=2`、PIE、RELRO、不可执行栈），并验证二进制安全属性 |
| **format-check** | Linux | clang-format 格式检查（仅检查变更文件） |

#### 关键设计

- **ASan+UBSan 与 TSan 分离**：两种 sanitizer 互斥，必须独立运行
- **Sanitizer 构建使用 Debug 模式**：`MC_ENABLE_SANITIZERS=ON` 时自动切换到 `-O1` 并禁用 `-march=native`、LTO、`-fno-stack-protector` 等优化选项
- **Linux Job 关闭客户端**：CI 无 GPU/Vulkan，所有 Linux Job 使用 `MC_BUILD_CLIENT=OFF`
- **vcpkg 缓存**：使用 `lukka/run-vcpkg@v11` 并锁定 baseline commit
- **并发控制**：同一分支/PR 的重复运行会自动取消
- **失败诊断输出**：每个 Job 在失败时输出结构化摘要信息，供自愈工作流分析

### 自愈CI工作流 (`self-heal.yml`)

位于 `.github/workflows/self-heal.yml`，当主CI工作流失败时自动触发，实现检测→诊断→修复的闭环：

#### 工作流程

```
CI失败 → 自愈工作流触发 → AI分析日志 → 分类
                                            ├─ 瞬时故障 → 自动重跑失败Jobs
                                            └─ 非瞬时故障 → 创建Issue分配给@copilot
                                                             → Copilot Agent自主修复
                                                             → 创建PR等待人工审查
```

#### 故障分类

| 类别 | 说明 | 处理方式 |
|------|------|----------|
| **transient** | 网络超时、vcpkg缓存损坏、runner临时问题 | 自动重跑失败的Jobs |
| **formatting** | clang-format格式违规 | 创建Issue，Copilot运行clang-format修复 |
| **build-error** | 编译错误（缺少include、类型不匹配、链接错误） | 创建Issue，Copilot修复代码 |
| **test-failure** | 单元测试断言失败 | 创建Issue，Copilot修复逻辑或测试 |
| **sanitizer** | ASan/UBSan/TSan违规（内存越界、UAF、数据竞争） | 创建Issue，Copilot修复内存/线程问题 |
| **infrastructure** | runner故障、磁盘空间不足、工具链问题 | 创建Issue，标记需人工干预 |

#### 安全机制

- **防循环**：同一分支最近20次提交中bot提交>=5则停止创建新Issue
- **去重检查**：同一CI run不创建重复的修复Issue
- **人工审查**：所有Copilot创建的PR必须经过人工review，不自动合并
- **最小权限**：工作流仅有`contents:read`、`actions:read`、`issues:write`、`models:read`权限

#### 启用自愈CI的前置条件

1. **创建 PAT**：在 GitHub Settings → Developer settings → Fine-grained tokens 创建 `auto-remediation` token，权限：Issues(Read/Write)、Actions(Read)、Models(Read)、Contents(Read)
2. **添加 Secret**：在仓库 Settings → Secrets → Actions 中添加 `AUTO_REMEDIATION_PAT`
3. **启用 GitHub Models**：仓库 Settings → Copilot/Models，确保 GitHub Models 已启用
4. **启用 Copilot Coding Agent**：仓库 Settings → Copilot，确保 Coding Agent 已启用
5. **分支保护**：`main` 分支要求 PR review + status checks
