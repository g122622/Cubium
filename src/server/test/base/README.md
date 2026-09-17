# base/ — GameTest 框架末梢基础类型

GameTest 子系统依赖链最底层的基础类型与工具库。被 `framework/`/`native/`（common 侧）与 `minecraft/`/`runner/`/`facade/`/`script/`/`simulated/`（server 侧 `src/server/test/`）所有上层依赖，本身不依赖本子系统内任何其他目录（仅依赖 `mc` 基础类型：`Types.hpp`/`BlockPos`/`Direction`/`Vector3`/nlohmann_json）。

## 目录结构

```
base/
├── error/                         # 错误模型（全框架统一断言返回）
│   ├── GameTestErrorType.hpp      # 错误类型枚举（int，对齐基岩；JS 侧映射为字符串名）
│   ├── GameTestErrorContext.hpp   # 错误上下文（绝对/相对 BlockPos + tick）
│   ├── GameTestError.hpp          # 错误信封（type+message+params+context）
│   ├── GameTestError.cpp
│   ├── GameTestResult.hpp         # = std::optional<GameTestError>（nullopt=通过）+ pass/fail 工厂
│   └── GameTestCompletedError.hpp # "测试已结束"信号 + GameTestCompletedErrorReason
├── data/                          # 数据 schema（注册期 + 运行期）
│   ├── TestData.hpp               # 注册期元数据（12 字段，Java schema 对齐 + padding/batchName）
│   ├── TestData.cpp               # nlohmann::json 序列化（ADL to_json/from_json）
│   ├── RetryOptions.hpp           # 重试选项（numberOfTries/haltOnFailure + noRetries/unlimitedTries）
│   └── TestParameters.hpp         # 运行期参数（testPos/stopOnFailure/repeatCount/testsPerRow/...）
├── coords/                        # 坐标变换工具
│   ├── TestTransform.hpp          # 结构相对 ↔ 世界绝对（含旋转，整型 BlockPos + 浮点 Vector3d）
│   └── TestTransform.cpp
└── tags/
    └── GameTestTags.hpp           # 预定义标签常量（SuiteAll/SuiteDefault/SuiteDisabled）
```

## 内部模块关系

- `error/` 是最底层：`GameTestErrorType`/`GameTestErrorContext` 无依赖；`GameTestError` 依赖前两者；`GameTestResult` 依赖 `GameTestError`；`GameTestCompletedError` 依赖 `GameTestError`（提供 `toGameTestError` 转换）。
- `data/` 依赖 `error/` 间接无（TestData 不持错误），但 `RetryOptions`/`TestParameters` 仅依赖 `BlockPos`/`Rotation`。`TestParameters` 前向声明 `BaseGameTestFunction`（1B 阶段定义），避免 base→framework 反向依赖。
- `coords/` 仅依赖 `BlockPos`/`Rotation`/`Vector3d`，独立于 `error/`/`data/`。
- `tags/` 仅依赖 `<string_view>`，完全独立。

## 上下游外部依赖关系

**上游（本目录依赖的外部）**：`common/core/Types.hpp`（i32/u64/f64/BlockCoord）、`common/world/block/BlockPos.hpp`、`common/util/Direction.hpp`（Rotation/Mirror）、`common/util/math/Vector3.hpp`（Vector3d）、nlohmann_json（仅 `data/TestData.cpp`）。

**下游（依赖本目录的外部）**：
- `framework/` 全部子目录（`GameTestResult` 是序列/实例/动作的统一返回类型；`TestData` 是 `BaseGameTestFunction` 构造参数）。
- `server/test/facade/GameTestHelper` 的断言方法返回 `GameTestResult`。
- `server/test/facade/GameTestServer`/`GameTestCommand` 构造 `TestParameters`。
- `native/NativeTestRegistrationBuilder` 链式填充 `TestData`。
- `server/test/script/` 绑定层把 `GameTestError`/`GameTestCompletedError` 转 JS Error。
- `server/test/minecraft/MinecraftStructurePlacer` 用 `TestTransform` 算旋转包围盒。

## 容易踩的坑

1. **`GameTestResult = std::optional<GameTestError>`，nullopt=通过**。判失败用 `result.has_value()` 或 `isPass(result)`，**不要**写成 `if (result)` 当作"成功"——optional 的 bool 语义是"有值"即"有错误"即"失败"，方向相反。
2. **`GameTestError::message` 中的 `{0}`/`{1}` 占位符**由 `formattedMessage()` 按 `params` 顺序替换；构造错误时 message 用占位符模板，参数放 `params`，便于 JS 侧本地化与上下文注入。
3. **`TestData` 字段是注册期元数据，`TestParameters` 是运行期参数**，二者分离：旋转在 `TestData` 是结构默认旋转，在 `TestParameters` 是本轮施加的额外旋转（`--verify` 时遍历 4 种）。不要混用。
4. **`TestTransform::rotateRelative` 旋转公式取 `size-1` 偏移**（整型方块坐标，结构局部 [0,size-1]）；`relativeToWorldF` 浮点版**不取 size-1**（实体坐标连续 [0,size]）。两者不能互换。
5. **`TestParameters` 前向声明 `BaseGameTestFunction`**（非完整类型），故成员只能用 `BaseGameTestFunction*` 指针容器；解引用需在 1B 阶段后、include `framework/function/BaseGameTestFunction.hpp` 的 TU 内进行。
6. **数据类默认值已征得用户同意保留**（对齐 Java TestData codec 默认值），勿按"配置结构体无默认值"规范删除。
