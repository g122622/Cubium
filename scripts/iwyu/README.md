# IWYU（clang-include-cleaner）工具

离线分析 C++ 源文件的 `#include` 冗余与缺失，基于 `clang-include-cleaner`（LLVM 20，随 Visual Studio 18 自带）。不挂钩编译，不影响日常构建。

本目录提供两类工具：
- **iwyu.sh / iwyu.ps1**：交互式 wrapper，只打印建议（`--print=changes`），人工 review；`--edit` 可就地应用但无法逐条过滤。
- **fix_includes.ts**：全仓库自动改写脚本，解析建议、做路径规范化、保护关联头、文本化增删，支持 dry-run。批量清理用这个。同时支持 `.cpp` 与 `.hpp`（`.hpp` 移除默认禁用，见坑 17）。

## 目录结构

```
scripts/iwyu/
├── iwyu.sh            # Bash wrapper（Git Bash 主入口，与 scripts/tidy/tidy.sh 对称；只打印/--edit）
├── iwyu.ps1           # PowerShell wrapper（Windows 原生，与 scripts/tidy/tidy.ps1 对称）
├── fix_includes.ts    # 全仓库自动增删脚本（TypeScript，Node 原生执行；解析建议+路径规范化+保护关联头+文本化改写）
└── README.md          # 本文档
```

## 内部模块关系

```
iwyu.sh / iwyu.ps1
   ├── 调用 clang-include-cleaner.exe（VS 18 自带 LLVM 20，零安装）
   ├── 消费 build/compile_commands.json（CMake 生成，CMakePresets 已固化 CMAKE_EXPORT_COMPILE_COMMANDS=ON）
   │      └── 从 database 取 -D 宏、项目 -I、vcpkg/tracy -isystem
   ├── 用 --extra-arg 补 MSVC/SDK/clang-builtin 系统头（database 缺这些路径）
   └── 复用 scripts/tidy/ 的 MSVC/SDK 路径检测逻辑（代码复制，非 import）

fix_includes.ts
   ├── 复用同一套 clang-include-cleaner 调用 + 系统头检测（逻辑从 iwyu.sh 移植为 TS）
   ├── 走 --print=changes → awk 等价去重 → 解析 +/- 建议
   ├── 路径规范化回查：把工具建议的短前缀（如 "command/Foo.hpp"，从 src/common 根）
   │      统一规范化为项目风格（"common/command/Foo.hpp"，从 src 根）
   ├── 保护 .cpp 关联同名 .hpp（默认跳过其移除建议）
   └── 文本化改写：移除按行号从后往前删；插入按 quoted/angled 分组+字母序
```

与 `scripts/tidy/` 的关键差异：tidy 完全绕开 compile database、全量 `--extra-arg` 重建命令行；iwyu 走 `-p build/` 消费 database + 仅补系统头的混合模式（更优，因为项目 -I/vcpkg 路径已在 database）。

## 上下游依赖

**上游（本目录依赖）：**

- `clang-include-cleaner.exe` — 位于 `D:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin/`
- `build/compile_commands.json` — 由 `cmake --preset windows-clang-relwithdebinfo` 生成（改了 CMakeLists/源文件后须重新 configure 才新鲜）
- `scripts/tidy/tidy.sh` 的系统头检测逻辑（路径检测代码原样移植，非 import）

**下游（依赖本目录）：**

- 无。开发者手动调用，不进 CI，不进构建。

## 容易踩的坑

### 1. compile_commands.json 必须新鲜

wrapper 不重新 configure，只读现有 database。若刚改了 CMakeLists.txt 或加减了源文件，必须先 `./scripts/configure.sh build` 再跑 iwyu，否则分析的是过期命令行。验证：`grep "src/common/core/Result.cpp" build/compile_commands.json` 应有结果。

### 2. --edit 会直接改源文件

默认只 `--print`（只看不改）。`--edit` 会就地修改 .cpp/.hpp，务必先 `git status` 确认工作树干净，或在新分支上跑。wrapper 会在工作树脏时交互确认。

### 3. 工具是 preview，可能崩溃

clang-include-cleaner 是 LLVM preview 工具，对复杂模板/Macro 可能 segfault（exit 139）。wrapper 把 segfault 计 SKIP 而非 FAIL，不阻断批量分析。若大面积崩溃，检查 LLVM 版本是否过低。

### 4. 退出码语义（已实测确认）

