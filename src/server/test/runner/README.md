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
│   ├── JUnitTestReporter.hpp            # JUnit XML（testcase/failure/skipped，time=tickCount/20）
│   └── JUnitTestReporter.cpp
├── tracker/
│   └── MultipleTestTracker.hpp          # 进度计数（total/passed/failed/done/remaining）
└── attempts/
    └── ExhaustedAttempts.hpp            # 重试耗尽错误（→ GameTestError(ExhaustedAttempts)）
```

## 内部模块关系

- `attempts/ExhaustedAttempts`：纯错误类型，依赖 `base/error/`。
- `tracker/MultipleTestTracker`：纯计数，无依赖。
- `reporter/TestReporter`（接口）← `LogTestReporter`（依赖 `framework/instance/` + spdlog）+ `JUnitTestReporter`（依赖 `framework/instance/` + `<fstream>`）+ `GlobalTestReporter`（依赖 `TestReporter`）。
- `spawner/StructureGridSpawner`：纯几何，依赖 `BlockPos`。
- `GameTestRunner`：依赖 `framework/batch/` + `minecraft/batch/MinecraftGameTestBatchRunner` + `tracker/` + `reporter/GlobalTestReporter`。`GameTestRunnerBuilder` 依赖 `GameTestRunner`。

## 上下游外部依赖关系

**上游（本目录依赖）**：`base/`+`framework/`+`minecraft/`（`MinecraftGameTestBatchRunner`）；`mc::server::ServerWorld`；spdlog。

**下游（依赖本目录）**：
- `facade/GameTestServer`（1F）经 `GameTestRunnerBuilder` 构造 runner，循环 `tick()` 直到 `isComplete()`，`exitCode()=failedRequiredCount`。
- `facade/GameTestCommand`（1F）经 runner 在线触发 `/gametest runall`。

## 容易踩的坑

1. **`GameTestRunner::tick()` 不调 `GameTestTicker::tick()`**——ticker 由 `GameTestServer`/`IntegratedServer` 的 tick 末尾统一驱动（`GameTestTicker::instance().tick()`），runner 内若再调会双重推进实例。runner 的 `tick()` 仅推进 batch runner（检查批次完成 + 推进下一批）。
2. **实例监听器挂载为 TODO**：`_RunnerListener`（更新 tracker + 广播 reporter）需挂到每个 `BaseGameTestInstance`，但 `MinecraftGameTestBatchRunner._runTest` 当前未暴露实例创建钩子。1D/1F 接线时经 `GameTestBatchListener` 在批次开始时遍历挂载，或改 batch runner 在 `_runTest` 内挂载。未挂载前 tracker/reporter 不更新（测试仍能跑，仅无报告）。
3. **`JUnitTestReporter` 的 `time` 用 `tickCount/20.0`**（20 tps），非 wall-clock；`tickCount` 是 `BaseGameTestInstance` 的相对 tick（含 setup 负值阶段，可能为负或小）。JUnit schema 要求非负，负值会产出非法 XML——1F 接线时须 clamp 到 ≥0（TODO）。
4. **`JUnitTestReporter` 写文件失败不抛**——置 `m_ioError=true` + warn 日志，调用方（`GameTestServer`）须查 `hasIoError()` 决定是否影响退出码（当前不影响，仅记日志）。
5. **`StructureGridSpawner` 两步协议**：`peekOrigin()` 取本测试原点（不推进），放结构后 `advance(sizeX, sizeZ, padding)` 用旋转后真实尺寸 + padding 推进游标，供下一测试。`MinecraftGameTestBatchRunner._createGameTestInstance` 已切换到此 spawner（不再是线性递增 X），按 `testsPerRow` 换行网格排列，间距 `SPACE_BETWEEN_COLUMNS/ROWS=32` 覆盖实体 FOLLOW_RANGE 避免跨测试目标搜索污染。
6. **`GlobalTestReporter` 是单例**——`GameTestServer`/`GameTestCommand` 启动期 `addReporter`，运行结束 `clear()` 避免跨运行残留。`-j16` 下各 `GameTestServer` 实例须用各自唯一 `JUnitTestReporter` 路径（`TempDirHelper` 已保证唯一）。
7. **`LogTestReporter` 区分 required/optional**：required 失败 `spdlog::error`，optional 失败 `spdlog::warn`（对齐 Java LogTestReporter 语义，optional 不计退出码但仍告警）。
