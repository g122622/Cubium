# CI 提速交接文档

> 交接对象：接手 CI 性能优化的工程师
> 分支：`diag/shutdown-crash`（`g122622/minecraft-reborn`，即 Cubium）
> 最后更新：2026-11-08

---

## 0. 硬性约束（务必先读）

1. **禁止本地构建、禁止本地运行测试**。所有验证必须通过 GitHub Actions 完成。
   触发的唯一方式：
   ```bash
   gh workflow run nightly.yml --ref diag/shutdown-crash
   gh run list --branch diag/shutdown-crash --limit 1
   ```
2. 分支上已有大量 `chore(diag)` 提交（探索过程中的临时诊断），**部分已回退，部分仍是历史提交**。
   接手时请以当前工作树为准，不要被历史提交的暂存内容误导。
3. 项目有 10 GB 的 GitHub Actions 缓存总配额，三个编译池（test/release/fuzz）与 vcpkg 共享，
   超限时 `scripts/ci/prune_cache_capacity.py` 会**整条淘汰**缓存。
4. pococc 源码在 `E:\dev\pococc`（WSL 下 `/mnt/e/dev/pococc`），CI 使用的是固定 commit
   `70955224a6cb37fe52c48ef845c0ea5d83650d7d`（见 `nightly.yml` 的 `POCOCC_REF`）。

---

## 1. 任务一：单元测试过慢（已完成，待最终验收）

### 1.1 现象

单测作业 20 个分片，每个分片 1362~1390 个用例，基线实测单分片 **5900~10600 秒**
（98~176 分钟）。历史水平是每分片约 20 分钟。

### 1.2 根因（已定量证实）

`gtest_discover_tests` 把每个用例注册成独立的 CTest 用例，ctest 因此**为每个用例起一个
独立进程**。而 `mc_tests` 每个进程都要重建全部原版方块、标签表与世界生成注册表：

| 构建 | 单进程初始化 | 说明 |
|---|---|---|
| Release（无 sanitizer） | 0.76 s | 同机对照 |
| RelWithDebInfo + ASan/UBSan | 12.6 s | 进程内 `main` 计时 |
| CI runner 端到端 | ≈ 38 s | 按分片进程数反推（ELF 装载 + sanitizer 运行库 + 退出） |

一个分片 1362 个用例 ≈ 1362 次初始化，而分片内**用例体总耗时只有约 332 秒**。
结论：问题不是测试逻辑，而是「初始化次数 × 进程数」。

### 1.3 实现

新增 `scripts/ci/run_unit_tests_batched.py`，替代 `ctest` 的逐用例执行：

- 按 ctest 的分片语义选取用例（`index % shard_count == shard_index`，与 `ctest -I first,,20` 等价），
  对外仍输出 `Start <全局编号>: <用例名>` 行 → `scripts/ci/verify_test_coverage.py` 校验不变；
- 输出 JUnit XML（`<testcase name="Suite.Case">`）→ `scripts/ci/summarize_failures.py` 不变；
- 每批 200 个用例合并进一个进程，`--jobs 4` 并发；
- 崩溃（信号终止，gtest XML 不落盘）→ 对未报告的子集**二分**，二分产生的子集重新提交回线程池并发推进；
- 超时 → 与崩溃分开处理：二分对卡死无效（含卡死用例的每层都跑满超时），直接展开为单用例；
- 批次内失败的用例在分片末尾**单跑复核**，用于区分「批内全局状态污染」与「真实回归」，
  污染事件以 `[BatchPollution]` 写进日志（只记录，不判失败）。

参数化用例有一个坑：`gtest_discover_tests` 的登记名是 `GetParam()` 的可读描述
（如 `.../(1-byte object <00>, 0, false)`），而 `--gtest_filter` 需要 `.../0`。
脚本按 `--gtest_list_tests` 输出的 `# GetParam() = ...` 注释建立映射，对外一律用 ctest 登记名。

### 1.4 实测效果

| 轮次 | 分片耗时 |
|---|---|
| ctest 逐用例（基线） | 5900~10600 s |
| 批 60 | 213~2061 s |
| 批 200 + 并发二分 | 146~1956 s |

覆盖率校验通过：`CTest coverage: 27790 registered tests partitioned exactly once across 20 shards`。

### 1.5 遗留

- **长尾分片 shard 6 仍有 ~1956 秒**，其根因是三个用例在批次内崩溃/卡死：
  - `GlazedTerracottaBlockTest.PistonLandingAppliesRotation`（SIGABRT）
  - `ServerChunkManagerTest.ConcurrentGenerateAndUnloadRace`（卡死，watchdog 120 s 后被杀）
  - `StructureReferencesRaceTest.MovePlayerAndGenerate_RaceRepro`（同上）
