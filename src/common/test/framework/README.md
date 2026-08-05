# framework/ — GameTest 引擎无关抽象核心

GameTest 框架的抽象核心层：定义测试函数/助手/动作/序列/实例/批次/Tick 驱动/注册表/监听器/环境的纯抽象与状态机，**不依赖任何具体世界类型**（不 include `ServerWorld`/`IWorld`/`Entity`），仅依赖 `base/` 的末梢类型与 `mc` 基础类型。可单独编译为 `mc_test` 库并被 headless 单元测试驱动（`NullGameTestHelper`）。本目录所有类**不对外**——外部经 `server/test/facade/` 门面间接使用。

## 目录结构

```
framework/
├── function/                          # 测试函数定义
│   ├── IGameTestFunctionContext.hpp   # 函数运行上下文接口 + EmptyGameTestFunctionContext
│   ├── IGameTestRunResult.hpp         # 轮询接口 isComplete()/getError()
│   ├── SyncGameTestRunResult.hpp      # 立即完成的轮询实现
│   ├── BaseGameTestFunction.hpp       # 抽象：元数据(TestData)+virtual run(helper,ctx)+hasTag
│   └── BaseGameTestFunction.cpp
├── helper/                            # 测试助手抽象
│   ├── IGameTestHelper.hpp            # 纯虚接口（~65 方法，全错误即值，9 类）
│   ├── IGameTestHelperProvider.hpp    # helper 创建/克隆解耦接口
│   ├── NullGameTestHelper.hpp         # 空实现（headless 单测用，断言即报错）
│   └── NullGameTestHelper.cpp
├── action/                            # 序列步骤动作
│   ├── BaseGameTestAction.hpp         # run() -> GameTestResult 抽象
│   └── CallbackAction.hpp             # 持 std::function<GameTestResult()> 的动作（含 isWait）
├── sequence/                          # 序列 DSL（链式 thenExecute/thenWait/thenIdle/thenSucceed/thenFail/thenTrigger）
│   ├── SequenceCondition.hpp          # thenTrigger 闩锁（trigger/assertTriggeredThisTick）
│   ├── GameTestSequence.hpp           # 9 方法 + Step 状态机（tick(currentTick)->GameTestResult）
│   └── GameTestSequence.cpp
├── instance/                          # 实例状态机
│   ├── GameTestState.hpp              # 状态枚举 NotStarted/Running/Succeeded/Failed/Stopped + isDone/hasSucceeded/hasFailed
│   ├── BaseGameTestInstance.hpp       # tick 驱动状态机：runAtTickTime/序列/succeedIf/failIf/超时
│   └── BaseGameTestInstance.cpp
├── batch/                             # 批次
│   ├── GameTestBatch.hpp              # 注册期批次：name+testFunctions+before/after+environment
│   ├── GameTestBatch.cpp
│   ├── GameTestBatchListener.hpp      # 批次级监听器 onBatchStarting/onBatchFinished
│   ├── BaseGameTestBatchRunner.hpp    # 抽象批次 runner：按 batch 顺序推进，rotation 叠加
│   └── BaseGameTestBatchRunner.cpp
├── ticker/                            # Tick 驱动（单例）
│   ├── GameTestClearTask.hpp          # 测试后清理任务抽象（tick()->bool 完成）
│   ├── GameTestTicker.hpp             # 单例 + 3 态 Idle/Running/Halting + 实例/clearTask 推进
│   └── GameTestTicker.cpp
├── registry/                          # 注册表（数据容器）
│   ├── GameTestRegistry.hpp           # 单例：按 className/testName 索引 + before/after 批次回调
│   └── GameTestRegistry.cpp
├── listener/                          # 测试状态监听器接口
│   └── IGameTestListener.hpp          # 6 回调（结构加载/通过/失败/开始/重试开始/重试结束）
└── environment/                       # 测试环境定义（setup/teardown）
    ├── TestEnvironmentDefinition.hpp  # 接口：setup/teardown(instance)->GameTestResult
    ├── EnvironmentRegistry.hpp        # 单例：按名解析环境（"default"→空 AllOf）
    ├── EnvironmentRegistry.cpp
    ├── AllOfEnvironment.hpp           # 复合环境：依次 setup/逆序 teardown
    ├── AllOfEnvironment.cpp
    ├── SetGameRulesEnvironment.hpp    # 设置游戏规则（意图存储，applier 应用）
    ├── SetGameRulesEnvironment.cpp
    ├── TimeOfDayEnvironment.hpp       # 设置世界时间
    ├── TimeOfDayEnvironment.cpp
    ├── WeatherEnvironment.hpp         # 设置天气（Clear/Rain/Thunder）
    ├── WeatherEnvironment.cpp
    ├── FunctionsEnvironment.hpp       # TODO: .mcfunction 执行（函数系统未就绪）
    └── FunctionsEnvironment.cpp
```

## 内部模块关系（依赖顺序，严格单向）

