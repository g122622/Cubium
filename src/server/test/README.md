# test/ — GameTest 集成测试框架（server 整体）

GameTest 框架整体物理位于 `src/server/test/`，作为 server 的一部分编入 `minecraft-server` exe 与 `mc_tests`。命名空间统一 `mc::test`。

在真实世界 Tick 循环中验证玩法行为的集成测试框架，结合 Java 1.21.11 GameTest 与基岩版 GameTest 的优势，并兼容基岩生态（同时支持原生 C++ 测试与 JS 脚本测试）。断言全程错误即值（`GameTestResult = std::optional<GameTestError>`，nullopt=通过），无任何异常。

## 架构演进：取消 mc_test 库

历史上 `base/`+`framework/`+`native/` 曾独立为 `mc_test` 库放在 `src/common/test/`，理由是"引擎无关抽象核心可被客户端/服务端共用"。但在当前代码库中这只是一个未兑现的设计假设：

- 实际只有服务端使用 gametest（MC 原版即如此）。
- client 完全不引用 `mc_test`，`mc_trident_tests` 也不链接它。
- `mc_test` 库被 `minecraft-server` exe 链接，造成"common 库被 server exe 链接"的跨层依赖。

故将其物理迁入 `src/server/test/` 下，取消 `mc_test` 独立库，消除了跨层依赖，目录与逻辑一致，CMake 拓扑更简洁。`base/`/`framework/`/`native/` 仍保持引擎无关（仅依赖 `mc` 基础类型，不 include 任何 `server/` 类型），只是物理位置从 `common/` 移到了 `server/`。

## 目录结构

```
server/test/
├── README.md                      # 本文件（server 层总览）
├── base/                          # 末梢基础类型（依赖链最底，被所有层依赖）
│   ├── error/                     # GameTestError/ErrorType/ErrorContext/Result/CompletedError
│   ├── data/                      # TestData(注册期)/RetryOptions/TestParameters(运行期)
│   ├── coords/                    # TestTransform（相对↔绝对坐标，含旋转）
│   └── tags/                      # GameTestTags（预定义标签常量）
├── framework/                     # 引擎无关抽象核心（内部，不对外）
│   ├── function/                  # BaseGameTestFunction/IGameTestRunResult/SyncRunResult/IGameTestFunctionContext
│   ├── helper/                    # IGameTestHelper(纯虚)/NullGameTestHelper/IGameTestHelperProvider
│   ├── action/                    # BaseGameTestAction/CallbackAction（序列步骤抽象）
│   ├── sequence/                  # GameTestSequence(9法)/SequenceCondition(thenTrigger闩锁)
│   ├── instance/                  # BaseGameTestInstance(状态机)/GameTestState(5态)
│   ├── batch/                     # GameTestBatch/BaseGameTestBatchRunner/GameTestBatchListener
│   ├── ticker/                    # GameTestTicker(单例3态)/GameTestClearTask
│   ├── registry/                  # GameTestRegistry（数据容器，被 GameTestRegistrar 封装）
│   ├── listener/                  # IGameTestListener（6回调）
│   └── environment/               # TestEnvironmentDefinition/EnvironmentRegistry/AllOf/SetGameRules/TimeOfDay/Weather/Functions[TODO]
├── native/                        # 原生测试函数族
│   ├── NativeGameTestFunction.hpp/cpp   # 注册机制（被 GameTestRegistrar 封装）
│   ├── NativeTestRegistrationBuilder.hpp/cpp
│   ├── GameTestMacros.hpp         # MC_REGISTER_GAME_TEST 宏（前向声明 facade，不拉 server）
│   └── builtin/                   # BuiltinNativeTests（样例测试，含 facade include 故在 server 侧）
├── facade/                        # 对外门面层（子系统唯一对外接口）
│   ├── GameTestRegistrar.hpp/cpp  # 注册门面
│   ├── GameTestHelper.hpp/cpp     # 测试体门面（~65 方法，绑 ServerWorld）
│   ├── GameTestServer.hpp/cpp     # 无头运行门面（MinecraftServer 子类）
│   └── GameTestCommand.hpp/cpp    # /gametest 命令门面
├── minecraft/                     # ServerWorld 具体绑定（内部）
│   ├── instance/                  # MinecraftGameTestInstance（状态机具体实现）
│   ├── batch/                     # MinecraftGameTestBatchRunner
│   ├── structure/                 # MinecraftStructurePlacer/StructureBounds
│   ├── helper/                    # MinecraftGameTestHelperProvider
│   ├── listener/                  # WorldVisualizationListener/TestInstanceBlockEntity[TODO]
│   └── environment/               # MinecraftEnvironmentApplier
├── runner/                        # 编排（内部，被 GameTestServer 封装）
│   ├── spawner/                   # StructureGridSpawner
│   ├── reporter/                  # GlobalTestReporter/LogTestReporter/JUnitTestReporter/TestReporter
│   ├── tracker/                   # MultipleTestTracker
│   ├── attempts/                  # ExhaustedAttempts
│   ├── GameTestRunner.hpp/cpp
│   └── GameTestRunnerBuilder.hpp/cpp
├── simulated/                     # 原生 SimulatedPlayer（ServerPlayer 子类，门面返回值对外可见）
│   └── SimulatedPlayer.hpp/cpp
└── script/                        # @minecraft/server-gametest JS 绑定（转发到 facade）
    ├── GameTestModuleBinding.hpp/cpp  # IModuleBindingFactory 实现
    ├── binding/                   # ScriptRegister/ScriptTestHelper/ScriptRegistrationBuilder/ScriptSequence[idle/until JS 桥接=TODO]/ScriptSimulatedPlayer/ScriptGameTestFunction[Sync,Async=TODO]
    └── context/                   # ScriptGameTestFunctionContext
```

