# facade/ — GameTest 对外门面层

`mc::test` 子系统对外暴露的唯一接口层。外部代码（`tests/`、`minecraft-server` exe、`mc_bedrock_addon` 脚本层）**只** include 本目录头 + `base/` 的对外值类型头。四个门面类各司一职，封装内部 `framework/`/`minecraft/`/`runner/`/`native/` 设施。

## 目录结构

```
facade/
├── GameTestRegistrar.hpp/.cpp   # 注册门面：register(suite,name,fn) -> NativeTestRegistrationBuilder（按值）
├── GameTestHelper.hpp/.cpp      # 测试体门面（~65 方法，绑 ServerWorld，实现 IGameTestHelper，回指 instance）
├── GameTestServer.hpp/.cpp      # 无头运行门面（MinecraftServer 子类，同步 tick，退出码=failedRequiredCount）
└── GameTestCommand.hpp/.cpp     # /gametest 命令门面（run/runall/pos/locate/clear，OP 权限 2）
```

## 四个门面类

| 门面类 | 职责 | 内部封装 | 使用方 |
|---|---|---|---|
| `GameTestRegistrar` | 注册原生测试，返回 builder（按值）链式设置元数据后 `registerTest()` 提交 | `GameTestRegistry`（数据容器） | 原生测试作者（经 `MC_REGISTER_GAME_TEST` 宏） |
| `GameTestHelper` | 测试体 API：断言/操作世界/序列/spawn/SimulatedPlayer，全错误即值 | `IGameTestHelper`+`ServerWorld`+`BaseGameTestInstance` 回指 | 测试作者（测试体入参） |
| `GameTestServer` | 无头运行：建世界→选测试→同步 tick 循环→JUnit XML→退出码 | `IntegratedServer` 世界初始化序列 + `GameTestRunner`+`GameTestTicker` | CI/测试宿主 |
| `GameTestCommand` | `/gametest` 在线调试命令 | `GameTestRegistry`+`GameTestTicker`+`MinecraftGameTestInstance` | 在线玩家/控制台（`IntegratedServer` 内） |

## 内部模块关系

- `GameTestRegistrar::register` → `NativeTestRegistrationBuilder`（`native/`）→ `registerTest()` → `GameTestRegistry`（`framework/registry/`）。builder 按值返回，作者链式 `.structureName(...)...registerTest()`。
- `GameTestHelper` 持 `ServerWorld&` + `BaseGameTestInstance&` + `TestTransform` + `StructureBounds*`。生命周期/调度方法转调 instance（`startExecution`/`succeed`/`fail`/`registerRunAtTickTime`/`registerSucceedCondition`/`registerFailCondition`）；块/实体方法直接操作 `ServerWorld`（经 `BlockRegistry`/`EntityRegistry`/`IWorld` API）；坐标变换委托 `TestTransform`。`startSequence` 经 `instance.createSequence()` 取得 instance 持有并 tick 推进的序列。
- `GameTestServer` 继承 `mc::server::MinecraftServer`（不继承 `IntegratedServer`/`StandaloneServer`，避免其线程）。`initialize(params)` 镜像 `IntegratedServer::initialize` 世界初始化序列（数据包+维度+命令注册），跳过线程/网络/本地客户端；尾段选测试 + 构造 `GameTestRunner` + 挂 reporter。`run()` 同步循环 `MinecraftServer::tick()` + `GameTestTicker::tick()` + `runner->tick()`。
- `GameTestCommand::registerTo` 构造 `gametest` 字面节点树（run/runall/pos/locate/clear 子命令），`setRequirement` OP 权限 2，注册到 `CommandRegistry::dispatcher()`。`_launchTests` 经 `MinecraftGameTestInstance` 创建实例 + 放结构 + 入 `GameTestTicker`。

## 上下游外部依赖关系

**上游（本目录依赖）**：
- `base/`（`TestData`/`TestTransform`/`GameTestError`/`GameTestResult`）+ `framework/`（`BaseGameTestInstance`/`GameTestSequence`/`GameTestRegistry`/`GameTestTicker`/`GameTestBatch`/`EnvironmentRegistry`/`BaseGameTestFunction`）+ `minecraft/`（`MinecraftGameTestInstance`/`MinecraftGameTestHelperProvider`/`StructureBounds`）+ `runner/`（`GameTestRunner`+`GameTestRunnerBuilder`+reporter）+ `native/`（`BuiltinNativeTests`/`NativeTestRegistrationBuilder`/`NativeGameTestFunction`）。
- 服务端类型：`MinecraftServer`/`ServerWorld`/`ServerDimension`/`ServerDimensionManager`/`ServerSettings`/`GameDirectory`/`CommandRegistry`/`ServerCommandSource`/`LevelDatCodec`/`WorldStoragePaths`/`OpListManager`/`TimeManager`。
- `mc::BlockRegistry`/`mc::entity::EntityRegistry`/`mc::ResourceLocation`/`AxisAlignedBB`/`Directions`/spdlog。

