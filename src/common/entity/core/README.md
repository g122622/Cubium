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
| `BoostHelper.hpp` | 可骑乘实体的鞍和加速管理（猪、炽足兽等） |

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

### 挥动动画系统
- `LivingEntity::swing(Hand)` - 触发手臂挥动动画
- `LivingEntity::swingingHand()` - 获取当前挥动的手（MainHand/OffHand）
- `LivingEntity::getArmSwingAnimationEnd()` - 获取挥动动画时长（tick 数）
  - 基础值：6 tick
  - 急迫效果：减少动画时长 (1 + hasteLevel)
  - 挖掘疲劳效果：增加动画时长 ((1 + fatigueLevel) * 2)
  - 最小值：1 tick

### 姿态系统
- `Entity::setPose()` / `Entity::getPose()` - 姿态状态管理
- `Entity::refreshDimensions()` - 刷新尺寸和碰撞箱
- `Player::updatePose()` - 自动姿态判断（鞘翅飞行>游泳>激流攻击>睡眠>潜行>站立）

### 鞘翅飞行与激流攻击（MC 1.16.5）
- `Entity::isElytraFlying()` - 检查实体是否正在鞘翅飞行（检查 `EntityFlags::FallFlying` 标志）
- `LivingEntity::isSpinAttacking()` - 检查实体是否正在进行三叉戟激流攻击
- `LivingEntity::startSpinAttack(i32 duration)` - 开始激流攻击，设置持续时间
- `LivingEntity::stopSpinAttack()` - 停止激流攻击，清除状态
- `LivingEntity::updateSpinAttack()` - 更新激流攻击（每 tick 调用，自动递减持续时间）
- `LivingEntity::spinAttackDuration()` - 获取剩余攻击持续时间

**激流攻击实现细节：**
- 使用 `LIVING_FLAGS_PARAM`（DataParameter ID 10）的第 2 位（0x04）存储状态
- 三叉戟激流攻击持续 20 tick（1 秒）
- 每 tick 自动递减持续时间，归零时自动停止
- `Player::updatePose()` 中激流攻击姿态优先级仅次于鞘翅飞行

## 尺寸与碰撞箱

- `EntitySize` 现在同时保存宽度、高度和眼睛高度，并提供碰撞箱构造帮助。
- `Entity` 会缓存当前的 `EntitySize` 和 `AxisAlignedBB`，姿态、尺寸状态或位置变化后需要通过 `refreshDimensions()` 重新计算。
- 运行时会改变体型的实体子类，应在尺寸变化后立即刷新碰撞箱，避免旧 AABB 继续参与物理计算。
- `Player` 的姿态切换会先检查目标碰撞箱是否能放下，再决定是否真正站立，避免在低顶空间里错误穿模。

## 鞍与加速系统（BoostHelper）

可骑乘实体（如猪、炽足兽）的鞍管理和加速辅助系统。参考 MC 1.16.5 `BoostHelper`。

### 核心类

```cpp
class BoostHelper {
public:
    // 初始化
    void init(EntityDataManager& manager,
              DataParameter<i32> boostTimeParam,
              DataParameter<bool> saddledParam);

    // 鞍状态
    void setSaddledFromBoolean(bool saddled);
    [[nodiscard]] bool getSaddled() const;

    // 加速功能
    template<typename Random>
    bool boost(Random& rng);  // 触发加速，返回是否成功
    bool tick();              // 每 tick 调用，返回是否仍在加速
    [[nodiscard]] bool isBoosting() const;

    // NBT 序列化
    void writeToNbt(nbt::tags::compound_tag& tag) const;
    void readFromNbt(const nbt::tags::compound_tag& tag);

    // 公开成员（与 MC 保持一致）
    bool saddledRaw = false;     // 原始鞍状态（加速中时为 true）
    i32 field_233611_b_ = 0;     // 当前加速 tick
    i32 boostTimeRaw = 0;        // 总加速时间
};
```

### 加速机制

1. **触发加速**：`boost(rng)` 方法
   - 仅当 `saddledRaw == false` 时可触发
   - 随机生成加速时间：`rand.nextInt(841) + 140` → [140, 980] tick
   - 设置 `saddledRaw = true`，重置 `field_233611_b_ = 0`
   - 同步加速时间到 `EntityDataManager`

2. **Tick 更新**：`tick()` 方法
   - 递增 `field_233611_b_`
   - 当 `field_233611_b_ > boostTimeRaw` 时，结束加速并返回 false