clang-include-cleaner（LLVM 20）的退出码语义：exit 0 + 有输出 = SUGGEST（有建议），exit 0 + 无输出 = CLEAN（无建议），exit 139/<0 = CRASH（segfault），其他非 0 = ERROR（解析失败）。"有建议"不改变退出码（仍为 0），wrapper 据此区分 SUGGEST/CLEAN。

### 5. --print=changes 输出 N 段重复（已由 wrapper 去重）

LLVM 20 的 include-cleaner 对单个翻译单元的 `--print=changes` 输出会重复 N 份完全相同的建议块（**N 不固定，与文件复杂度相关**：试点见过 Result.cpp 重复 3 次、BossInfo.cpp 重复 12 次；非多配置 database 导致，是工具固有行为，已验证过滤为单配置 database 后仍重复）。wrapper 内置去重函数：用"保留首次出现顺序"的去重（等价 `awk '!seen[$0]++'`），整段重复时每段内行相同，去重后只剩唯一建议行。`--print`（最终代码）和 `--html` 模式无此问题，不去重。

### 6. .cpp 的关联头会被建议移除

include-cleaner 不识别 ".cpp 应 include 自己的关联 .hpp" 约定。若 .cpp 体内未直接使用关联头的符号（如 Result.cpp 仅 `#include "Result.hpp"` 但体内为空），会被建议移除该 include。处理方式：在关联头 include 后加 `// IWYU pragma: keep`，或在项目层面约定忽略此类建议。这是工具的已知特性，非误报。

### 7. 系统头噪音

默认 `--ignore-headers` 排除 vcpkg/tracy/perfetto/OffsetAllocator/quickjs 等第三方头（单一大正则 `build/vcpkg_installed|third_party/|perfetto|tracy|_deps/|OffsetAllocator|quickjs`）。若 MSVC STL 头产生大量误报，可在调用时追加 `--ignore-headers='...|Program Files/Microsoft Visual Studio|Windows Kits'`。

### 8. --only-headers 是按"被 include 的头"过滤，不是按源文件过滤

源文件范围由 wrapper 的文件/目录参数控制。`--only-headers` 用于收窄"分析哪些头提供的符号"，试点阶段一般不用。

### 9. compile_commands.json 缺系统头是已知缺陷

CMake 不把 MSVC STL / Windows SDK / clang builtin 写入 database（靠 VsDevCmd.bat 的 `INCLUDE` 环境变量隐式注入）。wrapper 用 `--extra-arg` 补这些路径。后续若做 `CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES` 系统头固化改造（独立任务），本 wrapper 的系统头注入逻辑应移除。

### 10. Ninja Multi-Config 多配置重复条目（非风险）

每 .cpp 在 compile_commands.json 有 3 条（Debug/Release/RelWithDebInfo）。实测 include-cleaner 在 `-p` 模式下只取该文件的首条匹配条目（不遍历所有配置），故多配置不会放大重复——重复仅来自工具固有行为（见第 5 条）。三条命令行的 -D/-I 略有差异但对 include 分析无影响。

### 11. iwyu.ps1 必须带 UTF-8 BOM

iwyu.ps1 含大量简体中文注释。PowerShell 5.1 在 Windows 中文系统下，对无 BOM 的 .ps1 默认按 GBK（CP936）解码，会把 UTF-8 中文字节序列误解析，导致注释行与下一行合并、语法破坏、脚本无法运行。**因此 iwyu.ps1 必须以 UTF-8 BOM（`EF BB BF`）开头**，强制 PowerShell 5.1 按 UTF-8 解析。对比：`scripts/tidy/tidy.ps1` 因无中文注释故无此问题。修改 iwyu.ps1 时须确保编辑器保留 BOM（VS Code/Write 工具默认保留）。

### 12. fix_includes.ts 的路径规范化（双 -I 根歧义）

项目同时把 `src/` 和 `src/common/` 都设为 -I 根（见 `src/common/CMakeLists.txt`）。同一个 common 头既能写成 `"common/command/Foo.hpp"`（从 src 根），也能写成 `"command/Foo.hpp"`（从 src/common 根）——两者都能编译。**项目现有风格统一用从 `src` 根的 `common/...` 前缀**，但 clang-include-cleaner 建议插入时可能给短前缀（`"command/Foo.hpp"`）。fix_includes.ts 内置路径规范化回查：把工具建议的路径依次拼到各 -I 根下试探物理文件，找到后统一规范化为从 `src` 根表达，与项目主体风格一致。若回查失败（建议的头在所有 -I 根下都找不到），保守保留原始建议字符串不破坏。