- 已做「超时批次展开为单用例 + 批次上限 900→300 秒」的修复（commit `c07d6b05f`），
  但该版本尚未完成一次完整验收（被任务二打断）。**接手后先确认这一轮的 shard 6 耗时**。
- `[BatchPollution] EnderDragonEntityTest.CheckCrystals_HealsEveryTenTicksWhileNotFull` 是已知的
  批内污染受害者（单独跑通过）。根因是某些用例会改写全局注册表，属于测试隔离问题，
  值得单独修，但不阻塞本任务。

---

## 2. 任务二：构建阶段 ccache 未生效（**严重问题，进行中**）

### 2.1 现象

用户在 `Run ./.github/actions/build-pococc` 步骤日志中观察到：**几乎每一条
`[N/3816] Building CXX object ...` 后面都紧跟一条 `[pococc] info: Submitting ... to distcc`**。

实测统计（`gh api .../jobs/<id>/logs`）：

| job | run | `Submitting` | `Building CXX` |
|---|---|---|---|
| 128632725987 | 42448531304 | **3824** | 3800 |
| 128643718939 | 42452181845 | **3824** | 3800 |

两轮都接近「全量提交到 distcc」。

### 2.2 关键数据

`ccache --show-stats --verbose`（从诊断 artifact 提取）：

**run 42448531304（05:03:18）**
```
Cacheable calls:        171194 / 171200
  Hits:                  120423 (70.34%)   Direct 90623, Preprocessed 29800
  Misses:                 50771 (29.66%)
Cache size (GB):            2.0 / 2.0 (100.3%)
Files:                      7793
Cleanups:                  12036
```

**run 42452181845（05:57:38）**
```
Cacheable calls:        174998 / 175004
  Hits:                  123777 (70.73%)   Direct 93493, Preprocessed 30284
  Misses:                 51221 (29.27%)
Cache size (GB):            2.0 / 2.0 (100.5%)
Files:                      7768
Cleanups:                  12150
```

两轮增量：**3804 次调用 / 3354 命中 / 450 miss**。

### 2.3 已证实的事实链

`pococc-cc` 只在 `CompilationManager::compile` 决定走 distcc 时打印
`Submitting ... to distcc`（`/mnt/e/dev/pococc/src/pococc/compiler/CompilationManager.cpp:163`）。
若 ccache 命中、根本不调用编译器，就不可能出现这行。

进一步查证 `InvocationAnalyzer`（`src/pococc/compiler/arguments/InvocationAnalyzer.cpp`）：

- `-E` / `-S` / `-fsyntax-only` / `-M` 等一律返回 `non-object-action` → 本地执行，
  日志表现为 `[pococc] warn: Executing locally: non-object-action`；
- 因此「ccache preprocessed mode 的预处理被分发到 distcc」不会产生 `Submitting`。

**实测 logs（两个 job 一致）**：

| 指标 | 数量 |
|---|---|
| `Submitting ... to distcc` | 3824 |
| `Building CXX` | 3800 |
| `Executing locally` | **仅 4 条**（全部是 quickjs 的 `unrecognized-option`） |
| `Executing locally: non-object-action` | **0** |

结论：
1. 3824 次 `Submitting` 就是 3824 次**真实的编译调用**（`-c`），即 ccache 在编译器调用
   层面没有命中；
2. 日志里没有任何 `non-object-action`，只能说明没有「经 pococc 的预处理调用」；
   但 ccache 的内部预处理不经过 `CCACHE_PREFIX`，也不会打日志，所以**不能用它判断
   ccache 是否参与**；
3. `pococc.tsv` 只有 431 行、而 `Submitting` 有 3824 行，这个不一致本身也是待查项
   —— 已确认 `BuildJournal::append` 使用 `O_APPEND` + `flock`，理论上不会丢行。

### 2.4 尚未解释的矛盾（最高优先级）

上一节推得「本轮 3800 次编译全部未命中」，但同一个 job 的 ccache 统计显示：

```
（05:03:18）171200 次调用 / 120423 命中  →  （05:57:38）175004 次调用 / 123777 命中
增量：3804 次调用 / 3354 命中 / 450 miss
```

**「3824 次编译调用全部 miss」与「本轮 3354 次命中」直接矛盾。** 必须由下面的实验裁决，
不要在此前的任何结论上继续推演：

- 若 **`--zero-stats` 后的真实统计显示本轮命中接近 0**：日志与统计一致，
  ccache 确实没有解决问题，继续查 `CMAKE_CXX_COMPILER_LAUNCHER` 是否真的出现在编译命令里、
  缓存键是否稳定；
- 若显示**本轮命中约 3354**：那么 2.2 的「增量」结论不成立，说明统计文件在跨 run 读取时
  不可靠（`.ccache` 目录自带 stats，随缓存恢复/保存造成口径混乱）。此时要解释的是
  「ccache 命中后为何仍有 3800 次 Submitting」，重点检查 launcher 是否真的位于编译器命令之前。