## 对外门面类（唯一对外接口）

| 门面类 | 职责 | 使用方 |
|---|---|---|
| `GameTestRegistrar` | 注册门面：`register(suite,name,fn)->NativeTestRegistrationBuilder`，封装 `GameTestRegistry` | 原生测试作者（经 `MC_REGISTER_GAME_TEST` 宏） |
| `GameTestHelper` | 测试体门面：操作世界+断言+序列+spawn（~65 方法，错误即值），实现 `IGameTestHelper`，绑 `ServerWorld` | 测试作者（测试体入参） |
| `GameTestServer` | 无头运行门面：`MinecraftServer` 子类，同步 tick、退出码=失败必需测试数、`--report` JUnit XML、`--tests`/`--verify` CLI | CI/测试宿主 |
| `GameTestCommand` | 命令门面：`/gametest` 自动注册到 `CommandRegistry`，OP-only（权限 2） | 在线调试（`IntegratedServer` 内） |

JS 侧对外（`@minecraft/server-gametest` 模块全局 `register`/`Test`/`RegistrationBuilder`/`SimulatedPlayer`/`startSequence`）经 `script/GameTestModuleBinding` 转发到上述 C++ 门面。

## 内部模块关系（层间依赖，严格单向）

`base/`（末梢，无内部依赖）← `framework/`（依赖 base）← `native/`（注册机制，依赖 framework+base）；
`minecraft/`/`runner/`/`native/builtin/`（依赖 framework+base+server 类型）← `facade/`（顶层聚合）← `script/`（JS 绑定，依赖 facade+mc_bedrock_addon）；`simulated/` 依赖 `facade/GameTestHelper` 回指。

## 上下游外部依赖关系

**上游（本目录依赖）**：
- `mc_common`：`common/core/Result`/`Types`、`mc::math::Random`、`BlockPos`/`Rotation`/`Mirror`、`WorldConstants`、spdlog、nlohmann_json。
- 服务端类型：`ServerWorld`/`MinecraftServer`/`IntegratedServer`/`ServerDimension`/`ServerDimensionManager`/`ServerSettings`/`GameDirectory`/`CommandRegistry`/`ServerCommandSource`/`LevelDatCodec`/`WorldStoragePaths`/`OpListManager`/`TimeManager`/`ServerPlayer`。
- `mc::BlockRegistry`/`mc::entity::EntityRegistry`/`mc::ResourceLocation`/`AxisAlignedBB`/`Directions`/`mc::world::gen::feature::template_::TemplateManager`（结构放置）。
- `mc_bedrock_addon`（`IModuleBindingFactory`/`IScriptBindingContext`/`NativeModuleBuilder`/`ClassRegistrar`/`ScriptObjectRegistry`）——仅 `script/`。

