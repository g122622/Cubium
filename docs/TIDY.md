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

# 只对指定行区间报错（CI 的 PR 门禁用；TSV 由 scripts/ci/changed_lines.py 生成）
python3 scripts/ci/changed_lines.py --base origin/main --head HEAD --output ranges.tsv
./scripts/tidy/tidy.sh --changed-lines=ranges.tsv --warnings-as-errors='*' src/server/world/Foo.cpp
```

扫描器默认按逻辑处理器数量并发启动 clang-tidy；可通过 `--jobs N` 或 `TIDY_JOBS=N` 调整并发数。扫描目录时递归包含 `.cpp`、`.cc`、`.cxx`、`.hpp` 和 `.h` 文件。完成后会打印总耗时、逐文件运行时间均值及按检查项归类的诊断数量。

### 变更行过滤（`--changed-lines`）

PR 门禁只关心本次改动引入的问题，而不是顺手碰到的文件里积压的历史告警——项目里
`Block::rotate` / `Block::mirror` 这类返回 `const BlockState&` 参数的接口，以及
`StateHolder::get<T>()` 的 `noexcept` 逃逸，都会让被重命名触及的整个文件报出大量存量诊断。
`--changed-lines <tsv>` 把失败判定收窄到本次 diff 的新增行：落在区间外的诊断不影响退出码。

TSV 每行是 `<仓库相对路径>\t<起始行>\t<结束行>`，由 `scripts/ci/changed_lines.py` 从
`git diff -U0` 的 hunk 头解析而来（只统计 `--diff-filter=ACMR` 的文件，所以被删除的文件
不会贡献区间）。

两点实现说明：

- **不使用 clang-tidy 自带的 `--line-filter`**。该选项的 `name` 字段只与诊断的**裸文件名**
  做字符串比较（实测 `sub/deep/t2.cpp`、绝对路径、反斜杠形式均不匹配），而本仓库有 30 组
  重名文件，按文件名匹配会把它们的行区间混在一起。因此脚本自己按「仓库相对路径 + 行号」
  过滤，跨平台行为一致。
- **段错误（exit 139）不受该过滤影响**，始终上报——崩溃没有可用的诊断行号。
- **非零退出且一条诊断都没有时按失败处理**。clang-tidy 调用失败（编译数据库缺条目、参数错误、
  输出无法解析）时既没有诊断行号、也没有「区间外」的证据，若此时判为通过，门禁就会在**根本没检查
  该文件**的情况下变绿。因此只有「确实产出了诊断、且全部落在区间外」才视为区间外通过。

未传 `--changed-lines` 时行为与以前一致：任何诊断都计入失败，适合本地全量扫描。

### 环境变量

| 变量 | 说明 | 默认值 |
|------|------|--------|
| `CLANG_TIDY` | clang-tidy 可执行文件路径 | 自动检测 PATH / VS 内置 |
| `MSVC_ROOT` | MSVC 工具链根目录 | 自动检测最新版本 |
| `WIN_SDK_ROOT` | Windows Kits 根目录 | `D:/Windows Kits` |
| `WIN_SDK_VERSION` | SDK 版本号 | 自动检测最新版本 |
| `TIDY_JOBS` | clang-tidy 最大并发进程数 | 逻辑处理器数量 |
| `TIDY_COMPILE_DB_DIR` | Linux compile database 所在目录 | `build/` |
| `TIDY_CHANGED_LINES` | 等同 `--changed-lines`（CI 通过它传入） | 未设置 |

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
