# 基岩版Addon兼容模组系统

本目录实现了完整兼容基岩版Addon的模组系统，使用户可以直接使用现有的基岩版行为包（Behavior Pack）。

## 目录结构

```
addon/
├── core/                          # 脚本引擎核心抽象层
│   ├── Capabilities.hpp           # 脚本能力定义
│   ├── IScriptContext.hpp         # 脚本上下文接口
│   ├── IScriptEngine.hpp          # 脚本引擎接口
│   ├── IScriptRuntime.hpp         # 脚本运行时接口
│   ├── ModuleDependency.hpp       # 模块依赖定义
│   ├── ModuleDescriptor.hpp       # 模块描述符
│   ├── Privilege.hpp              # 权限定义
│   ├── ScriptData.hpp             # 脚本数据类型
│   ├── ScriptException.hpp        # 脚本异常
│   └── ScriptResult.hpp           # 脚本结果类型
├── engine/                        # QuickJS引擎实现
│   └── quickjs/                   # QuickJS后端
│       ├── QuickJSBindingContext.cpp/hpp   # 绑定上下文实现
│       ├── QuickJSContext.cpp/hpp          # 上下文实现
│       ├── QuickJSEngine.cpp/hpp           # 引擎实现
│       ├── QuickJSModuleLoader.cpp/hpp     # 模块加载器
│       ├── QuickJSRuntime.cpp/hpp          # 运行时实现
│       └── QuickJSValueConvert.hpp         # 值转换工具
├── binding/                       # 模块绑定框架（C++→JS桥接）
│   ├── IModuleBindingFactory.hpp  # 模块绑定工厂接口
│   ├── IScriptBindingContext.hpp  # 引擎无关的绑定接口（重要）
│   ├── ScriptCallbackHolder.hpp   # 回调持有器
│   └── ScriptClassBinding.cpp/hpp # 类绑定框架
├── component/                     # 自定义组件系统
│   ├── BlockComponentRegistry.cpp/hpp      # 方块组件注册
│   ├── ItemComponentRegistry.cpp/hpp       # 物品组件注册
│   ├── BlockComponentEvents.hpp            # 方块组件事件
│   ├── ItemComponentEvents.hpp             # 物品组件事件
│   └── CustomComponentParameters.hpp       # 组件参数定义
├── modules/                       # @minecraft/server 模块绑定实现
│   ├── MinecraftModuleFactory.cpp/hpp      # 模块工厂
│   ├── ScriptEventBinding.cpp/hpp          # 事件绑定
│   ├── ScriptCustomComponentBinding.cpp/hpp # 自定义组件绑定
│   └── types/                     # 类型绑定
│       ├── ScriptVec2.cpp/hpp              # Vec2类型
│       ├── ScriptVec3.cpp/hpp              # Vec3类型
│       ├── ScriptColor.cpp/hpp             # Color类型
│       └── ScriptWorldAccessor.cpp/hpp     # 世界访问器
├── plugin/                        # 插件管理
│   ├── ScriptPlugin.cpp/hpp                # 插件实例
│   ├── ScriptPluginManager.cpp/hpp         # 插件管理器
│   ├── ScriptPluginSource.cpp/hpp          # 插件来源
│   ├── ScriptPackConfiguration.cpp/hpp     # 包配置
│   ├── ScriptPackPermissions.cpp/hpp       # 包权限
│   └── PluginExecutionGroup.cpp/hpp        # 执行组
├── pack/                          # 行为包系统
│   ├── BehaviorPack.cpp/hpp                # 行为包
│   ├── BehaviorPackList.cpp/hpp            # 行为包列表
│   ├── AddonManifest.cpp/hpp               # manifest解析
│   ├── AddonModule.cpp/hpp                 # 模块定义
│   ├── AddonDependency.cpp/hpp             # 依赖定义
│   ├── PackDependencyResolver.cpp/hpp      # 依赖解析
│   └── PackVersion.cpp/hpp                 # 版本解析
├── event/                         # 脚本事件桥接
│   ├── ScriptEventBus.cpp/hpp              # 脚本事件总线
│   ├── BeforeEventSignal.cpp/hpp           # 前置事件信号（可取消）
│   └── AfterEventSignal.cpp/hpp            # 后置事件信号（延迟批量）
├── lifecycle/                     # 生命周期管理
│   ├── ScriptManager.cpp/hpp               # 脚本管理器
│   ├── ScriptScheduler.cpp/hpp             # 调度器
│   ├── ScriptTickListener.cpp/hpp          # Tick监听器
│   ├── ScriptWatchdog.cpp/hpp              # 看门狗
│   └── ScriptLogger.cpp/hpp                # 日志器
└── diagnostics/                   # 诊断与调试
    ├── ScriptDiagnostics.cpp/hpp           # 诊断工具
    └── ScriptSentryLogger.cpp/hpp          # Sentry日志器
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                      ScriptManager                          │
│  （顶层管理器，协调所有组件的生命周期）                      │
└─────────────────────────────────────────────────────────────┘
          │
          ├──→ plugin/ScriptPluginManager（插件加载与管理）
          │           └──→ pack/（行为包解析、依赖解析）
          │
          ├──→ core/IScriptEngine → engine/quickjs/（引擎实例）
          │           └──→ core/IScriptContext（脚本上下文）
          │
          ├──→ modules/（@minecraft/server API绑定）
          │           └──→ binding/（通过IScriptBindingContext注册）
          │
          ├──→ component/（自定义方块/物品组件注册）
          │
          ├──→ event/ScriptEventBus（事件分发）
          │           ├──→ BeforeEventSignal（同步可取消）
          │           └──→ AfterEventSignal（延迟批量）
          │
          └──→ lifecycle/（Tick驱动、调度、看门狗）
                      ├──→ ScriptScheduler（system.run/Interval/Timeout）
                      ├──→ ScriptTickListener（每tick执行）
                      └──→ ScriptWatchdog（超时监控）
```

**关键依赖链：**
- binding/提供引擎无关的抽象接口（IScriptBindingContext），modules/和component/通过它注册JS API
- engine/quickjs/是唯一允许直接引用`<quickjs.h>`的地方
- event/独立于引擎实现，只处理事件分发逻辑

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