**验证方法（已写进 CI，下一轮生效）**：`build-pococc` 在构建前 `ccache --zero-stats`，
构建后把 `ccache --show-stats --verbose` 直接打到作业日志。

### 2.5 一个高度可疑但尚未证实的假设

ccache 缓存长期处于 **2.0/2.0 GB、100% 占用、12000+ 次清理**的状态，
`Files: 7793` 远少于 3816 个 TU 所需的条目数。若容量不足导致大对象
（`perfetto.cc` 的产物）和 direct-mode manifest 被反复淘汰，就会出现
「每轮固定一批 TU 重新分发」的现象。这与 `perfetto.cc` 每轮都 miss（见第 4 节）
的现象吻合，也是本次把上限放宽到 5 G 的依据——但**必须先解决 2.4 的矛盾**，
否则无法判断调大容量是否有效。

### 2.6 已提交的改动（commit `3434defa7`）

1. **ccache 上限按池分配**（`.github/actions/build-pococc/action.yml`，在 build 步骤内）：
   ```
   test    → 5 G
   release → 2 G
   fuzz    → 1.5 G
   ```
   合计 8.5 G + vcpkg 0.3 G ≈ 8.8 G，低于 10 G 配额并留 1.2 G 余量。
   注意：不能用全局 `CCACHE_MAXSIZE=8G`，会挤占其它池（超限是整条淘汰）。
2. **每轮清零统计**：构建前 `ccache --zero-stats`，构建后 `ccache --show-stats --verbose`
   同时 `tee` 到作业日志。此前统计随 `.ccache` 一起恢复，`--show-stats` 给的是多轮累计值。
3. **Linux 统一使用 lld**（`CMakeLists.txt`）：见第 3 节。

### 2.7 接手后的验证清单

下一轮 CI 完成后依次确认：

0. **最省事、最决定性的一步：直接看编译命令里有没有 ccache。**
   在 configure 之后、build 之前执行（不需要真正编译）：
   ```bash
   ninja -C build -t commands \
     src/common/CMakeFiles/mc_common.dir/RelWithDebInfo/world/block/blocks/cave/HangingRootsBlock.cpp.o
   ```
   - 输出应以 `/usr/bin/ccache /usr/lib/llvm-22/bin/clang++ ...` 开头 → launcher 生效；
   - 若直接以 `pococc-cc` 或 `clang++` 开头 → **ccache 根本没有被调用**，
     这就是「零命中」的原因，与缓存容量无关。**优先做这一步**。
1. **构建日志中 `Submitting` 的次数是否显著下降**（目标：从 3824 降到接近真 miss 数）。
   ```bash
   gh api repos/g122622/minecraft-reborn/actions/jobs/<BUILD_JOB_ID>/logs > /tmp/build.log
   grep -c "\[pococc\] info: Submitting" /tmp/build.log
   grep -cE "\[[0-9]+/3816\] Building CXX" /tmp/build.log
   ```
2. **作业日志中的本轮 ccache 统计**（现在会直接打印）：
   ```
   Cacheable calls: ... / ...
     Hits: ... (Direct: ... Preprocessed: ...)
   ```
   - 若 `Direct` 仍接近于 0 → 容量不是唯一原因，继续查 manifest 失效的其它诱因。
   - 若 `Direct` 恢复到接近 100% → 结论成立，可考虑把 test 池上限再放宽（如 6 G）。
3. **缓存体积**：`gh api repos/g122622/minecraft-reborn/actions/caches` 检查总占用是否逼近 10 G。
4. **构建耗时**：对比 `[1/3816]` → `[3816/3816]` 与链接阶段的时间戳。

### 2.8 如果 2.4 的矛盾无法解释，继续排查的方向

- 用 `CCACHE_LOGFILE=/tmp/ccache.log` 抓单个 TU 两次编译的完整日志，对比哈希输入；
- 检查 `ccache --show-stats --verbose` 的 `Direct` / `Preprocessed` 比例（现在能按轮看到）；
- 确认 `CCACHE_SLOPPINESS`（`pch_defines,time_macros,include_file_mtime,include_file_ctime`）
  是否还需要 `system_headers`；
- 确认 `pococc run` 注入的环境变量（`DISTCC_HOSTS` 等）是否影响 ccache 的哈希
  （正常情况下不应影响，但 pococc 会改写编译命令、追加 `-Wno-gnu-line-marker`，需确认追加位置
  在 ccache 的哈希计算**之后**）；
