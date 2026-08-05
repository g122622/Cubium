# test/ — GameTest 集成测试框架（引擎无关核心层）

在真实世界 Tick 循环中验证玩法行为的集成测试框架，结合 Java 1.21.11 GameTest 与基岩版 GameTest 的优势，并兼容基岩生态（同时支持原生 C++ 测试与 JS 脚本测试）。断言全程错误即值（`GameTestResult = std::optional<GameTestError>`，nullopt=通过），无任何异常。

## 子系统物理拆分（重要）

`mc::test` 子系统按引擎相关性物理拆分为两处：

- **本目录 `src/common/test/`（引擎无关核心，→ `mc_test` 库）**：`base/`+`framework/`+`native/`（不含 builtin）。仅依赖 `mc_common`，不引入任何 `server/` 类型，可被客户端/服务端/测试共用。
- **`src/server/test/`（服务端专属逻辑，→ 编入 `minecraft-server` exe + `mc_tests`）**：`facade/`+`minecraft/`+`runner/`+`simulated/`+`script/`+`native/builtin/`。依赖 `mc_test` + 服务端类型（`ServerWorld`/`MinecraftServer`/`ServerPlayer`/`CommandRegistry`）+ `mc_bedrock_addon`（script 层）。

拆分动机：门面层 `GameTestHelper` 绑 `mc::server::ServerWorld`，属服务端类型；按项目分层，服务端专属逻辑统一放 `src/server/` 下，`common/` 不得依赖 `server/`。两处合起来构成完整 `mc::test` 子系统，命名空间统一 `mc::test`。

## 架构原则

- **门面模式**：整个 `mc::test` 子系统只对外暴露 4 个门面类（`GameTestRegistrar`/`GameTestHelper`/`GameTestServer`/`GameTestCommand`，均位于 `server/test/facade/`），其余类禁止外部直接使用。外部代码（`tests/`、`minecraft-server`、`mc_bedrock_addon` 的 script 层）只 include `server/test/facade/` 与 `common/test/base/` 的对外头。
- **深度树形目录**：分门别类，每子目录职责单一、2-6 个文件。
- **`base/` 放末梢依赖**：子系统的基础类型/工具库（依赖链末梢）放 `base/`。
- **无混淆命名**：类名/方法名单一清晰（见 `server/test/facade/README.md` 命名区分）。

## 目录结构（本目录：引擎无关核心）

```
common/test/
├── README.md                      # 本文件（核心层总览；子系统全貌见 server/test/README.md）
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
└── native/                        # 原生测试函数族（注册机制，被 GameTestRegistrar 封装）
    ├── NativeGameTestFunction.hpp/cpp
    ├── NativeTestRegistrationBuilder.hpp/cpp
    └── GameTestMacros.hpp         # MC_REGISTER_GAME_TEST 宏（前向声明 facade，不拉 server）
```

服务端专属部分（`facade/`/`minecraft/`/`runner/`/`simulated/`/`script/`/`native/builtin/`）目录结构见 `src/server/test/README.md`。

## 内部模块关系（层间依赖，严格单向）

`base/`（末梢，无内部依赖）← `framework/`（依赖 base）← `native/`（注册机制，依赖 framework+base）；
服务端侧 `server/test/` 内：`minecraft/`/`runner/`/`native/builtin/`（依赖 framework+base+server 类型）← `facade/`（顶层聚合）← `script/`（JS 绑定，依赖 facade+mc_bedrock_addon）；`simulated/` 依赖 `facade/GameTestHelper` 回指。

## 上下游外部依赖关系

**上游（本目录依赖）**：
- `common/core/Result`/`Types`、`mc::math::Random`、`BlockPos`/`Rotation`/`Mirror`、`WorldConstants`、spdlog、nlohmann_json。
- 仅 `mc_common`，**不依赖** `server/` 任何类型（这是本目录可放 `common/` 的前提）。

**下游（依赖本目录）**：
- `src/server/test/`（服务端专属层）：`mc_test` 库 + 服务端类型。
- `mc_tests`：8 个 GameTest 测试源（框架自测，经服务端门面）。

## 容易踩的坑

1. **`GameTestResult` 方向**：`optional<GameTestError>`，nullopt=通过，有值=失败。`if (result)` 判的是"有错误"而非"成功"。
2. **`Result<T>::value()` 失败抛异常**：框架无异常，绝不盲调 `.value()`，用 `MC_TRY_ASSIGN` 或先查 `.failed()`/`.success()`。
3. **本目录严禁引入 `server/` 头**：`base/`/`framework/`/`native/`（不含 builtin）必须保持引擎无关，否则破坏 `common`↔`server` 分层。`facade/GameTestHelper` 等绑 `ServerWorld` 的代码必须在 `server/test/` 下。`GameTestMacros.hpp` 仅前向声明 `GameTestRegistrar`，不 include facade 头，故可留 common。
4. **`native/builtin/` 在 server 侧**：`BuiltinNativeTests.cpp` include `facade/GameTestHelper.hpp`（绑 ServerWorld），故 builtin 移至 `server/test/native/builtin/`；`native/` 注册机制（`NativeGameTestFunction`/`NativeTestRegistrationBuilder`/`GameTestMacros`）留 common。
5. **旋转坐标变换**：`TestTransform` 整型版取 `size-1` 偏移（方块局部坐标 [0,size-1]），浮点版不取（实体坐标 [0,size]）。90°/270° 旋转 X/Z 维度互换，`rotatedSize()` 据此返回。
6. **数据类默认值**：`TestData`/`TestParameters`/`RetryOptions` 保留 Java schema 对齐的默认成员初始化值（已征得用户同意），勿按"配置结构体无默认值"规范删除。
7. **门面纪律**：外部代码只 include `server/test/facade/` 与 `common/test/base/` 对外头；`framework/`/`native/` 内部头仅在 `mc_test`/server exe 内部用。`GameTestSequence`/`TestData`/`GameTestError`/`RetryOptions` 作为门面返回值/参数对外可见，但其头放 `base/` 或 `framework/sequence/`，由 `facade/` 头重新导出。
