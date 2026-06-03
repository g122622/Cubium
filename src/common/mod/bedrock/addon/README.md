# 基岩版Addon兼容模组系统

本目录实现了完整兼容基岩版Addon的模组系统，使用户可以直接使用现有的基岩版行为包（Behavior Pack）。

## 架构概览

```
addon/
├── core/          # 脚本引擎核心抽象层（IScriptRuntime/Engine/Context）
├── engine/        # QuickJS引擎实现
├── binding/       # 模块绑定框架（C++→JS桥接）
├── component/     # 自定义组件系统（BlockComponentRegistry/ItemComponentRegistry）
├── modules/       # @minecraft/server 模块绑定实现
├── plugin/        # 插件管理（发现、加载、生命周期）
├── pack/          # 行为包系统（manifest解析、包管理、依赖解析）
├── event/         # 脚本事件桥接（BeforeEvent/AfterEvent）
├── lifecycle/     # 生命周期管理（ScriptManager、ScriptScheduler、Watchdog、Logger、TickListener）
└── diagnostics/   # 诊断与调试（ScriptDiagnostics、SentryLogger）
```

## 关键设计决策

- **JS引擎抽象**：通过 `IScriptRuntime`/`IScriptEngine`/`IScriptContext` 接口抽象JS引擎，当前使用QuickJS实现，未来可切换到V8。**【重要】所有需要使用脚本引擎能力的代码，必须通过这些抽象接口访问，严禁在 engine/ 目录之外直接引用 `<quickjs.h>` 或使用QuickJS特有类型（JSContext*、JSValue等）。详见 PROJECT_CONVENTIONS.md 中的"脚本引擎抽象准则"。**
- **模块绑定**：使用声明式API（`IModuleBindingFactory`）注册C++类和函数到JS上下文
- **事件系统**：BeforeEvent同步可取消，AfterEvent延迟批量处理
- **行为包优先级**：行为包内容覆盖DataPack中同路径内容
- **插件生命周期**：Unloaded → Loading → Loaded → Running → Error/Unloading

## 事件桥接架构

```
ServerEventBus (C++游戏事件)
    │
    ├── BeforeEventBridge ──→ ScriptEventBus ──→ BeforeEventSignal ──→ JS回调(可取消)
    │                                                                    │
    │                                          HandlerResult::BypassListeners
    │                                          → CoordinatorResult::Cancel
    │
    └── AfterEventBridge ──→ ScriptEventBus.enqueueAfterEvent()
                                    │
                              Tick结束时flush ──→ AfterEventSignal ──→ JS回调(只读)
```

## 游戏对象包装架构

```
C++游戏对象 (Entity*, Block*, Dimension*, ...)
    │
    ├── ScriptObjectHandle ──→ JS对象生命周期管理
    │
    ├── ScriptEntity ──→ JS Entity对象（持有 EntityId/Entity* 引用）
    ├── ScriptBlock ──→ JS Block对象（持有 BlockPos + DimensionId）
    └── ScriptItemStack ──→ JS ItemStack对象（持有 ItemStack数据副本，值语义）
```

## System调度器架构

```
ScriptTickListener
    │
    ├── m_pendingRuns ──→ std::vector<ScheduledCallback> (system.run)
    ├── m_intervalRuns ──→ std::unordered_map<RunId, IntervalCallback> (system.runInterval)
    ├── m_timeoutRuns ──→ std::unordered_map<RunId, TimeoutCallback> (system.runTimeout)
    │
    └── tick() 每tick执行:
          1. 处理m_pendingRuns
          2. 检查m_timeoutRuns到期项
          3. 检查m_intervalRuns到期项
          4. 递增m_currentTick
```

## 引擎抽象绑定架构

```
IScriptContext
    │
    └── bindingContext() ──→ IScriptBindingContext& (引擎无关接口)
                                    │
                                    ├── QuickJSBindingContext (QuickJS实现)
                                    │       └── 封装 JSContext*, JSValue 操作
                                    │
                                    ├── 值创建: createInt32/createString/createObject/...
                                    ├── 属性操作: setProperty/getProperty/...
                                    ├── 类型检查: isFunction/isObject/isNumber/...
                                    ├── 函数调用: callFunction/callFunction0/callFunction1
                                    ├── 类注册: allocateClassId/registerClass/createClassProto
                                    ├── 模块注册: createNativeModule/exportNativeValue/finalizeModule
                                    └── 引用管理: retainValue/releaseValue

modules/types/ (已重构):
    ScriptVec2/ScriptVec3/ScriptColor
        └── fromJs(IScriptBindingContext&, void*) / toJs(IScriptBindingContext&) → void*
            (不再依赖quickjs.h)

modules/ (待重构):
    MinecraftModuleFactory/ScriptEventBinding/ScriptCustomComponentBinding
        └── 目前仍直接使用JSContext*/JSValue，需要改为IScriptBindingContext
```

## 服务端集成

服务端集成代码位于 `src/server/mod/bedrock/addon/ServerScriptManager`，在MinecraftServer中初始化和驱动。

## 当前实现状态

