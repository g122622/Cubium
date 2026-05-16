# 灾厄村民模块 (Illager)

灾厄村民是 Minecraft 中的敌对生物类别，包括掠夺者、卫道士、唤魔者、幻术师、恼鬼、女巫和劫掠兽等。

## 目录结构

```
illager/
├── AbstractIllagerEntity.hpp/cpp   # 灾厄村民基类
├── AbstractRaiderEntity.hpp/cpp     # 袭击者基类
├── PatrollerEntity.hpp/cpp          # 巡逻者基类
├── SpellcastingIllagerEntity.hpp/cpp # 施法灾厄村民基类
├── EvokerEntity.hpp/cpp             # 唤魔者
├── IllusionerEntity.hpp/cpp         # 幻术师
├── IllagerEntities.hpp/cpp          # 掠夺者、卫道士
├── RavagerEntity.hpp/cpp            # 劫掠兽
├── VexEntity.hpp/cpp                # 恼鬼
├── WitchEntity.hpp/cpp              # 女巫
└── README.md
```

## 继承层次

```
MonsterEntity (敌对生物基类)
└── PatrollerEntity (巡逻者基类)
    └── AbstractRaiderEntity (袭击者基类)
        ├── AbstractIllagerEntity (灾厄村民基类)
        │   ├── SpellcastingIllagerEntity (施法灾厄村民基类)
        │   │   ├── EvokerEntity (唤魔者)
        │   │   └── IllusionerEntity (幻术师)
        │   ├── VindicatorEntity (卫道士)
        │   └── PillagerEntity (掠夺者)
        ├── WitchEntity (女巫)
        └── RavagerEntity (劫掠兽)

VexEntity (恼鬼) 独立继承自 MonsterEntity
```

## 实体列表

| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| AbstractIllagerEntity | 灾厄村民基类 | 手臂姿势状态、RAID 参与状态 | ✅ 完成 |
| VindicatorEntity | 卫道士 | 斧头近战攻击、冲向目标 | ✅ 完成 |
| EvokerEntity | 唤魔者 | 尖牙攻击、召唤恼鬼 | ✅ 完成 |
| IllusionerEntity | 幻术师 | 分身、失明攻击 | ⏳ 框架完成 |
| PillagerEntity | 掠夺者 | 弩远程攻击、RangedCrossbowAttackGoal | ✅ 完成 |
| RavagerEntity | 劫掠兽 | 冲撞攻击、破坏方块 | ⏳ 框架完成 |
| VexEntity | 恼鬼 | **穿墙飞行**、有限生命 | ✅ 完成 |
| WitchEntity | 女巫 | 药水攻击、喝药水治疗 | ✅ 完成 |

## PillagerEntity 详细实现

掠夺者是手持弩的灾厄村民，使用 `RangedCrossbowAttackGoal` 进行远程攻击。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.6f | 标准灾厄村民尺寸 |
| 高度 | 1.95f | MC 1.16.5 |
| 最大生命值 | 24.0 | MC 1.16.5 |
| 移动速度 | 0.35 | MC 1.16.5 |
| 攻击伤害 | 5.0 | MC 1.16.5 |
| 跟随范围 | 32.0 | MC 1.16.5 |
| 弩装填时间 | 25 ticks | 基础装填时间 |

### ICrossbowUser 接口实现

掠夺者实现了 `ICrossbowUser` 接口，支持弩的装填和发射：

```cpp
class PillagerEntity : public AbstractIllagerEntity, public ICrossbowUser {
public:
    // ICrossbowUser 接口
    void setChargingCrossbow(bool charging) override;
    [[nodiscard]] bool isChargingCrossbow() const override;
    void onCrossbowLoadComplete(ItemStack& crossbow) override;
    void shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge) override;
    [[nodiscard]] i32 getCrossbowChargeTime() const override;

    // IRangedAttackMob 接口
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override;
    [[nodiscard]] bool canRangedAttack() const override;
};
```

### 弩发射机制

`shootCrossbow()` 方法实现了弩箭发射逻辑：

| 参数 | 值 | 说明 |
|------|-----|------|
| 箭矢速度 | 3.15f | 普通箭矢 |
| 烟花速度 | 1.6f | 烟花火箭 |
| 不精确度 | 6.0f | 普通难度 |
| 弹道补偿 | `horizontalDist * 0.2` | 抛物线计算 |
| 目标高度偏移 | `height * 0.333` | 瞄准目标身体 |

