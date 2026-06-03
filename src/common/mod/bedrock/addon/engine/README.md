# QuickJS引擎实现

IScriptRuntime/Engine/Context的QuickJS实现。

- `quickjs/QuickJSRuntime` — 基于QuickJS的运行时实现，管理内存和JSContext生命周期
- `quickjs/QuickJSEngine` — 基于QuickJS的引擎实现，注册模块工厂并创建上下文
- `quickjs/QuickJSContext` — 基于QuickJS的上下文实现，执行脚本和模块
- `quickjs/QuickJSModuleLoader` — QuickJS模块加载器，从ScriptPluginSource加载脚本
- `quickjs/QuickJSValueConvert` — C++/JS类型转换特化