**下游（依赖本目录）**：
- `minecraft-server` exe：`GameTestServer`（无头运行宿主）、`GameTestCommand`（生产 `/gametest`）、`script/GameTestModuleBinding`（注册到 `ScriptManager`）。
- `mc_tests`：8 个 GameTest 测试源（框架自测）。

## 容易踩的坑

1. **本目录是服务端专属**：`facade/GameTestHelper` 绑 `ServerWorld&`，不得移回 `common/`。引擎无关抽象（`IGameTestHelper` 接口、`BaseGameTestInstance` 状态机）在 `framework/`，本目录的门面是其具体实现。
2. **`GameTestServer` 直接继承 `MinecraftServer`**：不继承 `IntegratedServer`（其 `initialize` 末尾起 `m_serverThread`）/`StandaloneServer`（其 `run()` 起线程）。`run()` 在调用线程同步循环 `tick()`，CI 单线程驱动。详见 `facade/README.md`。
3. **`GameTestServer::initialize` 须注入 vanilla builtin 数据包**：worldgen 100% 数据驱动，空数据包致 `RandomState::create` 断言失败。镜像 `IntegratedServer::initialize` 的 `ensureVanillaBuiltinPack`。
4. **`script/` 依赖方向**：`script/` 在本目录（server 侧）编入 exe，可同时见服务端类型与 `mc_bedrock_addon` 头。`mc_bedrock_addon` 本身**不**依赖 server（保持 common 纯净）；JS 工厂经 `scriptManager()->engine().addModuleFactory` 公共钩子由服务端层注册（`GameTestServer::initialize` 与生产 `main.cpp::initializeServerGameTest` 两处）。
5. **生产 `/gametest` 在线路径**：`IntegratedServer.cpp`/`MinecraftServer.cpp` 编入 client exe（client 不链接 server/test 任何符号），不能在那些 TU 内直接引用 GameTest 符号。故生产服务器的 GameTest 接入集中在 `src/server/main.cpp::initializeServerGameTest`（`server.initialize` 成功后调用）：注册内置测试 + 默认环境 + 程序化空模板、`GameTestCommand::registerTo`、`addModuleFactory`、并经 `MinecraftServer::addPostTickCallback` 注册 post-tick 回调驱动 `GameTestTicker::tick()` + `GameTestCommand::cleanupCompletedInstances()`。`addPostTickCallback` 是 `MinecraftServer` 共享基类上的 mc_test 无关钩子，client exe 不受影响。`GameTestServer`（无头宿主）不经此回调，在自身 `tickOnce()` 内直接驱动 ticker，避免双 tick。
6. **`ScriptManager::shutdown` 须先 `GameTestTicker::forceStop()`**：销毁 JS 上下文前清测试实例，避免悬垂 JS 回调访问死上下文。
7. **`SimulatedPlayer` 是 `ServerPlayer` 子类，但 `Player` 不是 `MobEntity`**：`MobEntity::lookAt`/`navigator()`/AI goal 体系不可用，`moveToLocation` 用 `handleMovementInput` 手动驱动。详见 `simulated/README.md`。
8. **`ServerWorld::getOrLoadChunk` 仅主线程同步加载**：`GameTestServer::run()` 须在调用线程 tick（不起线程）才安全；`GameTestCommand` 仅主线程 `/gametest` 调用。
9. **门面纪律**：外部代码（`tests/`、`minecraft-server`、`mc_bedrock_addon` script 层）只 include `facade/` 与 `base/` 对外头；`framework/`/`minecraft/`/`runner/`/`native/builtin/`/`simulated/`/`script/binding/` 内部头仅在 server exe/`mc_tests` 内部用。
10. **CMake 依赖边界**：本目录整体经 `target_sources` 注入 `minecraft-server` + `mc_tests`，link `mc_bedrock_addon`（script）。原 `mc_test` 库已取消，base/framework/native 不再独立成库。