3. **状态判断**：`isBoosting()` 方法
   - 返回 `saddledRaw && field_233611_b_ <= boostTimeRaw`
   - 边界值：加速最后一刻仍返回 true

### NBT 序列化

**只持久化鞍状态，加速状态不保存**（MC 1.16.5 行为）：

```cpp
// 写入 NBT
void writeToNbt(nbt::tags::compound_tag& tag) const {
    // NBT 无布尔类型，使用 byte_tag 存储
    tag.put("Saddle", static_cast<i8>(getSaddled() ? 1 : 0));
}

// 读取 NBT
void readFromNbt(const nbt::tags::compound_tag& tag) {
    auto it = tag.value.find("Saddle");
    if (it != tag.value.end() && it->second->id() == nbt::TagId::Byte) {
        i8 value = dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value;
        setSaddledFromBoolean(value != 0);
    }
    // 加速状态重置为默认值（未加速）
}
```

### 客户端同步

客户端通过 `syncFromDataManager()` 从 `EntityDataManager` 读取数据：

```cpp
void syncFromDataManager() {
    saddledRaw = true;
    field_233611_b_ = 0;
    boostTimeRaw = m_manager->get(m_boostTimeParam);
}
```

### 使用示例

```cpp
// 在 PigEntity 中使用
class PigEntity : public AnimalEntity {
    BoostHelper m_boostHelper;

    void registerData() override {
        auto boostTimeParam = EntityDataManager::createKey<i32>();
        auto saddledParam = EntityDataManager::createKey<bool>();
        m_dataManager.registerParam(boostTimeParam, 0);
        m_dataManager.registerParam(saddledParam, false);
        m_boostHelper.init(m_dataManager, boostTimeParam, saddledParam);
    }

    void tick() override {
        AnimalEntity::tick();
        if (m_boostHelper.isBoosting()) {
            m_boostHelper.tick();
        }
    }

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) override {
        AnimalEntity::addAdditionalSaveData(tag);
        m_boostHelper.writeToNbt(tag);
    }

    void readAdditionalSaveData(const nbt::tags::compound_tag& tag) override {
        AnimalEntity::readAdditionalSaveData(tag);
        m_boostHelper.readFromNbt(tag);
    }
};
```

### 测试用例

- [tests/common/entity/core/BoostHelperTest.cpp](../../../../tests/common/entity/core/BoostHelperTest.cpp) 验证初始化、鞍状态、加速、tick、NBT 读写、往返测试等。

## 实体生命周期

- `Entity::remove()` - 标记实体为已移除状态（虚函数，子类可重写以实现自定义逻辑，如史莱姆分裂）
- `Entity::isRemoved()` - 检查实体是否已被移除
- `Entity::onKillCommand()` - 由 /kill 命令调用，默认实现调用 remove()，LivingEntity 重写为使用虚空伤害
- `LivingEntity::die()` - 处理实体死亡（触发掉落、分数等）
- `LivingEntity::onKillCommand()` - 重写为使用 `Float.MAX_VALUE` 的虚空伤害杀死实体，确保完整死亡流程
- `MobEntity::dropExperience()` - 掉落经验球

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

## 空气供应与溺水系统

生物实体的空气管理和溺水伤害系统，参考 MC 1.16.5 `LivingEntity` 实现。

### 核心方法

```cpp
class LivingEntity {
public:
    // 获取当前空气供应量
    [[nodiscard]] i32 air() const;

    // 设置空气供应量
    void setAir(i32 air);

    // 获取最大空气供应量（默认 300 tick = 15 秒）
    [[nodiscard]] virtual i32 maxAir() const;

    // 检查是否可以水下呼吸
    // 亡灵生物（僵尸、骷髅等）返回 true
    [[nodiscard]] virtual bool canBreatheUnderwater() const;

    // 减少空气供应量（考虑水下呼吸附魔）
    // 返回减少后的空气值
    [[nodiscard]] i32 decreaseAirSupply(i32 currentAir);

    // 计算下一 tick 的空气值（恢复）
    // 返回 min(currentAir + 4, maxAir())
    [[nodiscard]] i32 determineNextAir(i32 currentAir) const;

    // 更新空气供应（每 tick 调用）
    // 处理空气消耗、恢复、溺水伤害
    virtual void updateAirSupply();

protected:
    i32 m_drownDamageTimer = 0;  // 溺水伤害计时器
};
```