**下游（依赖本目录）**：
- `tests/`（1I 8 个测试文件）经 `GameTestServer`/`GameTestCommand`/`GameTestHelper`/`GameTestRegistrar` 编写与运行集成测试。
- `minecraft-server` exe 生产侧：`GameTestServer` 作为无头测试宿主（CI）；`GameTestCommand` 经 `CommandRegistry::registerDefaults()` 注册 `/gametest` 供在线调试。
- `mc_bedrock_addon` 脚本层（1H `script/`）经 `GameTestRegistrar`/`GameTestHelper` 转发 JS `register`/`Test` 调用。

## 容易踩的坑

1. **`GameTestServer` 必须直接继承 `MinecraftServer`**——不继承 `IntegratedServer`（其 `initialize` 末尾起 `m_serverThread` 线程）或 `StandaloneServer`（其 `run()` 起线程）。`run()` 在调用线程内同步循环 `tick()`，CI 单线程驱动。`m_settings` 须为本类自有成员（基类 `m_settings` 是 `ServerSettings&` 引用，绑定到此）。
2. **`GameTestServer::initialize` 须注入 vanilla builtin 数据包**——worldgen 100% 数据驱动，空数据包致 `RandomState::create` 断言失败（见内存 `integratedserver-empty-datapack-crash`）。镜像 `IntegratedServer::initialize` 的 `ensureVanillaBuiltinPack` 调用。
3. **`GameTestServer::run()` 循环顺序**：`MinecraftServer::tick()`（世界/实体/时间）→ `GameTestTicker::instance().tick()`（测试实例状态机）→ `runner->tick()`（批次调度）。顺序错则实例 tick 与批次推进不同步。`GameTestRunner::tick()` 内部**不**再调 ticker（避免双重推进，见 `runner/README` 坑 1）。
4. **`GameTestHelper` 回指 `BaseGameTestInstance`**——构造时单阶段绑定（`MinecraftGameTestHelperProvider::createGameTestHelper` 传入 instance）。不绑则 `runAtTickTime`/`succeedIf`/`failIf`/`currentTick`/`maxTicks` 无从转调。`m_instance` 是非拥有引用，instance 由 batch runner 拥有，helper 生命周期短于 instance。
5. **`GameTestHelper::startSequence` 双轨 TODO**——当前既懒构造 `m_sequence` 又调 `instance.createSequence()` 返回 instance 持有的序列。`m_sequence` 实际未被 tick 推进（instance 只 tick 自己 `createSequence` 出的）。第一阶段样例测试 `thenSucceed()` 立即完成不暴露问题；完整修复需统一为 instance 单一持有（TODO）。
6. **`GameTestCommand::_launchTests` 实例所有权**——ticker 持裸指针，实例须保活到完成。当前用函数静态 `vector<unique_ptr<BaseGameTestInstance>>` 保活（线程不安全，仅主线程 `/gametest` 调用）。完整修复应由 `IntegratedServer` 持有 vector（1I 接线 TODO）。结构放置失败（无 `.nbt` 资源）会立即 fail，悬垂风险低。
7. **`GameTestServer` 退出码 = `failedRequiredCount`**——optional 测试失败不计入（CI 契约 0=全必需通过）。`run()` 末尾 `GlobalTestReporter::onAllFinished()` 通知 reporter 收尾。`-j16` 下各 `GameTestServer` 实例须用各自唯一临时目录（`TempDirHelper`）与唯一 `reportPath`，避免文件竞态。
8. **`GameTestServer::stop()` 顺序**：`m_running=false` → `GameTestTicker::clear()`（清悬垂实例指针）→ 移除 reporter → `runner.reset()` → `stopCore()`（落盘 + `shutdownManagers`）。无玩家/网络，`stopCore` 安全处理空连接。
9. **`GameTestHelper` 命名空间遮蔽**——`mc::test` 内非限定名 `world::gen::...` 不回退 `mc::world`（见内存 BossBarState 坑）。本目录 cpp 用全限定 `mc::world::gen::structure::StructureBoundingBox` 或 `using` 别名规避。
10. **门面纪律**：`framework/`/`minecraft/`/`runner/`/`native/`/`simulated/` 的内部头**仅**在 `mc_test`/server exe 内部使用；外部代码只 include `facade/` 与 `base/` 对外头。`GameTestSequence`/`SimulatedPlayer`/`TestData`/`GameTestError` 作为门面返回值/参数对外可见，但其头由 facade 重新导出。
