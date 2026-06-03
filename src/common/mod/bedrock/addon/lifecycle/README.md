# 生命周期管理

管理脚本系统的整体生命周期，包括初始化、tick驱动、调度、看门狗和日志。

## 文件结构

- `ScriptManager.hpp/.cpp` — 顶层脚本管理器，协调引擎、插件管理器、事件总线、看门狗、调度器等组件
- `ScriptTickListener.hpp/.cpp` — 每tick驱动脚本系统（beginTick→tick(currentTick)→endTick）
- `ScriptScheduler.hpp/.cpp` — 脚本调度器，实现system.run/runInterval/runTimeout/clearRun
- `ScriptWatchdog.hpp/.cpp` — 脚本超时看门狗，检测执行时间超限和内存越限
- `ScriptLogger.hpp/.cpp` — 脚本日志桥接，将脚本输出转发到spdlog

## 内部模块关系

```
ScriptManager
├── QuickJSEngine           — JS引擎
├── ScriptPluginManager     — 插件管理
├── ScriptEventBus          — 事件桥接
├── ScriptScheduler         — 调度器（system.run等）
├── ScriptWatchdog          — 看门狗
├── ScriptLogger            — 日志
└── BehaviorPackList        — 行为包列表

ScriptTickListener → ScriptManager
    beginTick()  → watchdog.beginTick()
    tick(n)      → scheduler.tick(n) → tickPlugins() → executePendingJobs()
    endTick()    → eventBus.tick() → watchdog.endTick() + watchdog.tick()
```

## ScriptScheduler 调度器

实现基岩版 `system.run()`/`runInterval()`/`runTimeout()`/`clearRun()` API：

| 方法 | 说明 |
|------|------|
| `run(callback)` | 下一tick执行回调，返回runId |
| `runTimeout(callback, tickDelay)` | 延迟N个tick后执行一次 |
| `runInterval(callback, tickInterval)` | 每隔N个tick执行一次 |
| `clearRun(runId)` | 取消已注册的调度 |
| `tick(currentTick)` | 每tick调用，执行到期回调 |
| `clearAll()` | 清除所有调度 |

线程安全：所有公共方法通过mutex保护。回调在锁外执行以避免死锁。

## 外部依赖关系

### 谁依赖了这个目录
- `src/server/mod/bedrock/addon/ServerScriptManager` — 服务端脚本管理器
- `modules/MinecraftModuleFactory` — system.run等API通过ModuleContextData访问调度器

### 这个目录依赖了谁
- `core/` — IScriptEngine等核心接口
- `engine/` — QuickJSEngine
- `plugin/` — ScriptPluginManager
- `event/` — ScriptEventBus
- `pack/` — BehaviorPackList

## 容易踩的坑

1. **调度器回调中的JS调用**：回调在锁外执行，但JS调用必须在脚本线程中进行，当前实现通过JS_DupValue保持函数引用
2. **JS函数引用泄漏**：system.run/runTimeout的回调执行后需要JS_FreeValue，runInterval的回调直到clearRun才释放
3. **ModuleContextData生命周期**：通过JS_SetContextOpaque设置，随JSContext销毁而失效，需要确保在JSContext销毁时释放