### 13. fix_includes.ts 保护关联头但保护不了"间接使用"

fix_includes.ts 默认跳过对 .cpp 关联同名 .hpp 的移除建议（判定：物理头与 .cpp 同目录且 basename 相同）。但工具对关联头的移除建议本身可能是**误判**——.cpp 体内可能通过关联头间接使用符号（如关联头里 `#include` 的其他头提供的类型）。fix_includes.ts 只保护"关联头"这一类，不保护其他"工具认为冗余但实际传递依赖"的 include。故 `--write` 落地后**必须编译验证**（`./scripts/configure.sh build`），不能盲信。

### 14. fix_includes.ts 插入位置是启发式，非工具权威

`--print=changes` 的插入建议**不带行号**，fix_includes.ts 自行决定插入位置：项目头（quoted）插到最后一个 quoted include 之后，系统头（angled）插到最后一个 angled include 之后，组内按字母序。这符合项目 include 块约定（关联头→项目头→系统头→空行→namespace），但若文件 include 块本身不符合约定（如系统头在项目头之前、或 include 散落在文件中间），插入位置可能不理想。落地后用 `clang-format -i` 复核。

### 15. fix_includes.ts 排除 main.cpp 与 .gen.cpp

`src/server/main.cpp`（无关联头、含 `pragma push_macro` 等预处理魔法）与所有 `.gen.cpp`（自动生成）被脚本硬编码排除。扩到其他目录时，若该目录有类似的特殊入口文件，应在脚本 `EXCLUDE_FILES` 中补上。

### 16. fix_includes.ts 串行执行，全仓库耗时较长

脚本串行处理每个 .cpp/.hpp（用户要求起步串行）。单文件 clang-include-cleaner 解析约 0.5-3 秒，src/server 165 个 .cpp 全量约 5-15 分钟。扩到全 `src/`（千级文件）会耗时数十分钟。后续若需并行，可改用 worker_threads，但要警惕 clang-include-cleaner 多进程的内存占用。

### 17. .hpp 移除默认禁用（传染性风险）

`.hpp` 不是翻译单元，被多个 `.cpp` include。移除 `.hpp` 的某个 include 可能导致**下游 .cpp 隐式依赖断裂**——某个 .cpp 原本靠"本 .hpp 传递引入了 X 头"间接使用 X 的符号，移除后该 .cpp 若未自己 include X 就会编译失败。这种传染性失败不在 .hpp 自身的编译验证范围内，难以本地发现。

故 fix_includes.ts 对 `.hpp` **默认只插入不移除**（`allowRemovals = cppRel.endsWith('.cpp') || REMOVE_HEADERS`）。需要 .hpp 移除时显式传 `--remove-headers`，且务必全量编译验证。`.cpp` 移除无此风险（.cpp 是叶子翻译单元，移除只影响自身），始终允许。

### 18. .hpp 插入建议的"传递可见性"冗余误报

clang-include-cleaner 对 `.hpp` 的插入建议**不识别"类型已通过传递 include 间接可见"**。例如 `CustomServerBossInfo.hpp` 已 include `ServerBossInfo.hpp`，而 `ServerBossInfo.hpp`→`BossInfo.hpp:28` 已 include `ITextComponentFwd.hpp`，故 `ITextComponentFwd` 在 `CustomServerBossInfo.hpp` 中已间接可见——但工具仍会建议插入它。fix_includes.ts 的去重只看"本文件 include 块是否已有该头"（第 533-537 行），不做递归传递可见性检测，故会忠实落地这类建议。

这是**已知特性非 bug**：从 IWYU「include what you use」正统哲学看，每个 .hpp 显式声明自身用到的符号（不依赖"碰巧上游头带了"）是正向收益——上游头重构时不会波及下游。落地后 include 列表变长，但耦合更清晰。若不认可此哲学、希望最小化改动，可人工 review dry-run 输出后选择性 `--write` 单文件。

### 19. .hpp 编译错误文件计 SKIP 不计 FAIL