```cpp
void PillagerEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge) {
    // 计算弹道
    f64 dx = target->x() - x();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);
    f64 dy = (target->y() + target->height() * 0.333) - (y() + eyeHeight() - 0.15)
           + horizontalDist * 0.2;

    // 创建箭矢实体
    auto arrow = std::make_unique<entity::ArrowEntity>(LegacyEntityType::Arrow, EntityId(0));
    arrow->setShotFromCrossbow(true);
    arrow->setDamage(5.0f);

    // 发射
    arrow->shootFrom(*this, pitch, yaw, 0.0f, velocity, inaccuracy);
    m_world->spawnEntity(std::move(arrow));
}
```

### AI Goals

掠夺者使用以下 AI 目标：

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳 |
| 3 | RangedCrossbowAttackGoal | 弩远程攻击 |
| 8 | RandomWalkingGoal | 随机漫步 |
| 9 | LookAtGoal<Player> | 看向玩家 |
| 10 | LookAtGoal | 看向生物 |

### 目标选择器

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 被攻击后反击并呼叫支援 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击玩家 |
| 3 | NearestAttackableTargetGoal<LivingEntity> | 攻击村民（TODO） |
| 3 | NearestAttackableTargetGoal<LivingEntity> | 攻击铁傀儡（TODO） |

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.PillagerEntity
├── registerGoals(): AI 目标注册
├── registerAttributes(): 属性设置
├── attackEntityWithRangedAttack(): 弩攻击实现
├── shootCrossbow(): 弩发射逻辑（ICrossbowUser 默认实现）
└── onCrossbowLoadComplete(): 装填完成回调
```

## VindicatorEntity 详细实现

卫道士是手持铁斧的近战灾厄村民。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.6f | 标准灾厄村民尺寸 |
| 高度 | 1.95f | MC 1.16.5 |
| 最大生命值 | 24.0 | MC 1.16.5 |
| 移动速度 | 0.35 | MC 1.16.5 |
| 攻击伤害 | 5.0 | 基础伤害（铁斧 +3 = 8） |
| 跟随范围 | 12.0 | MC 1.16.5 |

### AI Goals

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳 |
| 4 | MeleeAttackGoal | 近战攻击 |
| 8 | RandomWalkingGoal | 随机漫步 |
| 9 | LookAtGoal<Player> | 看向玩家（距离 3.0f） |
| 10 | LookAtGoal | 看向生物（距离 8.0f） |

### 目标选择器

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 被攻击后反击并呼叫支援 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击玩家 |
| 3 | NearestAttackableTargetGoal<LivingEntity> | 攻击村民（TODO） |
| 3 | NearestAttackableTargetGoal<LivingEntity> | 攻击铁傀儡（TODO） |

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.VindicatorEntity
├── registerGoals(): AI 目标注册
├── registerAttributes(): 属性设置
└── setAggressive(): 攻击状态设置
```

## VexEntity 详细实现

恼鬼是由唤魔者召唤的小型飞行敌对生物，具有独特的穿墙能力。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.4f | 极小碰撞箱 |
| 高度 | 0.8f | 小型实体 |
| 眼睛高度 | 0.4f | |
| 最大生命值 | 14.0 | MC 1.16.5 |
| 攻击伤害 | 4.0 | 铁剑伤害 |
| 飞行能力 | 是 | canFly() 返回 true |
| 日光燃烧 | 否 | shouldBurnInDaylight() 返回 false |

### 穿墙能力实现

恼鬼在每帧 tick 期间启用穿墙能力：

```cpp
void VexEntity::tick() {
    // MC 1.16.5: 恼鬼在 tick 期间可以穿墙
    setNoClip(true);
    MonsterEntity::tick();
    setNoClip(false);

    // 恼鬼始终不受重力影响
    setNoGravity(true);

    // 有限生命机制
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;
        if (m_lifeTime <= 0) {
            m_lifeTime = 20;  // 重置防止连续伤害
            hurt(DamageSources::starve(), 1.0f);
        }
    }
}
```

**实现要点**：
1. `setNoClip(true)` 在 tick 开始时设置，允许穿墙
2. 调用父类 `MonsterEntity::tick()` 处理常规逻辑
3. `setNoClip(false)` 在 tick 结束时重置
4. `setNoGravity(true)` 确保始终无重力飞行
5. 有限生命结束时使用饥饿伤害（`DamageSources::starve()`）

