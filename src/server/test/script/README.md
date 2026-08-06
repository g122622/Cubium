# script/ — `@minecraft/server-gametest` JS 绑定层

把 C++ GameTest 门面（`GameTestHelper`/`SimulatedPlayer`/`GameTestSequence`）与注册机制（`GameTestRegistry`）暴露给 JS 行为包，对齐基岩版官方 JS 模块 `@minecraft/server-gametest`。本目录编入 `minecraft-server` exe（不进 `mc_bedrock_addon`，因依赖服务端类型 + `mc_test`），经 `ScriptManager::registerModuleFactory` 钩子由服务端层注册（`GameTestServer`/`IntegratedServer` 初始化时）。

## 子系统物理拆分定位

本目录是服务端专属层（见 `server/test/README.md`）：可同时见服务端类型（`ServerWorld`/`ServerPlayer`/`GameMode`）与 `mc_bedrock_addon` 绑定基础设施头（`IModuleBindingFactory`/`IScriptBindingContext`/`NativeModuleBuilder`/`ClassRegistrar`/`ScriptObjectRegistry`）。`mc_bedrock_addon` 本身**不**依赖 `mc_test`/server（保持 common 纯净）——本层是单向依赖的"消费方"。

## 目录结构

```
script/
├── README.md                            # 本文件
├── GameTestModuleBinding.hpp/.cpp       # IModuleBindingFactory 实现（name="@minecraft/server-gametest"）
├── binding/                             # 各 JS 类/函数绑定
│   ├── ScriptRegister.hpp/.cpp          # 顶层 gametest 命名空间对象（register/registerAsync/setBeforeBatch/setAfterBatch/spawnSimulatedPlayer）
│   ├── ScriptRegistrationBuilder.hpp/.cpp            # C++ builder（持 TestData+JS 回调，registerTest 提交）
│   ├── ScriptRegistrationBuilderBinding.hpp/.cpp     # JS RegistrationBuilder 类绑定（包裹 C++ builder，11 链式方法）
│   ├── ScriptTestHelper.hpp/.cpp        # JS Test 类绑定（转发 GameTestHelper，~12 方法 + spawnSimulatedPlayer/startSequence）
│   ├── ScriptSequence.hpp/.cpp          # JS GameTestSequence 类绑定（thenExecute/thenIdle/thenSucceed/thenFail；thenWait=TODO）
│   ├── ScriptSimulatedPlayer.hpp/.cpp   # JS SimulatedPlayer 类绑定（moveToLocation/lookAtLocation/chat/respawn）
│   └── ScriptGameTestFunction.hpp/.cpp  # BaseGameTestFunction 子类（持 JS 回调，run 同步执行）
└── context/                             # 绑定运行期支撑
    ├── ScriptGameTestAccessor.hpp/.cpp  # 单例：当前测试 helper 压栈（JS 体回调取回 C++ GameTestHelper*）
    ├── ScriptBindingRegistry.hpp/.cpp   # 单例：classId → 原型句柄（运行期 wrap JS 对象取回原型）
    └── ScriptGameTestFunctionContext.hpp # IGameTestFunctionContext 空实现（异步 Future=TODO）
```

## 内部模块关系（依赖顺序，严格单向）

`context/`（`ScriptGameTestAccessor`/`ScriptBindingRegistry`/`ScriptGameTestFunctionContext`，无内部依赖）← `binding/ScriptGameTestFunction`（持 JS 回调，依赖 `context/ScriptGameTestAccessor` 压栈 + facade `GameTestHelper`）← `binding/ScriptRegistrationBuilder`（构造 `ScriptGameTestFunction` 提交 `GameTestRegistry`）← `binding/ScriptRegistrationBuilderBinding`（JS builder 类，转发 C++ builder）← `binding/ScriptTestHelper`（JS Test 类，依赖 `ScriptSequence.wrapSequence` + `ScriptSimulatedPlayer.wrapSimulatedPlayer`）← `binding/ScriptRegister`（顶层命名空间对象，依赖 `ScriptRegistrationBuilder` + `ScriptBindingRegistry`）← `GameTestModuleBinding`（顶层聚合，按序调各 `register*ClassBinding`）。

