# 核心抽象层

定义脚本引擎的抽象接口，使JS引擎可替换（当前实现为QuickJS，未来可切换到V8）。

## 目录结构树

```
src/common/mod/bedrock/addon/core/
├── IScriptRuntime.hpp       # JS运行时抽象，管理上下文生命周期和待执行任务
├── IScriptEngine.hpp        # 脚本引擎抽象，管理模块工厂和上下文创建
├── IScriptContext.hpp       # 脚本上下文抽象，执行脚本和调用函数
├── ScriptData.hpp           # 脚本数据结构（源码、模块名、文件路径、是否ES6模块）
├── ModuleDescriptor.hpp     # 模块描述符（名称、版本、UUID）及 ModuleVersion
├── ModuleDependency.hpp     # 模块依赖声明（支持原生模块依赖和包间依赖）
├── Capabilities.hpp         # 脚本能力集合（AllowEval、ScriptOnly）
├── Privilege.hpp            # 执行权限枚举（Default、RestrictedExecAllowed、EarlyExecAllowed）
├── ScriptResult.hpp         # 脚本执行结果（ScriptValue + 成功/失败状态）
└── ScriptException.hpp      # 脚本异常（错误类型、消息、文件名、行列号）
```

## 内部模块关系

```
IScriptEngine（引擎顶层入口）
    │
    ├── createScriptEngine() 工厂函数 → 创建具体引擎实例
    │
    ├── addModuleFactory() → 注册模块绑定工厂
    │
    └── createContext() → 创建脚本上下文
            │
            └── IScriptRuntime（运行时，一个引擎对应一个运行时）
                    │
                    ├── createContext() → 创建 IScriptContext
                    ├── destroyContext() → 销毁上下文
                    └── executePendingJobs() → 执行异步任务

IScriptContext（插件隔离单元）
    │
    ├── evaluate() / evaluateModule() → 执行脚本
    ├── callFunction() / importModule() → 调用函数/导入模块
    ├── nativeHandle() → 获取原生句柄（仅引擎层使用）
    └── bindingContext() → 获取绑定上下文（modules/层使用）
```

## 上下游外部依赖关系

### 被以下模块依赖
- `src/common/mod/bedrock/addon/engine/` — QuickJS引擎实现，实现本目录的所有接口
- `src/common/mod/bedrock/addon/binding/` — 模块绑定框架，通过 IScriptBindingContext 访问脚本能力
- `src/common/mod/bedrock/addon/modules/` — @minecraft/server 模块绑定实现
- `src/common/mod/bedrock/addon/plugin/` — 插件管理器，创建和管理 IScriptContext
- `src/server/mod/bedrock/addon/` — 服务端脚本管理器

### 依赖以下模块
- `src/common/core/Types.hpp` — 基础类型定义（i32、u8、f64 等）

## 容易踩的坑

1. **nativeHandle() 仅限引擎层使用**：`IScriptContext::nativeHandle()` 返回 QuickJS 的 `JSContext*`，仅允许 `engine/` 目录下的实现代码使用。`modules/` 和 `binding/` 层必须通过 `bindingContext()` 获取引擎无关的 `IScriptBindingContext` 接口。

2. **ContextConfig 默认值问题**：`ContextConfig` 结构体的 `maxMemoryBytes` 和 `maxStackSizeBytes` 设置了默认值（64MB/4MB），这违反了项目规范（函数参数和配置结构体不应使用默认值），但因修改影响面大暂时保留。

3. **上下文生命周期**：`IScriptContext` 由 `IScriptRuntime` 创建，必须通过 `IScriptRuntime::destroyContext()` 销毁，不能直接 delete。

4. **模块版本兼容性**：`ModuleVersion::isCompatibleWith()` 要求主版本号完全一致，次版本号和补丁版本号需大于等于要求值。

5. **ScriptValue 类型限制**：`ScriptValue` 目前不支持 Object/Array/Function 类型的实际数据存储，这些类型只能检测类型，无法提取值。复杂对象的传递需要通过 `IScriptBindingContext` 进行。