### 有限生命机制

| 参数 | 默认值 | 说明 |
|------|--------|------|
| m_limitedLife | true | 是否有限生命 |
| m_lifeTime | 2400 | 初始生命时间（约 2 分钟）|

有限生命结束时：
- 重置 `lifeTime` 为 20 tick（防止连续伤害）
- 造成 1.0 点饥饿伤害
- 恼鬼逐渐死亡

### VexEntity::create() 实现

恼鬼实体的 `create()` 工厂方法已修正为使用正确的 `LegacyEntityType::Vex` 类型：

```cpp
std::unique_ptr<VexEntity> VexEntity::create(EntityId id)
{
    return std::make_unique<VexEntity>(LegacyEntityType::Vex, id);
}
```

**注意**: 之前版本使用了 `LegacyEntityType::Unknown`，导致 `countNearbyVexes()` 无法正确识别恼鬼类型。

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.VexEntity
├── tick() 行 62-71: 穿墙、无重力、有限生命
├── registerAttributes(): 最大生命值 14.0、攻击伤害 4.0
├── registerGoals(): AI 目标注册
└── MoveHelperController: 自定义飞行移动控制
```

### VexEntity AI Goals

恼鬼使用专用的 AI 目标系统，实现独特的飞行攻击行为。

#### 专用移动控制器：VexMovementController

恼鬼使用自定义的 `VexMovementController` 替代标准移动控制器，实现穿墙飞行：

| 参数 | 值 | 说明 |
|------|-----|------|
| 速度因子 | speed * 0.05 / distance | MC 1.16.5 |
| 到达阈值 | 碰撞箱平均边长 | (width + height + width) / 3 |
| 减速因子 | 0.5 | 到达目标后速度减半 |

```cpp
void VexMovementController::tick() {
    if (m_action == MoveAction::MoveTo) {
        // 计算到目标的向量
        f64 dx = m_posX - m_vex->x();
        f64 dy = m_posY - m_vex->y();
        f64 dz = m_posZ - m_vex->z();
        f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (distance < avgEdgeLength) {
            // 到达目标，减速停止
            m_action = MoveAction::Wait;
            m_vex->setVelocity(velocity * 0.5);
        } else {
            // 添加速度向量
            f64 speedFactor = m_speed * 0.05 / distance;
            velocity += Vector3(dx, dy, dz) * speedFactor;
        }
    }
}
```

#### AI 目标列表

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳 |
| 4 | VexChargeAttackGoal | 冲锋攻击 |
| 8 | VexMoveRandomGoal | 随机飞行 |
| 9 | LookAtGoal<Player> | 看向玩家 |
| 10 | LookRandomlyGoal | 随机看向 |

#### 目标选择器

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 受攻击后反击 |
| 2 | VexCopyOwnerTargetGoal | 复制主人目标 |
| 3 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

#### VexChargeAttackGoal

冲锋攻击目标，恼鬼飞向目标的眼睛位置进行攻击：

| 参数 | 值 | 说明 |
|------|-----|------|
| 最小距离 | 2 格 | 距离大于 2 格才触发 |
| 停止追击 | 3 格 | 距离小于 3 格继续追击 |
| 攻击冷却 | 20 ticks | 1 秒 |
| 触发概率 | 1/7 | 约 14% |

```cpp
bool VexChargeAttackGoal::shouldExecute() {
    // 1. 有攻击目标
    // 2. 移动控制器未更新
    // 3. 1/7 概率
    // 4. 距离 > 2格
    return hasTarget && !isUpdating && rng.nextInt(7) == 0 && distSq > 4.0;
}
```

#### VexMoveRandomGoal

随机飞行目标，恼鬼在绑定点周围漫游：

| 参数 | 值 | 说明 |
|------|-----|------|
| 触发概率 | 1/7 | 约 14% |
| 漫游速度 | 0.25 | 较慢的漫游 |
| X 轴范围 | ±7 格 | 水平范围 |
| Y 轴范围 | ±5 格 | 垂直范围（较小） |
| Z 轴范围 | ±7 格 | 水平范围 |

```cpp
void VexMoveRandomGoal::tick() {
    // 在原点周围随机选择空气方块位置
    for (i32 i = 0; i < 3; ++i) {
        BlockPos targetPos(origin.x + offsetX, origin.y + offsetY, origin.z + offsetZ);
        if (world->getBlockState(targetPos)->isAir()) {
            moveController->setMoveTo(targetPos + 0.5, 0.25);
            break;
        }
    }
}
```

#### VexCopyOwnerTargetGoal

复制主人目标，当唤魔者有攻击目标时，恼鬼也会攻击同一目标：

```cpp
bool VexCopyOwnerTargetGoal::shouldExecute() {
    LivingEntity* owner = m_vex->getOwner();
    if (!owner) return false;

    MobEntity* ownerMob = dynamic_cast<MobEntity*>(owner);
    if (!ownerMob) return false;

    LivingEntity* ownerTarget = ownerMob->attackTarget();
    if (!ownerTarget || !ownerTarget->isAlive()) return false;

    // 检查视线
    if (!m_vex->canSee(*ownerTarget)) return false;

    return true;
}
```

#### 参考 MC 1.16.5

```
net.minecraft.entity.monster.VexEntity
├── ChargeAttackGoal: 冲锋攻击目标
│   ├── shouldExecute(): 1/7概率，距离>2格
│   ├── startExecuting(): 设置充电状态，移向目标眼睛
│   └── tick(): 检测碰撞，造成伤害
├── MoveRandomGoal: 随机飞行
│   └── tick(): 在±7x±5x±7范围找空气方块
├── CopyOwnerTargetGoal: 复制主人目标
│   └── shouldExecute(): 检查主人的攻击目标
└── MoveHelperController: 飞行移动控制器
    └── tick(): 修改velocity实现飞行
