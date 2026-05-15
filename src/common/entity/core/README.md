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
| `EntitySpawnPlacementRegistry.hpp` | 生成位置规则、SpawnReason 枚举 |
| `EntityUtils.hpp` | 模板型实体工具函数（搜索、距离） |
| `DataParameter.hpp` | 数据参数定义 |
| `MoverType.hpp` | 移动类型枚举 |
| `BoostHelper.hpp` | 可骑乘实体的鞍和加速管理（猪、炽足兽等） |

## SpawnReason 枚举（MC 1.16.5）

定义实体生成的各种原因，用于决定生成规则和实体初始化行为。

### 枚举值

定义在 `mc::world::spawn` 命名空间：

| 枚举值 | 说明 |
|--------|------|
| `Natural` | 自然生成（常规的自然刷新） |
| `ChunkGeneration` | 区块生成时放置实体 |
| `Spawner` | 刷怪笼生成 |
| `Structure` | 结构生成（如村民、掠夺者） |
| `Breeding` | 繁殖生成 |
| `MobSummons` | 被其他生物召唤（如恼鬼、铁傀儡） |
| `Jockey` | 骑乘生成（如小僵尸骑鸡） |
| `Event` | 游戏事件触发（如袭击、僵尸围城） |
| `Conversion` | 转化生成（如僵尸村民变村民） |
| `Reinforcement` | 僵尸增援 |
| `Trigger` | 特定条件触发（如骷髅陷阱马） |
| `Bucket` | 从水桶释放 |
| `SpawnEgg` | 刷怪蛋生成 |
| `Command` | /summon 命令生成 |
| `Dispenser` | 发射器使用刷怪蛋或水桶 |
| `Patrol` | 掠夺者巡逻队生成 |

### 辅助函数

```cpp
// 获取生成原因的名称字符串
const char* getSpawnReasonName(SpawnReason reason);
// 返回: "natural", "chunk_generation", "spawner" 等

// 根据名称字符串获取生成原因
SpawnReason getSpawnReasonByName(const std::string& name);
// 无效名称返回 SpawnReason::Natural
```

### 使用示例

```cpp
// 在实体创建时设置生成原因
SpawnedEntityData data("minecraft:pig", x, y, z, SpawnReason::ChunkGeneration);

// 从字符串解析（用于 NBT 反序列化）
SpawnReason reason = getSpawnReasonByName("spawn_egg");

// 序列化到字符串（用于 NBT 序列化）
const char* name = getSpawnReasonName(SpawnReason::Breeding);
```

### 参考

MC 1.16.5 `net.minecraft.entity.SpawnReason`

## 物理系统

### 步进高度系统（MC 1.16.5）

实体步进高度（stepHeight）决定实体可以自动走上多高的方块，无需跳跃。

**核心实现**：

```cpp
class Entity {
public:
    // 获取步进高度
    [[nodiscard]] virtual f32 stepHeight() const { return m_stepHeight; }
    
    // 设置步进高度
    void setStepHeight(f32 height) { m_stepHeight = height; }
    
protected:
    f32 m_stepHeight = 0.0f;  // 默认0.0f，LivingEntity设置为0.6f
};
```

**实体步高值**（参考 MC 1.16.5）：

| 实体类型 | stepHeight | 说明 |
|---------|------------|------|
| Entity（基类） | 0.0f | 默认无步进能力 |
| LivingEntity | 0.6f | 生物默认值，可走上台阶 |
| IronGolemEntity | 1.0f | 可走上完整方块 |
| AbstractHorseEntity | 1.0f | 马、驴、骡等 |
| EndermanEntity | 1.0f | 末影人 |
| DrownedEntity | 1.0f | 溺尸 |
| RavagerEntity | 1.0f | 劫掠兽 |
| TurtleEntity | 1.0f | 海龟 |
| ArmorStandEntity | 0.0f | 盔甲架无法步进 |

**骑乘系统动态步高**（IRideable）：

```cpp
// 玩家骑乘时
mount.setStepHeight(1.0f);  // 骑乘时步高增加到1.0

// 下马后
mount.setStepHeight(0.5f);  // 恢复到0.5
```

**使用示例**：

```cpp
// 在实体构造函数中设置步高
IronGolemEntity::IronGolemEntity(LegacyEntityType type, EntityId id)
    : GolemEntity(type, id)
{
    setStepHeight(1.0f);  // 铁傀儡可走上完整方块
}

// 运行时修改步高
entity.setStepHeight(0.5f);
```

**物理引擎集成**：

`PhysicsEngine::moveEntity()` 使用 `stepHeight` 参数实现自动步进：

```cpp
Vector3 actualMovement = physics->moveEntity(entityBox, desiredMovement, stepHeight());
```