部分 `.hpp` 因**项目预存在代码问题**无法被工具分析：典型是"前向声明 + `std::unique_ptr` 成员"——`unique_ptr` 析构需完整类型，若 `.hpp` 只 include 了前向声明头（如 `ITextComponentFwd.hpp`）而持有 `unique_ptr<ITextComponent>` 成员，工具在解析时遇到 `sizeof(incomplete type)` 报错。fix_includes.ts 检测到输出含 `due to compiler errors` 时计 **SKIP** 不计 FAIL（第 ~580 行 `compileErrorFiles` 追踪），不阻断批量分析。这是项目代码待修问题（应在 `.hpp` 改用完整头或把析构定义移到 .cpp），非工具误判。

### 20. .hpp 不在 compile_commands.json（CMake 不为头生成编译命令）

`.hpp` 从不出现在 `compile_commands.json` 里。clang-include-cleaner 分析 `.hpp` 时，用 database 中**同目录附近某个 .cpp 条目**的 `-I` 根来"假装编译"该 `.hpp`。fix_includes.ts 的 `buildSlimCompileDb` 已处理：若目标全是 `.hpp`（无 .cpp），从全量 db 取一条 .cpp 条目注入精简 db 提供 `-I` 根（第 328-336 行兜底）。故纯 .hpp 目录扫描也能正常工作。

### 21. IWYU 对「模板定义头」与「传递引入完整类型」的盲区（全量落地实证）

src/server 全量落地（286 文件）暴露两类工具误判，fix_includes.ts 无法自动识别，须人工修复后编译验证兜底：

1. **模板定义头被当冗余移除**：`server/advancement/TriggerInstantiation.hpp` 实质承载 `AbstractCriterionTrigger<T>::trigger<PredicateT>` 模板成员函数的**定义**（`CriterionTrigger.hpp` 里只有声明）。工具看不到"符号引用"（模板定义不产生被引用符号），判定冗余并建议移除。移除后 `VibrationSystemServer.cpp:202` 用 lambda 调用 `trigger` 时，因 lambda 类型无链接不能在别的 TU 定义，报 `used but not defined ... cannot be defined in any other translation unit`。修复：恢复该 include。这类"模板定义头"全仓可能还有，落地后编译验证是唯一兜底。

2. **传递引入完整类型被当冗余移除**：`SpawnConditions.cpp` 调用 `BlockState::isAir()`/`isLiquid()`，`BlockState` 完整定义原经 `Block.hpp` 传递引入。工具判定本文件未直接用 `Block` 符号而建议移除 `Block.hpp`，断裂 `BlockState` 完整定义，报 `member access into incomplete type 'BlockState'`。修复：显式 include `BlockState.hpp`（本文件直接用其成员函数，符合 IWYU「include what you use」——工具本应建议插入但漏了）。这类"成员函数调用的类型来源追踪不足"是工具固有局限。

**结论**：全量 --write 后**必须全量编译验证**（`./scripts/configure.sh build`），不能盲信工具建议。incomplete type / used but not defined 两类错误是 IWYU 误判的典型信号，对应"传递引入完整类型被移除"和"模板定义头被移除"，手动补回相应 include 即可。AssertAll.hpp 这类约定头已由 PROTECTED_HEADERS 机制保护，但模板定义头/完整类型头因无统一判定规则，无法自动保护，靠编译验证兜底。

### 22. PROTECTED_HEADERS 保护项目约定头（AssertAll.hpp）

fix_includes.ts 内置 `PROTECTED_HEADER_BASENAMES`（移除时跳过）与 `PROTECTED_SUBSTITUTES`（插入时若文件已有保护头则跳过替代头）双表。当前保护 `AssertAll.hpp`（项目断言库统一入口，见 `docs/PROJECT_CONVENTIONS.md:445`）：
- 工具对宏展开识别不全：宏经 `AssertAll.hpp→AssertMacros.hpp` 间接定义时，工具或建议「纯删 AssertAll」（误判，如 `ChunkLoadLightTask.cpp:95` 用了 `MC_ASSERT_RELEASE` 会编译失败），或建议「删 AssertAll 插 AssertMacros」（违反统一入口约定）。
- src/server 全量落地实证：21 处 AssertAll 移除 + 18 处 AssertMacros 插入被拦截（dry-run 输出 `SkipProt=39`），不影响其余文件的显式化收益。
- 扩到其他约定头时（如未来出现类似聚合入口），在脚本配置区 `PROTECTED_HEADER_BASENAMES` / `PROTECTED_SUBSTITUTES` 补条目即可。

## 试点验证结果（src/common/core，2026-08-02）