```

## AbstractIllagerEntity

灾厄村民的抽象基类，提供：

### 手臂姿势状态

```cpp
enum class ArmPose {
    Crossed,    // 交叉手臂（默认）
    Attacking,  // 攻击姿势
    Spellcasting, // 施法姿势
    Celebrating  // 庆祝姿势
};
```

### RAID 参与

- `getRaid()`: 获取参与的袭击
- `setRaid(Raid*)`: 设置袭击
- `getWave()`: 获取波次
- `setWave(int)`: 设置波次
- `isCelebrating()`: 是否在庆祝

## AbstractRaiderEntity

袭击者的抽象基类，继承自 PatrollerEntity：

### 袭击状态

```cpp
enum class RaiderState {
    Idle,       // 空闲
    Patrolling, // 巡逻
    Raiding,    // 袭击
    Celebrating // 庆祝
};
```

### 方法

- `canJoinRaid()`: 是否可以加入袭击
- `setCanJoinRaid(bool)`: 设置是否可加入
- `hasRaid()`: 是否在袭击中
- `getCurrentRaid()`: 获取当前袭击

## PatrollerEntity

巡逻者的基类，提供巡逻行为：

### 巡逻机制

- `canPatrol()`: 是否可以巡逻
- `isPatrolling()`: 是否在巡逻
- `setPatrolling(bool)`: 设置巡逻状态
- `getPatrolTarget()`: 获取巡逻目标
- `setPatrolTarget(BlockPos)`: 设置巡逻目标

## SpellcastingIllagerEntity

施法灾厄村民的基类：

### 施法状态

```cpp
enum class SpellType {
    None,       // 无
    SummonVex,  // 召唤恼鬼（唤魔者）
    Fangs,      // 尖牙攻击（唤魔者）
    Wololo,     // 唔噜噜（唤魔者，转换羊）
    Disappear,  // 消失（幻术师分身）
    Blindness   // 失明（幻术师失明攻击）
};
```

### 方法

- `isSpellcasting()`: 是否在施法
- `spellType()`: 获取法术类型
- `spellTicks()`: 获取施法 tick
- `setSpellType(SpellType)`: 设置法术类型
- `setSpellTicks(int)`: 设置施法持续时间
- `clearSpellcasting()`: 清除施法状态

## EvokerEntity

唤魔者是能够施法的灾厄村民，可以召唤尖牙攻击和恼鬼。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.6f | 标准灾厄村民尺寸 |
| 高度 | 1.8f | 标准灾厄村民高度 |
| 最大生命值 | 24.0 | MC 1.16.5 |
| 移动速度 | 0.5 | MC 1.16.5 |
| 跟随范围 | 12.0 | MC 1.16.5 |

### 施法能力

唤魔者有两种主要攻击法术：

#### 尖牙攻击 (Fangs Attack)

- **近距离攻击（<3格）**：两圈尖牙
  - 内圈：5个尖牙，半径1.5，延迟0
  - 外圈：8个尖牙，半径2.5，延迟3
- **远距离攻击**：直线16个尖牙
  - 朝目标方向直线排列
  - 延迟递增
- **施法参数**：
  - 准备时间：0 ticks
  - 施法时间：40 ticks
  - 冷却时间：100 ticks

#### 召唤恼鬼 (Summon Vex)

- 召唤3个恼鬼助战
- 只有当周围恼鬼数量少于8个时才会召唤
- 恼鬼有30-120秒的有限生命
- **施法参数**：
  - 准备时间：0 ticks
  - 施法时间：100 ticks
  - 冷却时间：340 ticks

### AI Goals

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 游泳 |
| 1 | EvokerCastingSpellGoal | 施法时看向目标 |
| 2 | AvoidEntityGoal | 避开玩家（距离8格） |
| 4 | EvokerSummonSpellGoal | 召唤恼鬼 |
| 5 | EvokerAttackSpellGoal | 尖牙攻击 |
| 8 | RandomWalkingGoal | 随机漫步 |
| 9 | LookAtGoal | 看向玩家 |
| 10 | LookAtGoal | 看向生物 |

### AI Goals 实现

EvokerEntity 使用专用 AI Goals：

- **EvokerSpellGoal** - 施法目标基类，管理施法准备时间和冷却
- **EvokerAttackSpellGoal** - 尖牙攻击目标
- **EvokerSummonSpellGoal** - 召唤恼鬼目标
- **EvokerCastingSpellGoal** - 施法期间看向目标

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.EvokerEntity
├── registerGoals(): AI 目标注册
├── registerAttributes(): 属性设置
├── AttackSpellGoal: 尖牙攻击目标
│   ├── castSpell(): 执行尖牙攻击
│   └── getCastingTime(): 40 ticks
├── SummonSpellGoal: 召唤恼鬼目标
│   ├── castSpell(): 召唤3个恼鬼
│   └── getCastingTime(): 100 ticks
└── CastingSpellGoal: 施法时看向目标
```