步进逻辑参考 MC 1.16.5 `Entity.getAllowedMovement()`：
1. 当水平方向被阻挡且在地面（或下落）时触发
2. 先尝试向上移动 `stepHeight` 高度
3. 然后尝试水平移动
4. 最后下降回地面
5. 选择水平移动距离最远的策略

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

### 雨天检测（MC 1.16.5 对齐）
- `Entity::isInRain()` - 检查实体是否在雨中
- `Entity::isWet()` - 检查实体是否湿润（水中或雨中）

**`isInRain()` 实现细节**（参考 MC 1.16.5 Entity.isInRain()）：

检查实体是否在雨中，使用双位置检测：
1. **脚底位置**：`floor(position.x), floor(position.y), floor(position.z)`
2. **碰撞盒顶部位置**：`floor(position.x), floor(boundingBox.maxY), floor(position.z)`

只要任一位置可以降雨（`world.canRainAt(pos)` 返回 true），就认为实体在雨中。

**检测条件**：
- 世界存在且正在下雨（`world.isRaining()` 返回 true）
- 至少一个检测位置可以降雨（天空可见、生物群系允许降水、温度足够高）

**使用示例**：
```cpp
// 末影人在雨中受到伤害
if (isInWaterOrRain()) {
    auto damageSource = DamageSources::drown();
    hurt(damageSource, WATER_DAMAGE);
    teleportAwayFromWater();
}

// 狼湿润时毛发颜色变化
if (isWet()) {
    // 渲染湿润效果
}
```

**水敏感生物**：
- 末影人（Enderman） - 水和雨中受到伤害
- 烈焰人（Blaze） - 水中受到伤害
- 雪傀儡（SnowGolem） - 水和雨中融化

### 火焰系统（MC 1.16.5 对齐）
- `Entity::isOnFire()` - 检查实体是否着火（`m_fire > 0`）
- `Entity::fire()` - 获取当前火焰计时器值（tick）
- `Entity::getFireTimer()` - 获取火焰计时器（与 `fire()` 功能相同，MC 命名方式）
- `Entity::setFire(i32 seconds)` - 设置燃烧时间（秒转 tick，只增不减）
- `Entity::forceFireTicks(i32 ticks)` - 强制设置火焰计时器值（直接设置，不检查当前值）
- `Entity::isImmuneToFire()` - 检查实体是否免疫火焰（查询 EntityType 标志）

**火焰机制**：
- `m_fire` 成员变量存储火焰计时器（tick 数）
- 每tick在 `Entity::baseTick()` 中自动递减（在水中/岩浆中立即归零）
- `setFire(seconds)` 将秒转换为 tick（×20），只在当前值较小时更新
- `forceFireTicks(ticks)` 直接设置值，用于增加/减少火焰时间
- 火焰免疫由 `EntityType::immuneToFire()` 标志决定，子类可重写 `isImmuneToFire()` 提供运行时可变状态

**火焰碰撞处理**（FireBlock::onEntityCollision）：
1. 检查 `isImmuneToFire()` - 免疫实体跳过
2. 增加 `fireTimer` (+1) - 每次碰撞 tick
3. 如果 `fireTimer == 0`，调用 `setFire(8)` - 点燃 8 秒
4. 对 LivingEntity 造成 `m_fireDamage` 点火焰伤害

**免疫火焰的实体**：
- 烈焰人（Blaze）
- 恶魂（Ghast）
- 岩浆怪（Magma Cube）
- 猪灵及其变种（Piglin, Zombified Piglin, Piglin Brute）
- 疣猪兽（Hoglin）
- 潜影贝（Shulker）
- 末影龙、凋灵（Boss 实体）

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

### 箭矢计数系统（MC 1.16.5）

生物实体的箭矢计数系统，用于渲染插在身上的箭矢。

**核心方法**：

```cpp
class LivingEntity {
public:
    // 获取插在身上的箭矢数量
    [[nodiscard]] i32 getArrowCount() const;
    
    // 设置箭矢数量（网络同步）
    void setArrowCountInEntity(i32 count);
    
    // 更新箭矢自动脱落逻辑（每 tick 调用）
    void tickArrows();
};
```

**箭矢计数规则**：
- 箭矢命中生物时增加计数（仅非穿透箭）
- 箭矢数量越多，脱落越快
- 脱落计时器公式：`20 * (30 - arrowCount)` ticks
  - 1 支箭：580 ticks（约 29 秒）
  - 15 支箭：300 ticks（15 秒）

**数据参数**：
- `ARROW_COUNT_PARAM`（DataParameter ID 13）用于网络同步

**使用示例**：

```cpp
// 箭矢命中时增加计数
if (livingTarget != nullptr) {
    bool hurt = livingTarget->hurt(*damageSource, damage);
    if (hurt && m_pierceLevel <= 0) {
        livingTarget->setArrowCountInEntity(livingTarget->getArrowCount() + 1);
    }
}
```

