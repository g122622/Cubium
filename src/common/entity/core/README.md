# Entity Core Module

实体系统的核心框架，包含所有实体的基类和基础设施。

## 核心类

| 类名 | 说明 | 参考 |
|------|------|------|
| `Entity` | 所有实体的基类 | MC Entity |
| `LivingEntity` | 有生命值的生物实体基类 | MC LivingEntity |
| `MobEntity` | 有AI的生物实体基类 | MC MobEntity |
| `CreatureEntity` | 陆地生物基类（有寻路） | MC CreatureEntity |
| `FlyingEntity` | 飞行生物基类 | MC FlyingEntity |

## 支持类

| 文件 | 说明 |
|------|------|
| `EntityType.hpp` | 实体类型定义 |
| `EntityRegistry.hpp` | 实体注册表 |
| `EntityDataManager.hpp` | 实体数据同步管理 |
| `EntityPose.hpp` | 实体姿态枚举 |
| `EntitySize.hpp` | 实体尺寸定义 |
| `EntityClassification.hpp` | 实体分类 |
| `EntitySpawnPlacementRegistry.hpp` | 生成位置规则 |
| `EntityUtils.hpp` | 模板型实体工具函数（搜索、距离） |
| `DataParameter.hpp` | 数据参数定义 |
| `MoverType.hpp` | 移动类型枚举 |

## 尺寸与碰撞箱

- `EntitySize` 现在同时保存宽度、高度和眼睛高度，并提供碰撞箱构造帮助。
- `Entity` 会缓存当前的 `EntitySize` 和 `AxisAlignedBB`，姿态、尺寸状态或位置变化后需要通过 `refreshDimensions()` 重新计算。
- 运行时会改变体型的实体子类，应在尺寸变化后立即刷新碰撞箱，避免旧 AABB 继续参与物理计算。
- `Player` 的姿态切换会先检查目标碰撞箱是否能放下，再决定是否真正站立，避免在低顶空间里错误穿模。

## 类型标识符同步

- `EntityType::create(...)` 会在工厂创建实体后自动注入注册表名称到实体（例如 `minecraft:pig`）。
- `Entity::getTypeId()` 优先返回显式注入的类型标识符；仅在未注入时才回退到 `LegacyEntityType` 映射。
- 通过繁殖流程创建幼体时，`BreedGoal` 会继承父体的类型标识符，避免网络层出现 `minecraft:unknown`。
- `LegacyEntityType -> typeId` 的具体映射表已经迁移到 `utils/EntityUtils.*`，`core/EntityUtils.hpp` 只保留模板型搜索和距离工具。

## 声音事件链路

- `IWorld::playSound(...)` 是世界级声音出口，实体不会直接碰网络层。
- `Entity::playSound(...)` 负责把声音事件转发给当前世界，并自动附带实体位置和声音分类。
- `LivingEntity` 统一提供受伤声、死亡声、音量和音高，减少各个生物重复实现。
- `MobEntity` 统一提供环境声播放入口，`getTalkInterval()` 和 `playAmbientSound()` 负责控制闲置发声节奏。
- `Player` 也走同一条声音链路，受伤和死亡会通过 `makeSoundEventId(...)` 发出对应事件。
- `ServerWorld` 可以挂接声音回调，把事件继续交给 `MinecraftServer` 的广播接口。

## 继承层次

```
Entity
├── LivingEntity (生命值、装备、药水效果)
│   ├── MobEntity (AI系统、目标选择、控制器)
│   │   ├── CreatureEntity (陆地移动、寻路)
│   │   │   ├── AgeableEntity (成长系统)
│   │   │   │   └── AnimalEntity (繁殖系统)
│   │   │   └── MonsterEntity (敌对行为)
│   │   └── FlyingEntity (飞行移动)
│   └── Player (玩家特有功能)
└── ItemEntity (掉落物)
```

## 命名空间

所有类定义在 `mc` 命名空间下。

## 使用示例

```cpp
// 创建实体
auto pig = EntityRegistry::create(EntityType::PIG, world);

// 访问实体属性
if (auto* living = dynamic_cast<LivingEntity*>(entity)) {
    living->heal(10.0f);
}

// AI系统
if (auto* mob = dynamic_cast<MobEntity*>(entity)) {
    mob->goalSelector().addGoal(1, std::make_unique<SwimGoal>(mob));
}
```

## 依赖关系

- `core/Types.hpp` - 基础类型定义
- `entity/attribute/` - 属性系统
- `entity/damage/` - 伤害系统
- `world/IWorld.hpp` - 世界级声音和位置查询入口
- `entity/ai/` - AI系统

## 测试用例

- [tests/entity/LivingEntityTests.cpp](../../../../tests/entity/LivingEntityTests.cpp) 验证受伤、死亡和环境声发声链路。
- [tests/common/entity/PlayerMovementTest.cpp](../../../../tests/common/entity/PlayerMovementTest.cpp) 验证玩家受伤和死亡时的声音事件。