## EvokerFangsEntity

唤魔者尖牙是唤魔者召唤的攻击实体。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.5f | 碰撞箱宽度 |
| 高度 | 0.8f | 碰撞箱高度 |
| 伤害 | 6.0 | 魔法伤害 |
| 生命时长 | 22 ticks | 出现到消失 |

### 攻击机制

1. 预热延迟：尖牙出现前有预热时间
2. 伤害时机：在 warmupDelay = -8 时造成伤害
3. 范围伤害：对碰撞箱扩展0.2范围内的 LivingEntity 造成伤害
4. 队伍判断：不伤害唤魔者及其队友
5. 自动消失：攻击后自动消失

### 参考 MC 1.16.5

```
net.minecraft.entity.projectile.EvokerFangsEntity
├── tick(): 更新状态和造成伤害
├── damage(): 对范围内实体造成伤害
└── getAnimationProgress(): 获取动画进度
```

## WitchEntity

女巫是使用药水的敌对生物，可参与掠夺事件。

### 核心特性

| 特性 | 值 | 说明 |
|------|-----|------|
| 宽度 | 0.6f | MC 1.16.5 |
| 高度 | 1.95f | MC 1.16.5 |
| 眼睛高度 | 1.62f | MC 1.16.5 |
| 最大生命值 | 26.0 | MC 1.16.5 |
| 移动速度 | 0.25 | MC 1.16.5 |
| 攻击间隔 | 60 ticks | 3秒 |
| 攻击半径 | 10.0f | 远程攻击范围 |

### IRangedAttackMob 接口实现

女巫实现了 `IRangedAttackMob` 接口，支持药水远程攻击：

```cpp
class WitchEntity : public AbstractRaiderEntity, public entity::IRangedAttackMob {
public:
    // IRangedAttackMob 接口
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 60; }
    [[nodiscard]] bool canRangedAttack() const override { return !m_drinking; }
};
```

### 喝药水逻辑

女巫在 tick 中自动检测是否需要喝药水：

| 药水类型 | 触发条件 | 概率 | 效果 |
|---------|---------|------|------|
| 水肺药水 | 眼睛在水中且无水肺效果 | 15% | 水下呼吸 3 分钟 |
| 抗火药水 | 燃烧中或受火焰伤害且无抗火效果 | 15% | 防火 3 分钟 |
| 治疗药水 | 生命值未满 | 5% | 恢复 4 点生命 |
| 速度药水 | 有攻击目标且距离>11格且无速度效果 | 50% | 速度提升 3 分钟 |

