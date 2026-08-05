# native/ — GameTest 原生测试函数族（注册机制，引擎无关）

原生 C++ 测试的函数类型、注册 builder、注册宏。本目录（`NativeGameTestFunction`/`NativeTestRegistrationBuilder`/`GameTestMacros`）编入 `mc_test` 库，仅依赖 `base/`+`framework/`，**不依赖 server**，故留 `common/`。`builtin/`（含 facade 依赖）已移至 `src/server/test/native/builtin/`。

## 目录结构

```
native/                                    # 本目录（common，引擎无关）
├── NativeGameTestFunction.hpp             # 原生测试函数（持 std::function<GameTestResult(IGameTestHelper&)>）
├── NativeGameTestFunction.cpp
├── NativeTestRegistrationBuilder.hpp      # 11 链式注册 builder（batch/maxAttempts/maxTicks/padding/required/...）
├── NativeTestRegistrationBuilder.cpp
└── GameTestMacros.hpp                     # MC_REGISTER_GAME_TEST 宏 + wrapNativeBody 适配模板（前向声明 facade，不 include）

# builtin/ 已移至 src/server/test/native/builtin/（含 facade include，依赖 ServerWorld）
```

## 内部模块关系

- `NativeGameTestFunction`：继承 `framework/function/BaseGameTestFunction`，`run()` 同步执行 body → `SyncGameTestRunResult`。body 签名是 `GameTestResult(IGameTestHelper&)`（framework 接口），facade `GameTestHelper`（server 侧）实现 `IGameTestHelper`。
- `NativeTestRegistrationBuilder`：持 `TestData` + body + tags，11 链式方法镜像 `TestData` 字段，`registerTest()` 构造 `NativeGameTestFunction` 提交到 `GameTestRegistry`。
- `GameTestMacros`：`wrapNativeBody<Body>` 把 `void(GameTestHelper&)` 适配为 `GameTestResult(IGameTestHelper&)`（`static_cast<GameTestHelper&>`）；`MC_REGISTER_GAME_TEST` 宏展开为 `static const bool = GameTestRegistrar::register(...).chain().registerTest();`。宏仅前向声明 `GameTestRegistrar`，不 include facade 头，故可留 common。

## 上下游外部依赖关系

**上游（本目录依赖）**：`base/`+`framework/`（`BaseGameTestFunction`/`IGameTestHelper`/`GameTestRegistry`/`SyncGameTestRunResult`）。**不依赖 server**（`GameTestMacros` 仅前向声明 facade，builtin 已移走）。

**下游（依赖本目录）**：
- `server/test/facade/GameTestRegistrar` `register()` 返回 `NativeTestRegistrationBuilder`（按值），供宏链式调用。
- `server/test/facade/GameTestServer` 启动期调 `server/test/native/builtin/registerBuiltinNativeTests()`。
- 测试作者经 `MC_REGISTER_GAME_TEST` 宏注册自定义测试（TU 须在 server 侧，先 include facade 头）。

## 容易踩的坑

1. **body 签名是 `GameTestResult(IGameTestHelper&)`（framework 接口），非 `void(GameTestHelper&)`**：作者写的 `void(GameTestHelper&)` 体经 `wrapNativeBody` 包装为 `IGameTestHelper&` 闭包（内含 `static_cast<GameTestHelper&>`）。包装器在体正常返回后返 `pass()`；主动失败经 `helper.fail(...)` 设状态，由 instance 状态机捕获——包装器仍返 pass。
2. **`MC_REGISTER_GAME_TEST` 宏展开为 `static const bool _x = GameTestRegistrar::register(...).chain().registerTest();`**：须在**命名空间作用域**（非函数内）使用；`GameTestRegistrar::register` 按值返回 `NativeTestRegistrationBuilder`，链式调用修改临时对象，`registerTest()` 提交后返回 bool。包含此头的 TU 须先 include `server/test/facade/GameTestRegistrar.hpp` + `server/test/facade/GameTestHelper.hpp`（这些头在 server 侧）。
3. **静态初始化顺序**：`GameTestRegistry` 是 Meyers 单例（函数局部静态），静态初始化期可安全访问；`server/test/native/builtin/BuiltinNativeTests.cpp` 的宏注册在 `main` 前执行。
4. **`mc_test` 库边界**：本目录（`NativeGameTestFunction`/`NativeTestRegistrationBuilder`/`GameTestMacros`）编入 `mc_test`（不依赖 facade/server）；`builtin/`（server 侧）含 facade include，编入服务器 EXE。
5. **`NativeTestRegistrationBuilder.rotate(bool)` 为 TODO**：`--verify` 旋转压测标记暂存，待 `GameTestServer --verify` 接线时消费。
6. **样例结构 `gametest:empty_3x3` 需资源**：TODO 提供该 `.nbt` 结构到资源包，或改用 `TemplateManager::createProceduralTemplate` 程序化生成空 3x3 模板；否则 `MinecraftStructurePlacer` 取模板失败即测试 fail。
