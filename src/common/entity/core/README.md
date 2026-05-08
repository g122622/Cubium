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

## 物理系统

### 移动与碰撞
- `Entity::moveWithCollision()` - 带碰撞检测的移动，自动处理步进
- `Entity::doBlockCollisions()` - 方块碰撞回调，在移动后触发 `onLanded` 和 `onEntityWalk`
- `Entity::isSteppingCarefully()` - 检测是否小心行走（潜行时返回true）
- `Entity::canTriggerWalking()` - 检测是否能触发行走事件

### 流体检测（MC 1.16.5 对齐）
- `Entity::isInWater()` / `Entity::isInLava()` - 是否在流体中
- `Entity::areEyesInWater()` / `Entity::areEyesInLava()` - 眼睛是否在流体中
- `Entity::canSwim()` - 是否可以游泳（眼睛在水中且在水中）
- `Entity::waterHeight()` / `Entity::lavaHeight()` - 流体浸入高度（0.0-1.0）
- `Entity::updateEnvironmentState()` - 更新流体状态，遍历碰撞箱内的方块

### 攀爬追踪（MC 1.16.5 对齐）
- `Entity::isOnLadder()` - 检测是否在攀爬方块上，并记录攀爬位置
- `Entity::getLastClimbPos()` - 获取最后攀爬位置（用于摔落死亡消息）
- `Entity::setLastClimbPos()` - 设置攀爬位置
- `Entity::clearLastClimbPos()` - 清空攀爬位置（落地时自动调用）
- 攀爬方块包括：梯子、藤蔓、脚手架、打开的活板门等

### 击退系统
- `LivingEntity::applyKnockback(strength, ratioX, ratioZ)` - 应用击退效果
- `LivingEntity::applyKnockbackFrom(attacker, strength)` - 从攻击者方向计算击退
- 击退抗性属性 `generic.knockback_resistance` 自动应用

### 姿态系统
- `Entity::setPose()` / `Entity::getPose()` - 姿态状态管理
- `Entity::refreshDimensions()` - 刷新尺寸和碰撞箱
- `Player::updatePose()` - 自动姿态判断（睡眠>游泳>潜行>站立）

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

## 传送系统

实体传送功能，支持安全传送和随机传送。参考 MC 1.16.5 `Entity.attemptTeleport` 和 `Entity.randomTeleport`。

### 核心方法

```cpp
class Entity {
public:
    // 安全传送到指定坐标
    bool attemptTeleport(f64 x, f64 y, f64 z, bool playEffects = true);
    
    // 在范围内随机传送
    bool randomTeleport(f64 range, bool playEffects = true, bool avoidFluid = true);
    
    // 查找安全传送位置
    std::optional<Vector3d> findSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;
    
    // 检查位置是否安全可传送
    bool isSafeTeleportPosition(f64 x, f64 y, f64 z, bool avoidFluid = true) const;
};
```

### attemptTeleport - 安全传送

传送到指定坐标，包含完整的安全检查：

1. **碰撞检测**：目标位置必须有足够空间容纳实体碰撞箱
2. **流体检查**：可选择避开水和岩浆（`avoidFluid` 参数）
3. **地面查找**：从指定 Y 坐标向下查找第一个安全地面
4. **音效播放**：可选择播放传送音效（`playEffects` 参数）

```cpp
// 末影人传送到目标附近
bool success = entity.attemptTeleport(targetX, targetY, targetZ, true);

// 玩家使用命令传送（不需要音效）
player.attemptTeleport(x, y, z, false);
```

### randomTeleport - 随机传送

在以实体为中心的立方体范围内随机寻找安全位置传送：

1. **范围定义**：`range` 参数定义传送半径（紫颂果 16.0，末影人 32.0）
2. **尝试次数**：最多 16 次尝试寻找安全位置
3. **避开水/岩浆**：`avoidFluid` 为 true 时会拒绝流体位置
4. **音效播放**：`playEffects` 控制是否播放传送音效

```cpp
// 紫颂果随机传送（16格范围）
bool success = player.randomTeleport(16.0, false, true);

// 末影人随机传送（32格范围）
bool success = enderman.randomTeleport(32.0, true, true);
```

### 传送算法细节

参考 MC 1.16.5 实现：

- **位置采样**：在 `[x-range, x+range] × [y-8, y+8] × [z-range, z+range]` 范围内随机采样
- **地面查找**：从采样点向下遍历，找到第一个非空气方块
- **安全检查**：检查碰撞箱是否与方块碰撞、是否在流体中
- **传送执行**：更新实体位置、重置运动向量、触发世界事件

### 使用示例

```cpp
// 紫颂果使用（ChorusFruitItem）
bool teleported = entity.randomTeleport(16.0, false, true);
if (teleported) {
    world.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT, ...);
}

// 末影人传送
bool success = enderman.teleport();  // 使用内置冷却
bool success = enderman.teleportToTarget();  // 传送到目标远离方向

// 安全传送位置查找
auto safePos = entity.findSafeTeleportPosition(x, y, z, true);
if (safePos.has_value()) {
    entity.setPosition(safePos.value());
}
```

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

// 应用击退
if (auto* living = dynamic_cast<LivingEntity*>(target)) {
    living->applyKnockbackFrom(attacker, 1.0f);
}
```

## 依赖关系

- `core/Types.hpp` - 基础类型定义
- `entity/attribute/` - 属性系统
- `entity/damage/` - 伤害系统
- `world/IWorld.hpp` - 世界级声音和位置查询入口
- `world/block/Block.hpp` - 方块交互回调
- `physics/PhysicsEngine.hpp` - 物理引擎
- `entity/ai/` - AI系统

## 测试用例

- [tests/entity/LivingEntityTests.cpp](../../../../tests/entity/LivingEntityTests.cpp) 验证受伤、死亡和环境声发声链路。
- [tests/common/entity/PlayerMovementTest.cpp](../../../../tests/common/entity/PlayerMovementTest.cpp) 验证玩家受伤和死亡时的声音事件。
- [tests/common/entity/PlayerSwimTest.cpp](../../../../tests/common/entity/PlayerSwimTest.cpp) 验证游泳、溺水、空气供应、效果影响等。
- [tests/common/test_entity_physics.cpp](../../../../tests/common/test_entity_physics.cpp) 验证重力、击退、滑度等物理常量。