### 基础设施（已完成）
- [x] QuickJS引擎集成与抽象接口
- [x] 行为包系统（manifest解析、包列表、依赖解析）
- [x] 事件系统基础设施（BeforeEvent/AfterEvent信号）
- [x] 模块绑定框架
- [x] 插件生命周期管理
- [x] 服务端集成（初始化、tick驱动、关闭）
- [x] 自定义方块/物品组件系统
- [x] 脚本绑定抽象接口（IScriptBindingContext + QuickJSBindingContext实现）

### 引擎抽象重构进度

modules/层和binding/层代码不应直接引用`<quickjs.h>`，应通过IScriptBindingContext访问脚本能力。

| 文件 | 状态 | 说明 |
|------|------|------|
| `binding/IScriptBindingContext.hpp` | ✅ 已抽象 | 引擎无关的绑定接口，含回调类型和高级注册方法 |
| `engine/QuickJSBindingContext.cpp` | ✅ 已实现 | QuickJS后端实现，含JSCFunctionMagic trampoline |
| `core/IScriptContext.hpp` | ✅ 已更新 | 新增bindingContext()方法 |
| `modules/types/ScriptVec2.hpp` | ✅ 已重构 | 不再依赖quickjs.h |
| `modules/types/ScriptVec3.hpp` | ✅ 已重构 | 不再依赖quickjs.h |
| `modules/types/ScriptColor.hpp` | ✅ 已重构 | 不再依赖quickjs.h |
| `binding/ScriptClassBinding.hpp` | ✅ 已重构 | 使用IScriptBindingContext&/void*/u64 |
| `binding/ScriptCallbackHolder.hpp` | ✅ 新增 | 引擎无关的回调持有器 |
| `binding/TypeConverter.hpp` | ✅ 已删除 | 功能已合并到IScriptBindingContext |
| `modules/MinecraftModuleFactory.cpp` | ✅ 已重构 | 使用IScriptBindingContext和ScriptMethodCallback |
| `modules/ScriptEventBinding.cpp` | ✅ 已重构 | 使用IScriptBindingContext，无QuickJS依赖 |
| `modules/ScriptCustomComponentBinding.hpp` | ✅ 已重构 | 头文件无QuickJS依赖，使用void*/IScriptBindingContext& |
| `modules/ScriptCustomComponentBinding.cpp` | ✅ 已重构 | 使用ScriptCallbackHolder和IScriptBindingContext |

### @minecraft/server API绑定（部分实现）
- [x] `blockComponentRegistry.registerCustomComponent()` — 完整实现
- [x] `itemComponentRegistry.registerCustomComponent()` — 完整实现
- [x] 类注册框架（World, System, Dimension, Entity, Player, Block, ItemStack）
- [ ] `system.run/runInterval/runTimeout/clearRun` — Stub，需集成ScriptTickListener
- [ ] `system.currentTick` — Stub，返回0
- [ ] `world.getDimension()` — Stub，返回undefined
- [ ] `world.getAllPlayers()` — Stub，返回空数组
- [ ] `world.sendMessage()` — Stub，无操作
- [ ] `world.beforeEvents/afterEvents` — 未实现
- [ ] Entity/Player/Dimension属性 — 全部返回undefined
- [ ] Block方法 — 未实现
- [ ] ItemStack属性 — 返回undefined
- [ ] ServerEventBus → ScriptEventBus事件桥接 — 未连接

### 未实现的模块
- [ ] @minecraft/server-ui（ActionForm/ModalForm/MessageForm + FormPromiseTracker）
- [ ] @minecraft/server-net（HttpClient/WebSocket）
- [ ] @minecraft/server-gametest
- [ ] @minecraft/server-admin
- [ ] @minecraft/server-editor
- [ ] Dynamic Properties持久化
- [ ] Structure Manager API
- [ ] Custom Spawn Rules
- [ ] 调试器支持（ScriptDebugger）
- [ ] V8引擎后端
- [ ] 脚本热重载（/reload命令）
- [ ] Watchdog完整实现（超时终止脚本上下文）

## 外部依赖关系

### 本目录被以下模块依赖
- `src/server/mod/bedrock/addon/` — 服务端脚本管理器（ServerScriptManager）
- `src/server/` — MinecraftServer通过ServerScriptManager驱动脚本系统

### 本目录依赖以下模块
- `src/common/world/` — 世界、维度、方块、实体等游戏对象
- `src/common/event/` — 事件总线基础设施
- `src/server/event/` — ServerEventBus
- `src/common/util/` — 工具类（日志、随机数等）

## 容易踩的坑

1. **Stub陷阱**：modules/中的大量API绑定目前是stub实现，返回undefined/0/空数组，不要误以为它们已经可用
2. **双事件系统**：ServerEventBus（C++原生事件）和ScriptEventBus（JS脚本事件）目前未桥接，脚本无法订阅游戏事件
3. **对象生命周期**：ScriptEntity等包装类持有的C++指针可能在tick间失效，需要使用弱引用或ID引用
4. **QuickJS线程安全**：QuickJS是单线程的，所有JS调用必须在脚本线程中执行
5. **模块版本**：当前绑定@minecraft/server 2.x版本，manifest中依赖声明需匹配
6. **【重要】QuickJS抽象合规**：modules/和binding/层代码已全部通过IScriptBindingContext抽象接口访问脚本能力，不再直接依赖QuickJS API。只有engine/目录下的实现文件允许`#include <quickjs.h>`。新代码必须遵循此原则。
