# QuickJS引擎实现

IScriptRuntime/Engine/Context的QuickJS实现。

- `quickjs/QuickJSRuntime` — 基于QuickJS的运行时实现，管理内存和JSContext生命周期
- `quickjs/QuickJSEngine` — 基于QuickJS的引擎实现，注册模块工厂并创建上下文
- `quickjs/QuickJSContext` — 基于QuickJS的上下文实现，执行脚本和模块
- `quickjs/QuickJSModuleLoader` — QuickJS模块加载器，从ScriptPluginSource加载脚本
- `quickjs/QuickJSValueConvert` — C++/JS类型转换特化

`quickjs/` 目录之外没有任何 `.cpp`/`.hpp` 文件引用这些 QuickJS 类。

这是因为架构设计得好——外部代码只通过抽象接口 `IScriptEngine`/`IScriptRuntime`/`IScriptContext`/`IScriptBindingContext` 访问脚本引擎，唯一的耦合点是 `QuickJSEngine.cpp` 里的工厂函数 `createScriptEngine()`（它返回 `std::make_unique<QuickJSEngine>()`），而这个函数本身就定义在 `quickjs/` 内部。所以移动文件只需要改内部的 `#include` 路径和 CMakeLists.txt，不需要动外部代码。