3 个 .cpp 全部跑通，0 FAIL 0 SKIP，去重后输出干净。建议合理性人工核对：
- `Result.cpp`：建议移除 `Result.hpp`（技术上正确，因体内未直接使用其符号；但违反 .cpp/.hpp 配对约定，属工具已知特性，见第 6 条）。
- `GameDirectory.cpp`：5 条插入建议（`Result.hpp`/`<string>`/`<system_error>`/`<utility>`/`<vector>`），均为显式化间接依赖，合理。
- `SettingsBase.cpp`：建议移除 `<sstream>`（正确，体内用 `fstream` 运算符非 stringstream）+ 9 条插入建议（显式化间接依赖），合理。

误报率 0%（16 条建议无真正误报，Result.cpp 的关联头建议属已知特性）。工具可用性高，可推广到 `src/common/` 再逐步扩到 `src/`。

## 全量落地结果（src/server，2026-08-03）

333 文件（164 .cpp + 169 .hpp）全量分析落地，286 文件改动，净增 ~1473 include。编译验证 exit 0（8:39）。

| 指标 | 数值 |
|---|---|
| Clean（无需改） | 27 |
| Suggest（有建议） | 305 |
| Fail | 0 |
| Skip（预存在代码问题） | 1（ServerDragonBossBar.hpp 前向声明+unique_ptr） |
| 移除 | 218（几乎全 .cpp） |
| 插入 | 1688 |
| .hpp 移除被默认禁 | 150 |
| AssertAll 保护拦截 | 39（21 移除 + 18 替代插入） |

落地过程暴露两类工具盲区（见坑 21），手动修复 2 文件后编译通过：
- `SpawnConditions.cpp`：补 `BlockState.hpp`（移除 Block.hpp 断裂完整类型）
- `VibrationSystemServer.cpp`：恢复 `TriggerInstantiation.hpp`（移除模板定义头断裂实例化）

**推广结论**：工具可用性高，但全量落地后**必须全量编译验证**兜底 IWYU 对模板定义头/传递完整类型的盲区。下一阶段可推广到 `src/common/`、`src/client/`。

## 用法速查

### iwyu.sh / iwyu.ps1（交互式 review）

```bash
# 扫试点目录（默认 src/common/core）
./scripts/iwyu/iwyu.sh

# 扫指定文件/目录
./scripts/iwyu/iwyu.sh src/common/core/Result.cpp
./scripts/iwyu/iwyu.sh src/common/core

# 只建议移除 / 插入头
./scripts/iwyu/iwyu.sh --remove src/common/core/
./scripts/iwyu/iwyu.sh --insert src/common/core/

# 应用建议（危险！先确保 git 工作树干净）
./scripts/iwyu/iwyu.sh --edit src/common/core/

# HTML 报告
./scripts/iwyu/iwyu.sh --html=/tmp/iwyu-core.html src/common/core/

# 覆盖默认第三方头排除
./scripts/iwyu/iwyu.sh --ignore-headers='build/vcpkg_installed|third_party/' src/common/core/
```

### fix_includes.ts（全仓库自动改写）

```bash
# dry-run 扫 src/server（默认目标），打印逐文件 diff
node --experimental-strip-types scripts/iwyu/fix_includes.ts

# dry-run 只看汇总统计
node --experimental-strip-types scripts/iwyu/fix_includes.ts --summary-only

# 落地改写（工作树必须干净，脚本会校验）
node --experimental-strip-types scripts/iwyu/fix_includes.ts --write

# 扫指定目录/文件（目录递归展开为 .cpp + .hpp，git ls-files 排除 .gitignore 与 .gen.*）
node --experimental-strip-types scripts/iwyu/fix_includes.ts src/common/core
node --experimental-strip-types scripts/iwyu/fix_includes.ts --write src/common/core

# 允许 .hpp 移除 include（默认禁止，见坑 17；务必全量编译验证）
node --experimental-strip-types scripts/iwyu/fix_includes.ts --write --remove-headers src/server/bossbar

# 落地后三步验收
#   1. git diff 复核
#   2. clang-format -i 格式化改动的 .cpp/.hpp
#   3. ./scripts/configure.sh build 验证编译
```

`fix_includes.ts` 需 Node.js ≥ 22.6（原生 TypeScript 类型擦除，无需 tsx/bun）。环境变量 `IWYU_BUILD_DIR` 可覆盖 compile_commands.json 目录。
