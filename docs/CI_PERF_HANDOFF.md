# CI 提速交接文档

> 分支：`diag/shutdown-crash`（`g122622/Cubium`）
> 最后更新：2026-11-08

## 硬性约束

- 禁止本地编译和运行测试；全部验证通过当前分支的 GitHub Actions 完成。
- 只允许向当前分支推送，禁止合入 main。触发方式：
  `gh workflow run nightly.yml --ref diag/shutdown-crash`。
- GitHub Actions 缓存总配额为 10 GB，test/release/fuzz 与 vcpkg 共享。
- CI 固定 pococc commit `70955224a6cb37fe52c48ef845c0ea5d83650d7d`，本地源码参考位置
  `/mnt/e/dev/pococc`。临时诊断提交已有回退，以当前工作树为准。

## 已有 CI 证据

最近已完成构建验收的运行：
[42456395241](https://github.com/g122622/Cubium/actions/runs/42456395241)，提交 `cc1f4d44d`。
本轮已通过 prepare 的 7 项执行器回归测试，以及三个编译池的构建、真实 launcher 和冷暖探针。
单测分片及最终覆盖率报告仍在等待后台监控唤醒后验收。

| 指标 | 旧运行 42452181845 | 运行 42453916366 | 本轮 42456395241 |
|---|---:|---:|---:|
| test 构建测量区间（编译及链接） | 485.3 s | 367.3 s | 196.8 s |
| 本轮 ccache 命中 | 3354 | 3640 | 3801 |
| 本轮 ccache 未命中 | 450 | 164 | 3 |
| 本轮 journal 远端编译调用 | 450 | 164 | 3 |
| test 缓存上限 | 2 GB | 5 GB | 5 GB |
| test 实际缓存占用 | 约 2 GB，频繁清理 | 2.1 GB / 5 GB | 2.1 GB / 5 GB |

各池本轮统计（构建前清零，探针使用独立缓存）：

| 池 | 命中 / 可缓存调用 | 命中率 | journal 远端调用 | 编译及链接耗时 |
|---|---:|---:|---:|---:|
| test | 3801 / 3804 | 99.92% | 3 | 196.8 s |
| release | 2026 / 2029 | 99.85% | 3 | 455.7 s |
| fuzz | 1345 / 1346 | 99.93% | 1 | 52.1 s |

所有命中均为 direct hit，所有远端调用均成功、无重试。test/release 仅三个携带构建版本信息的
application 入口 TU 未命中：`BaseApplicationEntry.cpp`、`ServerApplicationEntry.cpp`、
`StandaloneServer.cpp`；fuzz 仅第一个。全部缓存压缩后约 3.56 GB，低于配额。

三个池的 runner-info 均确认链接器为 `/usr/lib/llvm-22/bin/ld.lld`（22.1.8）。
`perfetto.cc` 已命中：本轮不在 journal 的三个未命中源码中，Ninja 的完成行出现在
06:55:47.105，比首个完成项晚约 0.43 秒。上一轮它实际分发并耗时 142.6 秒。
这确认暖缓存已消除该 TU 的重复远端编译；Ninja 完成行不是独立编译耗时，不用它推算链接耗时。
test 总构建测量时间较 485.3 秒基线下降约 59.4%。Release 仍耗时约 7.6 分钟，
高缓存命中率不代表链接也被缓存。

## ccache 日志矛盾的排查

**旧交接中“每条 Submitting 都意味着本轮真实编译”的前提不成立。**
ccache 缓存编译器 stderr，`CCACHE_PREFIX=pococc-cc` 的路由日志也在该缓存范围内。
命中时可能重放历史 `Submitting` 和 `Executing locally`，而不会重新调用 pococc。

历史证据已排除“ccache 没有参与”的判断：

- test 清零后统计为 3640 hit / 164 miss，journal 恰有 164 条成功远端调用；
- Release 清零后统计为 2026 hit / 3 miss，日志却有 2029 条 `Submitting`；
- pococc 的 `Logger.hpp` 确认其日志写入 stderr，journal 则在实际执行后追加。

新增 `scripts/ci/verify_ccache_replay.py` 在各池构建后完成直接验证：

1. 从 Ninja 实际命令检查 launcher 为 `ccache clang++ ...`。
   `compile_commands.json` 自身通常不包含 launcher，不能单靠它判断。
2. 在独立缓存中对小型 TU 冷跑、暖跑，核对冷跑 1 miss / 1 journal，
   暖跑增加 1 hit / 0 journal，同时仍打印相同的 `Submitting` stderr。
3. 归档 Ninja 命令、两次 ccache 日志、stderr、计数器及 `replay.json`。
   探针使用 `pococc-probe-*` 路径，沿用现有 worker 探针分类，不混入项目性能计量。

**三个池的新探针均已通过本轮 CI 验收。** 每池冷跑 1 miss / 1 新 journal，暖跑增加
1 direct hit / 0 新 journal，两次 stderr 完全相同且各含一条 `Submitting`。
暖跑 ccache 日志明确记录 `Reading embedded entry #0 <stderr>` 和 `Result: direct_cache_hit`。
test 的项目构建日志仍有 3824 条 `Submitting`，而本轮实际仅分发 3 次。

分发次数应交叉核对本轮 ccache 统计、journal、distcc client 与 worker 记录，不能统计
`Submitting` 行数，也不能对跨运行恢复的累计计数直接相减。探针归档在各池
`pococc-<pool>-diagnostics-1/ccache-replay/`，原始项目计量仍在同 artifact 根目录。

## 单元测试批处理

根因已经确认：逐用例 CTest 为每个用例新建进程，每次重复初始化全局方块、标签、世界生成注册表。
ASan/UBSan 下初始化约 12.6 秒，runner 上端到端开销更高。旧分片耗时 5900～10600 秒。

现有执行器 `scripts/ci/run_unit_tests_batched.py` 保持 CTest 的全局编号、20 片 stride、
日志 Start 行和 JUnit 接口；每批 200 个用例、四个进程并发。大多数分片已降至数分钟。
批内失败单跑复核，单跑通过以 `[BatchPollution]` 标记。

遗留长尾集中于 shard 6：

- `GlazedTerracottaBlockTest.PistonLandingAppliesRotation`：SIGABRT；
- `ServerChunkManagerTest.ConcurrentGenerateAndUnloadRace`：watchdog 卡死；
- `StructureReferencesRaceTest.MovePlayerAndGenerate_RaceRepro`：watchdog 卡死。

旧实现只有进程结束后的 XML；崩溃时反复二分，超时时整批展开为单例，
会重复初始化大量已经完成或尚未开始的用例。将批次超时降至 300 秒仍不能解决这一点。

本次修改：

- 保留完整 gtest 输出，固定无颜色、非 brief、显示时间的格式；
- XML 未落盘时，用成对 RUN/结束事件恢复已完成结果，包括失败及跳过；
- 明确识别中断用例后，只将它单跑，其余未开始用例继续成批执行；
- 无可用进度时保留二分/单例恢复；单例直接使用 test timeout，不重复复核已隔离失败。
- 在 nightly 的 prepare 阶段验证恢复逻辑，包括崩溃、超时、参数化名、跳过、完整输出和 XML 优先级。

**以上恢复改动尚待 CI 验收。** 不修改实际失败用例；与 CI 性能无关的存量失败另行处理。

## 后续验收

1. 等待本轮单测监控唤醒，读取 shard 6 的真实耗时、恢复日志及 JUnit 失败归属。
2. 等待整轮结束，确认报告核验 27790 个 CTest 登记项在 20 片中恰好覆盖一次，性能证据无错误。
3. 仅将验收文档更新推送至 `diag/shutdown-crash`；文档更新不需要重新触发构建。

后台监控 PID 为 `212200`，每两分钟检查一次；仅构建完成、单测全部完成、整轮完成或
构建失败时通过 `codex queue --resume` 唤醒原会话。
状态和日志分别为 `/tmp/cubium-ci-monitor-42456395241.json`、同名前缀的 `.log`。

为满足只在当前分支验证的约束，Release 发布、image-data 推送和 issue 创建已限制为 main
运行；诊断分支仍保留所有构建、测试及报告 artifact。
