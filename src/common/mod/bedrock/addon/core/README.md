# 核心抽象层

定义脚本引擎的抽象接口，使JS引擎可替换。

- `IScriptRuntime` — JS运行时抽象，管理上下文生命周期和待执行任务
- `IScriptEngine` — 脚本引擎抽象，管理模块工厂和上下文创建
- `IScriptContext` — 脚本上下文抽象，执行脚本和调用函数
- `ScriptData` — 脚本数据（源码、模块名等）
- `ModuleDescriptor` — 模块描述符（名称、版本、UUID）
- `ModuleDependency` — 模块依赖
- `Capabilities` — 脚本能力（AllowEval等）
- `Privilege` — 执行权限枚举
- `ScriptResult` — 脚本执行结果
- `ScriptException` — 脚本异常
