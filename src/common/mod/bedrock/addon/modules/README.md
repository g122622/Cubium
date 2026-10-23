# @minecraft/server模块绑定

实现基岩版@minecraft/server API的C++→JS绑定。

## 目录结构

```
modules/
├── MinecraftModuleFactory.hpp/.cpp      # 模块工厂，注册所有@minecraft/server绑定
├── ScriptCustomComponentBinding.hpp/.cpp # 自定义组件JS绑定（方块/物品组件注册）
├── ScriptEventBinding.hpp/.cpp          # 事件绑定（beforeEvents/afterEvents）
└── types/                               # 脚本类型包装类
    ├── ScriptColor.hpp/.cpp             # RGBA颜色类型（JS↔C++转换）
    ├── ScriptVec2.hpp/.cpp              # 2D向量类型
    ├── ScriptVec3.hpp/.cpp              # 3D向量/坐标类型
    └── ScriptWorldAccessor.hpp/.cpp     # world全局对象访问器接口
```

## 内部模块关系

```
MinecraftModuleFactory
    ├── 注册System/World/Dimension/Entity/Player/Block/ItemStack类到JS
    ├── 注册Entity组件类（RideableComponent/HealthComponent/MovementComponent/EquippableComponent/OnFireComponent）
    │   └── Entity.getComponent(componentId) 派发到对应组件类，opaque 持同一 mc::Entity*
    ├── 注册blockComponentRegistry/itemComponentRegistry全局对象
    │   └── ScriptCustomComponentBinding — 解析JS回调并注册到C++ Registry
    ├── 注册world.beforeEvents/afterEvents
    │   └── ScriptEventBinding — 创建事件信号对象并桥接到ScriptEventBus
    └── 使用types/中的类型进行JS↔C++值转换

ScriptWorldAccessor (单例)
    └── 抽象接口，由服务端注入实现，提供currentTick/getAllPlayerNames/sendMessage

types/
    ├── ScriptVec2/ScriptVec3 — 坐标转换，支持fromJs/toJs
    ├── ScriptColor — RGBA颜色转换
    └── ScriptWorldAccessor — world对象后端接口
```

## 外部依赖关系

### 依赖
- `../binding/` — NativeModuleBuilder、ClassRegistrar、ScriptObjectRegistry
- `../component/` — BlockComponentRegistry、ItemComponentRegistry
- `../event/` — ScriptEventBus（事件信号发布）
- `../engine/` — IScriptBindingContext（引擎抽象）
- `../lifecycle/` — ScriptScheduler（system.run/runInterval/runTimeout）
- `src/common/world/` — BlockPos、Vector3f/d（类型转换）
- `src/server/` — ServerScriptManager注入ScriptWorldAccessor实现

### 被依赖
- `src/server/mod/bedrock/addon/ServerScriptManager` — 初始化并调用MinecraftModuleFactory

## 容易踩的坑

- 所有类与原型从当前 IScriptBindingContext 的 classRegistry 查找，禁止复用其他插件的类注册结果。
- 定时及生成器调度由 system/ScriptSystemBinding 实现，参数句柄跨 tick 保存时必须复制。
- getComponent 的组件名、返回类型和写回行为必须同时符合目标 API，不能用普通属性快照替代可写组件。
- 模块内容以实际绑定和行为测试为准，不能根据类名导出推断完整 API 支持。

- 自定义组件注册表通过 system.beforeEvents.startup 的事件对象提供，各插件只持有本上下文的注册入口。脚本消息按命名空间过滤并在后置事件批次中分发。
