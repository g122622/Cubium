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

1. **Stub陷阱**：各 JS 类的补全进度不一，调用前须确认目标成员已实现：
   - 已实现：System（run/runInterval/runTimeout/clearRun/currentTick）、World（sendMessage/getAllPlayers）、Dimension（getEntities/id）、Entity（id/typeId/getLocation/getDimension/getComponent[派发 rideable/health/movement/equippable/onfire]）、Player（name）、ItemStack（typeId/amount getter）、RideableComponent（addRider）、HealthComponent（currentValue/effectiveMax/setCurrentValue/resetToMaxValue 等）、MovementComponent（currentValue/setCurrentValue/resetToDefaultValue 等）、EquippableComponent（getEquipment/setEquipment 仅清空）、OnFireComponent（onFireTicksRemaining）
   - 仍 stub：World.getDimension（返 undefined）、Player 其他成员、Block（空壳）、ItemStack.amount setter、EquippableComponent.setEquipment（仅清空槽位，传 ItemStack 对象抛 TypeError）、EquippableComponent.totalArmor/totalToughness（返 0）、Health/Movement 的 effectiveMin/defaultValue（保守硬编码）
2. **对象生命周期**：JS对象持有的C++游戏对象指针可能在tick间失效（EntityManager 经 graveyard 延迟析构 Entity）。当前包装类（ScriptVec3/ScriptColor）使用值语义；Entity/Dimension/RideableComponent/HealthComponent/MovementComponent/EquippableComponent/OnFireComponent 均 opaque 持裸 `mc::Entity*`（owned=false），同受此悬垂风险约束，未来需弱引用或ID引用统一解决。仅 ItemStack 在 Equippable.getEquipment 返回时以 owned=true 拷贝（JS GC 时 delete），规避装备数组改写致引用悬垂
3. **事件桥接未连接**：ScriptEventBinding已实现，但ServerEventBus→ScriptEventBus的桥接未完成，脚本无法订阅游戏事件
4. **模块版本**：绑定@minecraft/server 2.x版本，manifest中依赖声明需匹配
5. **基岩 vs Java componentId**：`minecraft:nameable`/`minecraft:collision_box`/`minecraft:gravity` 等是 Java 版/JSON 组件名，**不是基岩版合法 componentId**（基岩命名走 Entity.nameTag 属性、碰撞箱走 getAABB）。getComponent 仅派发基岩合法且有数据源的 componentId（rideable/health/movement/equippable/onfire），其他返 undefined