**ArrowLayer 渲染**：
- 渲染层通过 `getArrowCount()` 获取箭矢数量
- 最多渲染 10 支箭矢，随机分布在实体身上
- 箭矢位置和旋转基于实体 ID 确定性随机

### 发光效果系统（MC 1.16.5）

实体的发光状态系统，用于发光药水效果和团队发光规则。

**核心方法**：

```cpp
class Entity {
public:
    // 检查实体是否发光
    // 客户端检查数据参数标志位，服务端检查 m_glowing 字段
    [[nodiscard]] bool isGlowing() const;
    
    // 设置发光状态（服务端设置字段并同步标志位）
    void setGlowing(bool glowing);
};
```

**发光效果来源**：
1. 发光药水效果（`EffectType::Glowing`）
2. Entity 的发光标志（由 `setGlowing()` 设置）
3. 发光鱿鱼实体类型（未实现）
4. 团队发光规则（未实现）

**使用示例**：

```cpp
// GlowEffect 中检查发光状态
bool hasGlowEffect = entity.isGlowing();
if (auto* living = dynamic_cast<LivingEntity*>(&entity)) {
    if (living->hasEffect(entity::effect::EffectType::Glowing)) {
        hasGlowEffect = true;
    }
}
```

**数据参数**：
- `EntityFlags::Glowing`（第 6 位）用于网络同步发光状态

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

## LegacyEntityType 枚举

实体旧版类型标识符，用于网络同步和实体类型检查。

### 恼鬼 (Vex) 类型

恼鬼 (`LegacyEntityType::Vex`) 是唤魔者召唤的小型飞行敌对生物：

```cpp
enum class LegacyEntityType : u32 {
    // ...
    Vex = 81,  // 恼鬼
    // ...
};
```

**用途**:
- `EvokerSummonSpellGoal::countNearbyVexes()` 使用此类型统计周围恼鬼数量
- `VexEntity::create()` 使用此类型创建恼鬼实体
- 网络同步中标识实体类型

**参考**: MC 1.16.5 `net.minecraft.entity.EntityType.VEX`

## 声音事件链路

- `IWorld::playSound(...)` 是世界级声音出口，实体不会直接碰网络层。
- `Entity::playSound(...)` 负责把声音事件转发给当前世界，并自动附带实体位置和声音分类。
- `LivingEntity` 统一提供受伤声、死亡声、音量和音高，减少各个生物重复实现。
- `MobEntity` 统一提供环境声播放入口，`getTalkInterval()` 和 `playAmbientSound()` 负责控制闲置发声节奏。
- `Player` 也走同一条声音链路，受伤和死亡会通过 `makeSoundEventId(...)` 发出对应事件。
- `ServerWorld` 可以挂接声音回调，把事件继续交给 `MinecraftServer` 的广播接口。

## 水花溅射效果

实体入水时的水花效果系统，参考 MC 1.16.5 `Entity.doWaterSplashEffect()`。

### 核心方法

```cpp
class Entity {
public:
    // 获取溅水声音（子类可覆盖）
    [[nodiscard]] virtual ResourceLocation getSplashSound() const;

    // 获取高速溅水声音（子类可覆盖）
    [[nodiscard]] virtual ResourceLocation getHighspeedSplashSound() const;

    // 执行水花溅射效果
    virtual void doWaterSplashEffect();
};
```

### 速度因子计算

速度因子 f1 决定水花强度和声音选择：

```cpp
// f1 = sqrt(vx² × 0.2 + vy² + vz² × 0.2) × 0.2
f32 f1 = std::sqrt(vx * vx * 0.2f + vy * vy + vz * vz * 0.2f) * 0.2f;
f1 = std::min(f1, 1.0f);  // 限制在 [0, 1]
```

- 水平速度权重：0.2（降低水平速度影响）
- 垂直速度权重：1.0（保留完整影响）

### 声音选择

| 速度因子 | 声音类型 |
|---------|---------|
| f1 < 0.25 | 普通溅水声 (`getSplashSound()`) |
| f1 >= 0.25 | 高速溅水声 (`getHighspeedSplashSound()`) |

音量使用 f1，音调随机化：`1.0 + (rand - rand) × 0.4`

### 粒子生成

| 粒子类型 | 数量公式 | 位置 | 速度 |
|---------|---------|------|------|
| Bubble | `1 + width × 20` | 包围盒内随机，Y=floor(posY)+1 | `(vx, vy - rand(0,0.2), vz)` |
| Splash | `1 + width × 20` | 包围盒内随机，Y=floor(posY)+1 | `(vx, vy, vz)` |

### Player 覆盖

