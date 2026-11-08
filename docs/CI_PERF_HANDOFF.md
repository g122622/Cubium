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

最近已完成构建的运行：
[42453916366](https://github.com/g122622/Cubium/actions/runs/42453916366)，提交 `3434defa7`。

| 指标 | 旧运行 42452181845 | 运行 42453916366 |
|---|---:|---:|
| test 构建测量区间（编译及链接） | 485.3 s | 367.3 s |
| 本轮 ccache 命中 | 3354 | 3640 |
| 本轮 ccache 未命中 | 450 | 164 |
| 本轮 journal 远端编译调用 | 450 | 164 |
| test 缓存上限 | 2 GB | 5 GB |
| test 实际缓存占用 | 约 2 GB，频繁清理 | 2.1 GB / 5 GB |

新运行构建前已清零统计：3804 次可缓存调用，命中率 95.69%，其中 direct 3425 次、
preprocessed 215 次。Release 命中 2026/2029（99.85%），实际未命中 3 次。
全部缓存压缩后合计约 3.56 GB，低于配额。

Linux + Clang 已实际选用 `/usr/lib/llvm-22/bin/ld.lld`，两个构建 job 均成功。
`perfetto.cc` 在这一轮仍未命中，journal 记录耗时 142.6 秒；需看下一轮暖缓存结果，
不能仅凭旧缓存清理次数就断言它每轮未命中的具体原因。

## ccache 日志矛盾的排查

**旧交接中“每条 Submitting 都意味着本轮真实编译”的前提不成立。**
ccache 缓存编译器 stderr，`CCACHE_PREFIX=pococc-cc` 的路由日志也在该缓存范围内。
命中时可能重放历史 `Submitting` 和 `Executing locally`，而不会重新调用 pococc。

现有证据已排除“ccache 没有参与”的判断：

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

**以上新探针尚待 CI 验收。** 分发次数应交叉核对本轮 ccache 统计、journal、distcc
client 与 worker 记录，不能统计 `Submitting` 行数，也不能对跨运行恢复的累计计数直接相减。

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

1. 仅将改动推送至 `diag/shutdown-crash`，在同分支触发 nightly。
2. 确认 prepare 中的执行器回归测试通过。
3. 确认三个池的冷/暖探针通过，暖跑没有新增 journal 记录但确有缓存 stderr。
4. 确认 test 暖缓存命中率和 `perfetto.cc` 是否实际再次分发，以及构建耗时。
5. 读取 shard 6 的真实耗时、恢复日志、JUnit 失败归属和分片覆盖率。
6. 确认报告核验 27790 个 CTest 登记项在 20 片中恰好覆盖一次，性能证据无错误。

为满足只在当前分支验证的约束，Release 发布、image-data 推送和 issue 创建已限制为 main
运行；诊断分支仍保留所有构建、测试及报告 artifact。