`binding/ScriptSequence` 与 `binding/ScriptSimulatedPlayer` 平级，均依赖 `context/ScriptBindingRegistry`（取原型）+ facade 类型。

## 上下游外部依赖关系

**上游（本目录依赖）**：
- `mc_bedrock_addon` 绑定基础设施：`IModuleBindingFactory`/`IScriptBindingContext`/`NativeModuleBuilder`/`ClassRegistrar`/`ScriptObjectRegistry`（`common/mod/bedrock/addon/binding/`）+ `IScriptContext`/`ModuleVersion`/`ModuleDependency`（`core/`）。
- `mc_test` framework+base：`BaseGameTestFunction`/`IGameTestFunctionContext`/`IGameTestFunctionRunResult`/`SyncGameTestRunResult`/`GameTestRegistry`/`GameTestSequence`/`TestData`/`GameTestError`/`GameTestErrorType`/`GameTestResult`。
- 服务端 facade/类型：`server/test/facade/GameTestHelper`（测试体门面）、`server/test/simulated/SimulatedPlayer`（ServerPlayer 子类）、`mc::GameMode`（`common/core/Types.hpp`）、`BlockPos`（`common/world/block/BlockPos.hpp`）。
- spdlog（info/warn）。

**下游（依赖本目录）**：
- `GameTestModuleBinding` 经 `ScriptManager::registerModuleFactory(unique_ptr<IModuleBindingFactory>)` 注册（`GameTestServer`/`IntegratedServer` 初始化时）。
- JS 行为包经 `import { gametest } from "@minecraft/server-gametest"` 调用（`gametest.register(suite,name,fn).structureName(...).maxTicks(...)`）。

## 对外 API（JS 侧）

| JS 名 | 类型 | C++ 转发目标 |
|---|---|---|
| `gametest.register(suite,name,fn)` | 顶层函数 → `RegistrationBuilder` | `ScriptRegistrationBuilder`（GC 时 `registerTest` 提交 `GameTestRegistry`） |
| `gametest.registerAsync(...)` | 顶层函数（TODO 同步） | 同上 |
| `gametest.setBeforeBatchCallback`/`setAfterBatchCallback` | 顶层函数（TODO） | `GameTestRegistry.registerBefore/AfterBatchFunction` |
| `gametest.spawnSimulatedPlayer` | 顶层函数（TODO） | 待接线 |
| `RegistrationBuilder` 类 | 11 链式方法 | `ScriptRegistrationBuilder`（batch/maxAttempts/maxTicks/padding/required/requiredSuccessfulAttempts/rotateTest/setupTicks/structureName/structureLocation/tag） |
| `Test` 类 | ~12 方法 + `spawnSimulatedPlayer`/`startSequence` + `currentTick` | `GameTestHelper`（经 `ScriptGameTestAccessor::currentHelper()`） |
| `GameTestSequence` 类 | thenExecute/thenExecuteAfter/thenExecuteFor/thenIdle/thenSucceed/thenFail | `GameTestSequence`（原生） |
| `SimulatedPlayer` 类 | moveToLocation/lookAtLocation/chat/respawn + name | `SimulatedPlayer`（原生） |

## 容易踩的坑