```cpp
class Player {
public:
    // 返回玩家专用溅水声音
    [[nodiscard]] ResourceLocation getSplashSound() const override {
        return SoundEvents::ENTITY_PLAYER_SPLASH;
    }

    [[nodiscard]] ResourceLocation getHighspeedSplashSound() const override {
        return SoundEvents::ENTITY_PLAYER_SPLASH_HIGH_SPEED;
    }

    // 观察者模式不产生水花效果
    void doWaterSplashEffect() override {
        if (isSpectator()) return;
        Entity::doWaterSplashEffect();
    }
};
```

### 使用示例

```cpp
// 在 Player::updateAirSupply() 中检测入水
void Player::updateAirSupply() {
    bool inWater = isInWater();
    bool justEnteredWater = inWater && !m_wasInWater;

    LivingEntity::updateAirSupply();

    if (justEnteredWater) {
        doWaterSplashEffect();  // 触发水花效果
    }

    m_wasInWater = inWater;
}
```

### 测试用例

- [tests/common/entity/WaterSplashTest.cpp](../../../../tests/common/entity/WaterSplashTest.cpp) 验证粒子生成、声音播放、速度因子计算、玩家观察者模式等。

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

## 玩家交互系统

实体与玩家交互的核心方法，支持右键点击交互。

### 核心方法

```cpp
class Entity {
public:
    /**
     * @brief 处理玩家初始交互
     *
     * 当玩家右键点击实体时首先调用此方法。
     * 子类可重写此方法处理特定的交互行为（如骑乘、打开容器等）。
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手（主手/副手）
     * @return 交互结果类型
     */
    virtual ActionResultType processInitialInteract(Player& player, Hand hand);

    /**
     * @brief 处理玩家指定位置的交互
     *
     * 当玩家右键点击实体的特定位置时调用。
     * hitPosition 是相对于实体坐标的局部坐标，用于确定点击的是实体的哪个部位。
     * 子类可重写此方法处理基于点击位置的交互（如盔甲架装备槽）。
     *
     * @param player 与此实体交互的玩家
     * @param hitPosition 点击位置（相对于实体坐标系）
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    virtual ActionResultType applyPlayerInteraction(Player& player,
        const Vector3& hitPosition, Hand hand);
};
```

### 返回值类型

```cpp
enum class ActionResultType : u8 {
    Success = 0,  // 成功执行，消耗物品
    Consume = 1,  // 消耗物品但不执行动作
    Fail = 2,     // 执行失败，不消耗物品
    Pass = 3      // 传递给下一个处理器
};
```

### 交互流程

当玩家右键点击实体时，服务端处理流程：

1. **Interact 动作**（无位置信息）：
   ```
   客户端发送 UseEntityPacket(Interact)
   → PacketHandler 调用 player->interactOn(target, hand)
   → interactOn 先调用 target.processInitialInteract()
   → 如果返回 Pass，则尝试物品交互 item->itemInteractionForEntity()
   ```

2. **InteractAt 动作**（有位置信息）：
   ```
   客户端发送 UseEntityPacket(InteractAt, hitX, hitY, hitZ)
   → PacketHandler 调用 target.applyPlayerInteraction(player, hitPosition, hand)
   → 子类根据 hitPosition 执行特定交互
   ```

### 基类默认行为

Entity 基类的默认实现：

```cpp
ActionResultType Entity::processInitialInteract(Player& player, Hand hand)
{
    // 基类默认返回 Pass，表示不处理交互
    (void)player;
    (void)hand;
    return ActionResultType::Pass;
}

ActionResultType Entity::applyPlayerInteraction(Player& player,
    const Vector3& hitPosition, Hand hand)
{
    // 基类默认调用 processInitialInteract
    (void)hitPosition;
    return processInitialInteract(player, hand);
}
```

### 子类重写示例

**ArmorStandEntity**（盔甲架）重写 `applyPlayerInteraction`：

```cpp
ActionResultType ArmorStandEntity::applyPlayerInteraction(
    Player& player, const Vector3& hitPosition, Hand hand)
{
    // 标记模式下的盔甲架不能交互
    if (hasMarker()) {
        return ActionResultType::Pass;
    }

    // 观察者模式
    if (player.isSpectator()) {
        return ActionResultType::Success;
    }

    // 获取点击的装备槽位（基于 hitPosition.y）
    EquipmentSlot slot = getClickedSlot(hitPosition);
    ItemStack itemStack = player.getHeldItem(hand);

    // 执行装备放置或交换
    return equipOrSwap(player, slot, itemStack, hand);
}
```

### 使用场景

| 方法 | 使用场景 |
|------|---------|
| `processInitialInteract` | 骑乘（船、马）、打开容器（矿车）、驯服动物 |
| `applyPlayerInteraction` | 盔甲架装备槽交互（基于点击位置） |

### 测试用例

- [tests/entity/EntityCoreTests.cpp](../../../../tests/entity/EntityCoreTests.cpp) 验证方法签名、虚拟方法和多态行为。

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
