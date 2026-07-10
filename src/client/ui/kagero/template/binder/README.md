# Kagero 模板绑定上下文

本目录包含 Kagero UI 引擎的绑定上下文组件，负责在模板实例与 C++ 状态系统之间架起数据与回调的桥梁。

## 目录结构

```
binder/
├── BindingContext.hpp  # Value 动态类型、BindingContext 绑定上下文声明
└── BindingContext.cpp  # Value 类型转换、路径解析、订阅桥接实现
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                       BindingContext                              │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────────┐   │
│  │  ExposedVar    │  │  Callback 表    │  │  LoopVariables   │   │
│  │  (expose/       │  │  (exposeCallback│  │  (for: 循环变量) │   │
│  │   Writable/     │  │   /Simple)      │  │                  │   │
│  │   Reactive)     │  │                 │  │                  │   │
│  └────────┬───────┘  └────────────────┘  └──────────────────┘   │
│           │                                                       │
│           ▼                                                       │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  resolveBinding / resolveCollection / setBinding            │ │
│  │           ↓                                                │ │
│  │  _resolvePath  ←─ _splitPath（支持 `.` 与 `[index]`）       │ │
│  │           ↓                                                │ │
│  │  Value::fromAny（StateStore 的 std::any → Value）           │ │
│  └────────────────────────────────────────────────────────────┘ │
└──────────────┬──────────────────────────────────────────────────┘
               │
               ▼
        state::StateStore / event::EventBus
```

**核心职责：**
- `Value`：运行时动态类型（Null/Bool/Integer/Float/String/Array/Object），用于在模板绑定系统中传递任意类型的值。
- `BindingContext`：管理暴露变量、回调、循环变量、订阅者，并将绑定路径解析请求分发到 ExposedVar、循环变量表或 StateStore。

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `kagero/state/StateStore.hpp` - 全局状态存储（`getAny`、`has`、`subscribe`）
- `kagero/state/ReactiveState.hpp` - `Reactive<T>` 响应式包装器（`exposeReactive` 使用）
- `kagero/event/EventBus.hpp` - 事件总线（回调与事件分发）
- `kagero/widget/Widget.hpp` - Widget 基类（回调签名中作为事件源）
- 标准库：`<any>`、`<functional>`、`<unordered_map>`、`<vector>`

**下游依赖（依赖本模块）：**
- `kagero/template/runtime/TemplateInstance` - 实例化模板时通过 `BindingContext` 解析 `bind:` 属性、调用 `on:` 回调、迭代 `for:` 集合。
- `kagero/template/bindings/BuiltinWidgets` / `BuiltinEvents` - 内置属性 setter 与事件绑定器通过 `BindingContext` 读写绑定值。
- `kagero/template/compiler/TemplateCompiler` - 编译期会引用 `BindingContext` 的 API 形态（仅类型层面，不直接持有实例）。

## 容易踩的坑

### 1. StateStore 标量键的嵌套路径解析

`resolveBinding("a.b.c")` 会优先查 `m_exposedVars`，未命中再查 StateStore。**注意**：StateStore 的键是扁平字符串，`a.b.c` 不会自动按 `.` 拆分查 `a` → `b` → `c`，而是：
- 若 StateStore 中存在键 `"a.b.c"`，直接返回该标量值。
- 若不存在，`_resolvePath` 会以 `.` 与 `[index]` 拆分路径，第一段作为根键查 StateStore，根键对应的 `std::any` 通过 `Value::fromAny` 转为 `Value`，再逐层 `getProperty` / `getElement`。

因此要让 `bind:text="player.name"` 走嵌套解析，应在 StateStore 中存 `"player"` 键，值为 `Value::fromObject({{"name", Value("Steve")}})` 或 `std::unordered_map<std::string, Value>`，而不是存标量键 `"player.name"`。两种写法不可混用：一旦 StateStore 中同时存在 `"player"` 与 `"player.name"`，`resolveBinding("player.name")` 会命中 `"player.name"` 标量路径，跳过嵌套解析。

### 2. Value::fromAny 不支持的类型返回 Null

`Value::fromAny` 仅支持 `bool`、`i32`、`i64`、`u32`、`f32`、`f64`、`std::string`、`const char*`、`Value`、`std::vector<Value>`、`std::unordered_map<std::string, Value>`。其他类型（如自定义结构体、`i8`/`u16` 等）会静默返回 `Null`，不会抛异常。若绑定始终拿到空值，先用 `StateStore::getAny` 确认存储的实际类型。

`Value` 内部整型以 `i64` 存储、浮点型以 `f64` 存储，因此 `i64`/`u32`/`f64` 均无精度丢失。访问时按需调用 `asI64()`/`asU32()`/`asF64()`（原生精度）或 `asInteger()`/`asFloat()`（窄化为 `i32`/`f32`，对超出范围的值会截断）。

### 3. 暴露变量优先级高于 StateStore

同一路径下，`expose` / `exposeWritable` / `exposeReactive` 注册的 `ExposedVar` 会**完全屏蔽** StateStore 中的同键值（`resolveBinding`、`hasPath`、`setBinding` 都先查 `m_exposedVars`）。若发现 StateStore 的值在模板中读不到，先检查是否在同一路径上暴露了 C++ 变量。

### 4. setBinding 仅作用于 ExposedVar

`setBinding` 只能写入通过 `exposeWritable` / `exposeReactive` 暴露且 `isWritable = true` 的变量，**不能**回写 StateStore。要修改 StateStore 中的值，必须直接调用 `StateStore::set<T>`。

### 5. 订阅桥接仅在 subscribe 时建立一次

`BindingContext::subscribe(path, ...)` 内部会调用 `StateStore::subscribe(path, ...)` 把 StateStore 的变更转发给 BindingContext 的订阅者，但**仅在调用 `subscribe` 的那一刻**检查 `m_store.has(path)` 并建立桥接。若调用 `subscribe` 时 StateStore 中尚无该键，则后续 `StateStore::set` 不会触发 BindingContext 的订阅者。建议先 `StateStore::set` 初始化键，再 `BindingContext::subscribe`。

### 6. 循环变量 `$var` 与根键同名时的歧义

`resolveBinding("$item.field")` 会优先匹配当前 `loopVar`/`loopValue` 参数；若未命中，再查 `m_loopVariables` 表。`hasPath` 仅检查 `m_loopVariables`，**不会**考虑 `loopValue` 参数。若模板在嵌套 `for:` 中遇到 `$item` 找不到的情况，确认外层是否通过 `setLoopVariable` 注册了同名变量。

### 7. `_splitPath` 对 `[index]` 的处理

路径 `arr[0].field` 会被拆分为 `["arr", "[0]", "field"]` 三段，`[0]` 保留方括号作为独立一段，由 `_resolvePath` 识别为索引访问。**不要**在路径中混用 `arr.0`（不会有索引语义，会被当作属性名 `"0"` 查找，对 `Value::Array` 也能命中，但对 StateStore 中的 `std::vector` 原生数组无效）。