- 注意 `pococc.tsv` 的第一列是 **PID**，不是任务编号；且它的行数（431）与
  `Submitting` 次数（3824）严重不符，**不能**用它推断远程编译次数。
  （已确认 `BuildJournal::append` 使用 `O_APPEND` + `flock`，不会丢行，这个不一致本身也值得查。）

---

## 3. 顺带发现并修复的问题：GNU ld 链接巨慢

`CMakeLists.txt` 中 `-fuse-ld=lld` 原先只写在 `if(CMAKE_BUILD_TYPE STREQUAL "Release")`
的 LTO 分支里，**且条件写的是 `if(WIN32 OR LINUX)`，而 `LINUX` 这个变量在全仓库从未定义**
（CMake 中未定义变量在 `if()` 里为假）。后果：

- Linux 上 Release 构建也从未使用 lld，`-Wl,-gc-sections` 同样未生效；
- sanitizer 构建（CI 单测）完全没有链接器选项。

实测链接耗时（RelWithDebInfo + ASan，2.3 GB 二进制）：
- `mc_tests`：**155 秒**
- `minecraft-server`：**52 秒**

已改为在 Linux + Clang 下 `find_program(ld.lld)` 并全局 `add_link_options(-fuse-ld=<abs path>)`。
**验证方式**：configure 输出中应出现 `-- Linker: lld (/usr/lib/llvm-22/bin/ld.lld)`；
链接耗时目标降到 30~60 秒。

注意该改动在 commit `3434defa7` 中，尚未经过一次完整构建验证。

---

## 4. 构建耗时构成（run 42452181845，job 128643718939）

| 阶段 | 时间戳 | 耗时 |
|---|---|---|
| `[1/3816]` | 05:51:11 | — |
| `[100/3816]` | 05:53:48 | 2 分 37 秒（关键路径是 `perfetto.cc` 单个 TU **149 秒**） |
| `[3000/3816]` | 05:55:48 | 2 分 |
| `[3800/3816]` | 05:57:34 | 1 分 46 秒 |
| `[3815/3816]` 链接 `mc_tests` | 05:59:xx | 155 秒 |
| `[3816/3816]` 链接 `minecraft-server` | 06:01:xx | 52 秒 |

结论：
- **`perfetto.cc` 是固定关键路径**（149 秒），它每轮都 miss（大对象被 2 G 缓存淘汰）。
  提高上限后应能命中，这是本次改动预期收益最大的一项。
- 链接阶段 3.5 分钟，改用 lld 后预计节省 2~3 分钟。

---

## 5. 涉及文件

```
.github/workflows/nightly.yml                     # 单测作业改用批处理脚本；删除临时诊断作业
.github/actions/build-pococc/action.yml           # ccache 上限按池分配、清零统计、打印本轮统计
scripts/ci/run_unit_tests_batched.py              # 新增：批处理单测执行器
scripts/ci/record_ctest_partition.py              # 未改，仍由 ctest 生成分片清单
scripts/ci/verify_test_coverage.py                # 未改，消费 ctest-output.log 的 Start 行
scripts/ci/summarize_failures.py                  # 未改，消费 ctest-results.xml
CMakeLists.txt                                    # Linux + Clang 统一启用 lld
tests/unit/main.cpp、src/common/world/block/registry/VanillaBlocks.cpp
                                                  # 临时诊断已回退到 main 版本
tests/unit/test_main_entry.hpp                    # 已删除（临时诊断产物）
```

### 主要提交（本分支，从旧到新）

| commit | 内容 |
|---|---|
| `602782a16` | 单测按批执行并提高并发，移除临时诊断 |
| `94b9125c4` | 批次崩溃时二分定位 |
| `0f12bcf32` | 二分并发推进，批大小 60→200 |
| `c07d6b05f` | 超时批次展开为单用例，批次超时 900→300 秒 |
| `3434defa7` | Linux 统一用 lld；ccache 上限按池分配 + 统计可观测 |

---

## 6. 风险与注意事项

1. **ccache 上限调大后，`.ccache` 目录会上传更多数据**（actions/cache save 是同步的），
   构建结束后多花几分钟上传是预期代价；如果发现上传时间不可接受，可回落到 4 G。
2. **`config` 阶段也会调用 ccache**（try_compile 探针），`--zero-stats` 放在 build 步骤开头，
   已排除这部分干扰。
3. 单测批处理改变了「一个用例一个进程」的语义，**批内全局状态污染是真实存在的**，
   已用 `[BatchPollution]` 标记而不是掩盖。若后续要彻底解决，应修测试隔离本身。
4. 单测作业按既定口径**不阻塞 CI**（步骤恒以 0 退出），失败清单由 report 作业汇总。
5. 报告作业的 `verify_test_coverage.py` 对 CTest 的校验是「全局编号集合完全匹配」，
   批处理脚本输出的 `Start` 行已按此实现，**不要修改编号顺序**。