1. **`exportNativeFunction` 不支持捕获状态的 `std::function`**：绑定基础设施的 `IScriptBindingContext::exportNativeFunction` 接收裸 `JSCFunction*`（无 user-data），无法承载捕获 `classId` 的闭包。故顶层 `register` 等用"命名空间对象 + 方法"模式（`gametest.register`，对齐 `@minecraft/server` 的 `system`/`world` 全局对象）。**TODO 偏差**：JS 作者须写 `gametest.register(...)` 而非官方顶层 `register(...)`，待绑定层扩展 `exportNativeFunction` 支持 `std::function`/JSCFunctionData 后迁回。
2. **JS 体回调取 helper 经 Test 对象 opaque（非单例）**：`ScriptGameTestFunction::run` 创建 Test JS 对象，opaque 存 `GameTestHelper*`（非拥有），作首参传给 JS 体（`callFunction1`，对齐基岩 `register(suite,name,(test)=>{...})`）。各 `Test` 方法从 thisVal 经 `ScriptObjectRegistry::unwrap(thisVal, testClassId)` 取回 helper——不依赖单例，async 测试体 `await` 挂起后 then-handler resume 时 thisVal 仍携带正确 helper，多 async 并发安全。`ScriptGameTestAccessor` 单例已废弃保留（见其头注释 TODO）。helper 生命周期由 `MinecraftGameTestInstance` 拥有，测试运行期间稳定。
3. **JS 对象 wrap 需原型句柄，但原型仅在注册期可得**：`Test.startSequence`/`spawnSimulatedPlayer` 返回值 wrap 时需原型，原型句柄存 `ScriptBindingRegistry`（classId → proto），运行期回调查表取回。注册期须 `registerProto` 登记每个类；`setTestClassId` 登记 Test classId 供 `run` 创建对象 + `ScriptTestHelper` 取 opaque。
4. **`RegistrationBuilder` JS 对象 owned=true + 自定义 destroy**：`gametest.register` 构造 C++ `ScriptRegistrationBuilder`（`new`）后 wrap（owned=true），JS GC 时 `_destroyRegistrationBuilder` 先 `registerTest("gametest:empty_3x3")` 提交到 `GameTestRegistry` 再 `delete`。对齐 JS 文档"链式末尾无显式提交"。
5. **脚本引擎销毁前须清测试实例（避免悬垂 JS 句柄）**：异步测试实例的 `m_runResult` 持 JS Promise 句柄，`ScriptGameTestFunction` 持 `IScriptBindingContext*`。引擎销毁后析构实例会访问死上下文。`GameTestServer::stop` 自有顺序保护（`forceStop`→`m_runner.reset()`→`stopCore`）；生产 `/gametest` 路径由 `main.cpp::cleanupServerGameTest`（`GameTestTicker::forceStop` + `GameTestCommand::forceClearAllInstances`）在 `server.stop()` 前调用。`ServerScriptManager::shutdown` 不直接调 forceStop（共享 TU 编入 client，不能引 mc_test）。
6. **`ScriptGameTestFunction` 持 `IScriptBindingContext*` 非拥有**：脚本引擎生命周期内稳定；析构 `releaseValue(m_jsCallback)`。若引擎已销毁则析构不可调（由 `cleanupServerGameTest`/`GameTestServer::stop` 规避）。
7. **`thenWait`/`thenWaitAfter`/`until`/`idle`（JS Promise 语义）已桥接**：`idle(ticks)`→Promise<void>（`ScriptScheduler::runTimeout` 定时 resolve）；`until(fn)`→void（转发原生 `helper->until` 轮询）；`thenWait`/`thenWaitAfter` 转发原生 Wait 步骤。`registerAsync` 与 `register` 统一经 `ScriptGameTestFunction::run` 的 `isPromise` 检测，二者皆允许 JS 体返回 Promise 或普通值。
8. **`ScriptRegistrationBuilder.tag` = TODO**：`TestData` 无 tag 字段（tag 在 `BaseGameTestFunction::addTag`），需 `ScriptGameTestFunction` 持 tags 或 `GameTestRegistry` 支持；当前忽略并记 TODO。
9. **`rotateTest` 复用 `manualOnly` 占位**：`TestData` 无 rotate 字段，`--verify` 旋转标记暂存待 `GameTestServer --verify` 接线（与原生 `NativeTestRegistrationBuilder.rotate` 一致 TODO）。