### 溺水机制（MC 1.16.5）

1. **空气消耗**：
   - 在水中或岩浆中且不能水下呼吸时，每 tick 调用 `decreaseAirSupply()` 减少 1 点空气
   - 水下呼吸附魔（Respiration）有 `level/(level+1)` 概率不消耗空气
   - 空气值可以从正数变成负数（用于溺水计时）

2. **空气恢复**：
   - 不在水中/岩浆中或可以水下呼吸时，每 tick 恢复 4 点空气
   - 恢复上限为 `maxAir()`（默认 300）

3. **溺水伤害**：
   - 当空气值降到 -20 时重置为 0 并触发一次溺水伤害
   - 伤害间隔由 `DROWN_DAMAGE_INTERVAL`（20 tick）控制
   - 每次伤害量为 `DROWN_DAMAGE_AMOUNT`（2.0）
   - 伤害类型为 `DamageSources::drown()`，绕过护甲

### 特殊情况

- **亡灵生物**：`canBreatheUnderwater()` 返回 true，不会溺水
- **水下呼吸效果**：拥有 `WaterBreathing` 或 `ConduitPower` 效果时不消耗空气
- **玩家**：创造模式的 `invulnerable` 状态阻止溺水伤害
- **水生生物**：`WaterMobEntity` 使用反逻辑（在陆地上溺水，在水中恢复）

### 子类重写

```cpp
// Player::updateAirSupply() - 创造模式免疫
void Player::updateAirSupply() override {
    if (m_abilities.invulnerable) return;
    LivingEntity::updateAirSupply();
}

// WaterMobEntity::updateAirSupply() - 反逻辑
void WaterMobEntity::updateAirSupply() override {
    // 在水中恢复，在陆地上消耗空气
}

// AbstractFishEntity::maxAir() - 更长的空气储备
i32 AbstractFishEntity::maxAir() const override {
    return 480;  // 24 秒
}
```

## 日光检测系统

生物实体的日光检测功能，用于亡灵生物燃烧等机制。

### MobEntity::isInDaylight()

检查生物是否暴露在日光下，参考 MC 1.16.5 `MobEntity.isInDaylight()`。

**检测条件**：
1. 世界存在且不为客户端
2. 当前为白天（dayTime < 12000）
3. 实体位置亮度 > 0.5
4. 随机检查通过（亮度越高概率越大）
5. 天空可见（canSeeSky）

**船骑乘位置偏移**：
- 当生物骑乘船时，检测位置向上偏移一格
- 这是因为船在水面上，生物坐在船中位置较低
- 使用 `dynamic_cast<BoatEntity*>` 检测载具是否为船

```cpp
// MobEntity::isInDaylight() 实现
bool MobEntity::isInDaylight() const {
    if (m_world == nullptr || m_world->isClientSide()) {
        return false;
    }
    if (!m_world->isDaytime()) {
        return false;
    }
    f32 brightness = getBrightness();
    if (brightness <= 0.5f) {
        return false;
    }
    // 随机检查
    math::Random rng = getRandom();
    f32 randomCheck = rng.nextFloat() * 30.0f;
    f32 brightnessThreshold = (brightness - 0.4f) * 2.0f;
    if (randomCheck >= brightnessThreshold) {
        return false;
    }
    // 获取检测位置
    BlockPos pos(...);
    // 船骑乘时向上偏移
    if (isRiding()) {
        EntityId vehicleId = getVehicle();
        if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
            const Entity* vehicle = m_world->getEntity(vehicleId);
            if (vehicle != nullptr && dynamic_cast<const entity::BoatEntity*>(vehicle) != nullptr) {
                pos = pos.up();
            }
        }
    }
    return m_world->canSeeSky(pos);
}
```

### 骑乘系统改进

**canFitPassenger() 虚函数**：

`Entity::canFitPassenger()` 现在是虚函数，允许子类重写添加额外检查条件：

```cpp
// Entity 基类
[[nodiscard]] virtual bool canFitPassenger() const {
    return static_cast<i32>(m_passengers.size()) < getMaxPassengers();
}

// BoatEntity 重写
[[nodiscard]] bool canFitPassenger() const override {
    return static_cast<i32>(m_passengers.size()) < MAX_PASSENGERS 
           && m_status != BoatStatus::UnderWater;
}
```

这个改动确保通过基类引用调用 `canFitPassenger()` 时能正确调用子类的实现。

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
