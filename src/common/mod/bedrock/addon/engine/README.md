# QuickJS引擎实现

IScriptRuntime/Engine/Context的QuickJS实现，是脚本引擎抽象接口的具体实现层。

## 目录结构树

```
engine/
└── quickjs/                            # QuickJS引擎实现（外部代码不直接引用）
    ├── QuickJSRuntime.hpp/cpp          # 运行时实现，管理JSRuntime生命周期和内存限制
    ├── QuickJSEngine.hpp/cpp           # 引擎实现，注册模块工厂并创建上下文（包含createScriptEngine工厂函数）
    ├── QuickJSContext.hpp/cpp          # 上下文实现，执行脚本和模块加载
    ├── QuickJSModuleLoader.hpp/cpp     # ES6模块加载器，处理import语句和路径规范化
    ├── QuickJSBindingContext.hpp/cpp   # 绑定上下文实现，将IScriptBindingContext映射到QuickJS C API
    └── QuickJSValueConvert.hpp         # C++/JS类型转换模板特化（bool/i32/f64/string/vector）
```

## 内部模块关系

```
QuickJSEngine
    │
    ├── owns ──→ QuickJSRuntime（JSRuntime生命周期管理）
    │                   │
    │                   └── creates ──→ QuickJSContext（JSContext实例）
    │                                         │
    │                                         ├── uses ──→ QuickJSModuleLoader（import解析）
    │                                         │
    │                                         └── owns ──→ QuickJSBindingContext（C++/JS桥接）
    │                                                              │
    │                                                              └── uses ──→ QuickJSValueConvert（类型转换）
    │
    └── stores ──→ IModuleBindingFactory[]（模块绑定工厂注册表）
```

## 上下游外部依赖关系

### 本目录被以下模块依赖
- `core/IScriptEngine.hpp` — 通过 `createScriptEngine()` 工厂函数间接使用（工厂函数定义在QuickJSEngine.cpp）
- `lifecycle/ScriptManager` — 通过IScriptEngine抽象接口使用

### 本目录依赖以下模块
- `core/IScriptRuntime.hpp` — 运行时抽象接口
- `core/IScriptContext.hpp` — 上下文抽象接口
- `core/IScriptEngine.hpp` — 引擎抽象接口
- `core/ScriptResult.hpp` — 脚本执行结果类型
- `binding/IScriptBindingContext.hpp` — 绑定上下文抽象接口
- `binding/IModuleBindingFactory.hpp` — 模块绑定工厂接口
- `<quickjs.h>` — QuickJS C API（QuickJS-NG版本）

## 容易踩的坑

1. **【重要】外部禁止直接引用**：`quickjs/`目录外的代码禁止直接引用QuickJS类或包含`<quickjs.h>`，必须通过`IScriptEngine`/`IScriptRuntime`/`IScriptContext`/`IScriptBindingContext`抽象接口访问。唯一的对外暴露点是`QuickJSEngine.cpp`中的`createScriptEngine()`工厂函数。

2. **void*句柄语义**：`QuickJSBindingContext`通过接口传递的`void*`句柄都是`JSValue*`（堆分配，引用计数为1），使用`retainValue()`/`releaseValue()`管理生命周期。**唯一例外是 `createClassProto()` 返回的类原型句柄**——它是**借用**句柄（归绑定上下文所有，上下文析构时统一释放），调用方**不得** `releaseValue`，否则 double free；反之若像早期实现那样把所有权交给调用方而调用方不释放，句柄持有的原型引用永不复位，`JS_FreeRuntime` 末尾的 `JS_RunGC` 就回收不掉类原型对象及其引用图（方法闭包/字符串/属性表，实测 5.7 MB），同时句柄自身也泄漏。`~QuickJSContext` 必须先 `m_bindingContext.reset()` 再 `JS_FreeContext`，保证句柄在 `ctx->class_proto` 根被释放前归还引用。回归测试见 `tests/unit/common/mod/bedrock/addon/test_binding_handle_lifetime.cpp`。

3. **单线程限制**：QuickJS是单线程引擎，所有JS调用必须在同一线程执行，跨线程调用会导致未定义行为。

4. **内存管理**：QuickJS使用引用计数，必须配对调用`JS_DupValue`/`JS_FreeValue`，`QuickJSBindingContext`已封装此逻辑。

5. **模块加载顺序**：`QuickJSModuleLoader`先查找原生C++模块，再查找源码提供者，最后查找路径映射。
