## clang-tidy 静态分析

项目配置了 `.clang-tidy` 文件，启用 `bugprone-*`、`clang-analyzer-*`、`concurrency-*`、`performance-*` 等检查规则。

Windows 上项目使用 clang-cl 编译，`compile_commands.json` 会导致 clang-tidy segfault，脚本会注入 MSVC / Windows SDK / vcpkg 头文件路径。Linux 上则使用 CMake 的 `build/compile_commands.json`，无需手工注入头文件路径。

### 用法

```bash
# Bash (Git Bash)
./scripts/tidy/tidy.sh src/server/world/ServerChunkManager.cpp    # 扫描单个文件
./scripts/tidy/tidy.sh src/common/world/*.cpp                      # 扫描多个文件
./scripts/tidy/tidy.sh                                             # 不指定文件则扫描全部 src/

# PowerShell
.\scripts\tidy\tidy.ps1 src\server\world\ServerChunkManager.cpp
.\scripts\tidy\tidy.ps1 src\server\world                       # 递归扫描 C++ 源文件和头文件
.\scripts\tidy\tidy.ps1 --jobs 8 src\server\world                # 指定并发数
.\scripts\tidy\tidy.ps1                                            # 扫描全部 src/

# Linux（先配置项目以生成 build/compile_commands.json）
./scripts/tidy/tidy.sh --warnings-as-errors='*' src/server/world/Foo.cpp
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

扫描器默认按逻辑处理器数量并发启动 clang-tidy；可通过 `--jobs N` 或 `TIDY_JOBS=N` 调整并发数。扫描目录时递归包含 `.cpp`、`.cc`、`.cxx`、`.hpp` 和 `.h` 文件。完成后会打印总耗时、逐文件运行时间均值及按检查项归类的诊断数量。

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `CLANG_TIDY` | clang-tidy 可执行文件路径 | 自动检测 PATH / VS 内置 |
| `MSVC_ROOT` | MSVC 工具链根目录 | 自动检测最新版本 |
| `WIN_SDK_ROOT` | Windows Kits 根目录 | `D:/Windows Kits` |
| `WIN_SDK_VERSION` | SDK 版本号 | 自动检测最新版本 |
| `TIDY_JOBS` | clang-tidy 最大并发进程数 | 逻辑处理器数量 |
| `TIDY_COMPILE_DB_DIR` | Linux compile database 所在目录 | `build/` |

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
