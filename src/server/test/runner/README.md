# runner/ — GameTest 运行编排层

把 `framework/` 的批次 runner + `minecraft/` 的 ServerWorld 绑定组合成可运行的编排器，并聚合结果到报告器。本目录所有类**不对外**——由 `facade/GameTestServer`/`GameTestCommand` 门面封装。依赖 `framework/`+`base/`+`minecraft/`+`mc` server 类型。

## 目录结构

```
runner/
├── GameTestRunner.hpp                   # 运行编排器（持 batch runner + tracker，start/tick/isComplete/exitCode）
├── GameTestRunner.cpp
├── GameTestRunnerBuilder.hpp            # builder（world/ticker/batches/gridStart/testsPerRow → build）
├── GameTestRunnerBuilder.cpp
├── spawner/
│   ├── StructureGridSpawner.hpp         # 网格布局（peekOrigin/advance 两步协议，8/行，列/行间距 32）
│   └── StructureGridSpawner.cpp
├── reporter/                            # 报告输出（与 listener 概念区分：reporter 聚合全局结果）
│   ├── TestReporter.hpp                 # 报告器接口（onTestPassed/onTestFailed/onBatchFinished/onAllFinished）
│   ├── GlobalTestReporter.hpp           # 静态单例委托（广播到全部注册的 reporter）
│   ├── GlobalTestReporter.cpp
│   ├── LogTestReporter.hpp              # spdlog 输出（required=error，optional=warn，passed=info）
│   ├── LogTestReporter.cpp
│   ├── JUnitTestReporter.hpp            # 原子更新 JUnit 快照（失败/中止/跳过，实际耗时）
│   ├── JUnitTestReporter.cpp
│   ├── FailedTestCollector.hpp          # 失败 testName 收集器（供重跑过滤）
│   └── FailedTestCollector.cpp
├── tracker/
│   └── MultipleTestTracker.hpp          # 进度计数（total/passed/failed/done/remaining）
├── attempts/
│   └── ExhaustedAttempts.hpp            # 重试耗尽错误（→ GameTestError(ExhaustedAttempts)）
└── watchdog/
    └── GameTestWatchdog.hpp             # 独立线程监控绝对期限，覆盖阻塞主线程
```

## 内部模块关系

- `attempts/ExhaustedAttempts`：纯错误类型，依赖 `base/error/`。
- `tracker/MultipleTestTracker`：纯计数，无依赖。
- `reporter/TestReporter`（接口）← `LogTestReporter`（依赖 `framework/instance/` + spdlog）+ `JUnitTestReporter`（依赖 `framework/instance/` + `<fstream>`）+ `FailedTestCollector`（依赖 `framework/instance/`，收集失败 testName 供重跑过滤）+ `GlobalTestReporter`（依赖 `TestReporter`）。
- `spawner/StructureGridSpawner`：纯几何，依赖 `BlockPos`。
- `GameTestRunner`：依赖 `framework/batch/` + `minecraft/batch/MinecraftGameTestBatchRunner` + `tracker/` + `reporter/GlobalTestReporter`。`GameTestRunnerBuilder` 依赖 `GameTestRunner`。

## 上下游外部依赖关系

**上游（本目录依赖）**：`base/`+`framework/`+`minecraft/`（`MinecraftGameTestBatchRunner`）；`mc::server::ServerWorld`；spdlog。

**下游（依赖本目录）**：
- `facade/GameTestServer`（1F）经 `GameTestRunnerBuilder` 构造 runner，循环 `tick()` 直到 `isComplete()`，`exitCode()=failedRequiredCount`。
- `facade/GameTestCommand`（1F）经 runner 在线触发 `/gametest runall`。

## 容易踩的坑

1. **`GameTestRunner::tick()` 不调 `GameTestTicker::tick()`**——ticker 由 `GameTestServer`/`IntegratedServer` 的 tick 末尾统一驱动（`GameTestTicker::instance().tick()`），runner 内若再调会双重推进实例。runner 的 `tick()` 仅推进 batch runner（检查批次完成 + 推进下一批）。
2. **实例监听器在 `_trackInstance` 挂载**；放置结构时已失败的实例必须补发失败事件，否则计数失败但 XML 漏报。
3. **JUnit 使用实际耗时**，批内用例共享主线程，耗时不能当作独占 CPU 时间。快照中的未开始用例为 skipped，正在运行用例为 error；正常结束事件替换这些占位状态。
4. **报告 IO 错误视为流水线错误**；先写临时文件再 rename，避免看门狗中止时破坏上一份报告。
5. **`StructureGridSpawner` 两步协议**：`peekOrigin()` 取本测试原点（不推进），放结构后 `advance(sizeX, sizeZ, padding)` 用旋转后真实尺寸 + padding 推进游标，供下一测试。`MinecraftGameTestBatchRunner._createGameTestInstance` 已切换到此 spawner（不再是线性递增 X），按 `testsPerRow` 换行网格排列，间距 `SPACE_BETWEEN_COLUMNS/ROWS=32` 覆盖实体 FOLLOW_RANGE 避免跨测试目标搜索污染。
6. **`GlobalTestReporter` 是单例**——`GameTestServer`/`GameTestCommand` 启动期 `addReporter`，运行结束 `clear()` 避免跨运行残留。`-j16` 下各 `GameTestServer` 实例须用各自唯一 `JUnitTestReporter` 路径（`TempDirHelper` 已保证唯一）。
7. **`LogTestReporter` 区分 required/optional**：required 失败 `spdlog::error`，optional 失败 `spdlog::warn`（对齐 Java LogTestReporter 语义，optional 不计退出码但仍告警）。
8. **`FailedTestCollector` 供失败重跑消费**：`onTestFailed` 收集失败 testName（供外层协调脚本 `scripts/test/run-gametests.ts` 构造 `--gametest-tests` 重跑过滤）。挂载在 `GlobalTestReporter` 单例上，随 `GameTestServer::stop()` 中的 `clear()` 一并移除。
9. **看门狗只共享期限和诊断文字**，不得在监控线程访问世界或报告器。已过期的期限不能在切换阶段时延长。