- 喝药水时长：32 ticks
- 喝药水期间移动速度减少 25%

### 药水攻击逻辑

`attackEntityWithRangedAttack()` 根据目标状态选择药水类型：

| 目标状态 | 药水类型 | 说明 |
|---------|---------|------|
| 掠夺者同伴 && 生命<=4 | 治疗药水 | 治疗同伴 |
| 掠夺者同伴 && 生命>4 | 再生药水 | 恢复同伴生命 |
| 距离>=8格 && 无缓慢效果 | 缓慢药水 | 减速远程目标 |
| 生命>=8 && 无中毒效果 | 中毒药水 | 持续伤害 |
| 距离<=3格 && 无虚弱效果 | 虚弱药水 (25%概率) | 近战削弱 |
| 默认 | 伤害药水 | 即时伤害 |

### 投掷参数

```cpp
void WitchEntity::throwPotionAt(LivingEntity* target, EffectType potionType) {
    // 投掷方向（考虑目标运动）
    Vector3 targetMotion = target->velocity();
    f64 dx = target->x() + targetMotion.x - x();
    f64 dy = target->y() + target->eyeHeight() - 1.1 - y();
    f64 dz = target->z() + targetMotion.z - z();

    // 高度补偿
    f64 adjustedY = dy + horizontalDist * 0.2;

    // 发射参数
    potion->shoot(dx, adjustedY, dz, 0.75f, 8.0f);
    // velocity=0.75, inaccuracy=8.0

    // 播放音效
    playSound(SoundEvents::ENTITY_WITCH_THROW, 1.0f, 0.8f + rng.nextFloat() * 0.4f);
}
```

### 魔法伤害减免

女巫对魔法伤害有特殊抗性：

- **85% 魔法伤害减免**：只受到 15% 的魔法伤害
- **免疫自伤**：免疫自己造成的伤害（药水等）

```cpp
f32 WitchEntity::applyMagicDamageReduction(DamageSource& source, f32 amount) {
    if (source.getTrueSource() == this) {
        return 0.0f;  // 免疫自伤
    }
    if (source.isMagic()) {
        return amount * 0.15f;  // 只受15%伤害
    }
    return amount;
}
```

### AI Goals

| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳（父类注册） |
| 2 | RangedAttackGoal | 药水攻击 |
| 3+ | 其他目标 | 父类注册 |

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.WitchEntity
├── registerGoals(): AI 目标注册 (RangedAttackGoal)
├── registerAttributes(): 属性设置
├── livingTick(): 喝药水决策逻辑
├── attackEntityWithRangedAttack(): 药水攻击实现
├── throwPotion(): 投掷药水
└── applyMagicDamageReduction(): 魔法伤害减免
```

### 测试用例

| 测试文件 | 测试内容 |
|---------|---------|
| `WitchEntityTest.cpp` | 构造、属性、喝药水状态、效果应用、魔法伤害减免、IRangedAttackMob 接口 |

## 属性值对齐状态

| 实体 | 属性 | MC 1.16.5 | 项目值 | 状态 |
|------|------|-----------|--------|------|
| VindicatorEntity | ATTACK_DAMAGE | 5.0 | 5.0 | ✅ 已修复 |
| VindicatorEntity | FOLLOW_RANGE | 12.0 | 12.0 | ✅ 已修复 |
| PillagerEntity | FOLLOW_RANGE | 32.0 | 32.0 | ✅ 已修复 |
| VexEntity | MAX_HEALTH | 14.0 | 14.0 | ✅ 已修复 |
| VexEntity | ATTACK_DAMAGE | 4.0 | 4.0 | ✅ 已修复 |
| EvokerEntity | MOVEMENT_SPEED | 0.5 | 0.5 | ✅ 已修复 |

## 测试用例

| 测试文件 | 测试内容 |
|---------|---------|
| `VexEntityTest.cpp` | VexEntity 穿墙能力、属性、有限生命、构造、充电状态 |
| `EvokerEntityTest.cpp` | EvokerEntity 构造、属性、施法状态、EvokerFangsEntity |
| `WitchEntityTest.cpp` | WitchEntity 构造、属性、喝药水状态、效果应用、魔法伤害减免、IRangedAttackMob 接口 |

## 相关文档

- [敌对生物模块](../README.md)
- [实体系统](../../README.md)
- [实体核心](../../../core/README.md)