`function/` → `helper/`（`BaseGameTestFunction::run` 收 `IGameTestHelper&`）→ `action/`（独立，`CallbackAction` 持 `GameTestResult`）→ `sequence/`（`GameTestSequence` 持 `IGameTestHelper&` + `BaseGameTestAction`）→ `instance/`（`BaseGameTestInstance` 持 function+helper+sequence+listener）→ `batch/`（`GameTestBatch` 持 function+environment；`BaseGameTestBatchRunner` 持 batch+ticker+params+instance）→ `ticker/`（`GameTestTicker` 持 `BaseGameTestInstance*` + `GameTestClearTask`）→ `registry/`（`GameTestRegistry` 持 `BaseGameTestFunction`）→ `listener/`（`IGameTestListener` 收 `BaseGameTestInstance&`）→ `environment/`（`TestEnvironmentDefinition` 收 `BaseGameTestInstance&`，独立于 registry）。

`helper/`↔`sequence/` 存在循环：`IGameTestHelper::startSequence()` 返回 `GameTestSequence&`，`GameTestSequence` 持 `IGameTestHelper&`。已用前向声明 + `.cpp` 内 include 解开（`NullGameTestHelper` 持 `unique_ptr<GameTestSequence>` 成员）。

## 上下游外部依赖关系

**上游（本目录依赖）**：`base/` 全部（`GameTestResult`/`GameTestError`/`TestData`/`TestParameters`/`RetryOptions`/`TestTransform`）、`common/core/Types.hpp`、`common/util/Direction.hpp`（Rotation/Rotations）、`common/util/assert/AssertMacros.hpp`（MC_ASSERT_RELEASE/MC_UNUSED）。

**下游（依赖本目录）**：
- `native/NativeGameTestFunction` 继承 `BaseGameTestFunction`。
- `server/test/script/ScriptGameTestFunction` 继承 `BaseGameTestFunction`（脚本与原生在 `GameTestRegistry` 汇聚）。
- `server/test/minecraft/MinecraftGameTestInstance` 继承 `BaseGameTestInstance`，`MinecraftGameTestBatchRunner` 继承 `BaseGameTestBatchRunner`。
- `server/test/facade/GameTestHelper` 实现 `IGameTestHelper`（绑 `ServerWorld`），`GameTestRegistrar` 封装 `GameTestRegistry`，`GameTestServer` 经 `GameTestTicker::instance()` 驱动。
- `server/test/runner/GameTestRunner` 经 `BaseGameTestBatchRunner` 编排。

## 容易踩的坑

1. **`IGameTestHelper` 是 framework 层接口，`GameTestHelper`（`server/test/facade`）是其具体实现**。状态机（`BaseGameTestInstance`/`GameTestSequence`）只持 `IGameTestHelper&`，不依赖 `ServerWorld`——这保证 framework 可被 headless 单测驱动（`NullGameTestHelper`）。勿在 framework 层引入世界类型 include。
2. **`GameTestTicker` 是单例（`instance()`），3 态 Idle/Running/Halting**。`clear()` 在 Running 态设 Halting 而非立即清空（对齐 Java，规避迭代中清空容器的 UB）；Halting 态下一轮 tick 才真正清空。勿在 Running 态直接 `m_instances.clear()`。
3. **`BaseGameTestInstance::startExecution()` 设 tickCount = -(setupTicks+1)**（负值 setup 阶段，对齐 Java）；`tick()` 内 `++tickCount`，到 0 才触发测试函数。runAtTickTime 回调的 tick 参数是相对测试开始的绝对 tick（非负）。
4. **`GameTestSequence` 的 wait 类步骤（thenWait/thenIdle）失败返回 `nullopt` 继续等待，execute 类步骤失败返回错误立即 fail**；`thenSucceed`/`thenFail` 是终态步骤。`thenTrigger` 经 `SequenceCondition` 本 tick 必须触发否则 Waiting 错误。
5. **环境 setup/teardown 作用域是整个批次而非单个测试**（对齐 Java `GameTestRunner.endCurrentEnvironment`）：批次开始 setup 一次，下一批次 setup 前 teardown 一次。`TestData.environment()` 持字符串键，runner 经 `EnvironmentRegistry` 解析为实例。
6. **`SetGameRulesEnvironment`/`TimeOfDayEnvironment`/`WeatherEnvironment` 的 setup 返回 `MethodNotImplemented`**——framework 层引擎无关无法操作 `ServerWorld`，实际应用由 1C 阶段 `MinecraftEnvironmentApplier` 拦截接管。`FunctionsEnvironment` 因 .mcfunction 系统未就绪整类为 TODO stub。
7. **`GameTestRegistry` 是内部数据容器，不对外**——外部经 `GameTestRegistrar`（`server/test/facade`）间接访问。`registerTestMethod` 同名返回 false 不覆盖。
8. **`BaseGameTestBatchRunner::_createGameTestInstance`/`_runTest` 是纯虚**，由 `MinecraftGameTestBatchRunner`（1C）实现；`_trackInstance` 把实例纳入 ticker + runner 所有权。
