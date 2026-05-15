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
| VindicatorEntity | 卫道士 | 斧头近战攻击、冲向目标 | ✅ 属性已修复 |
| EvokerEntity | 唤魔者 | 尖牙攻击、召唤恼鬼 | ✅ 属性已修复 |
| IllusionerEntity | 幻术师 | 分身、失明攻击 | ⏳ 框架完成 |
| PillagerEntity | 掠夺者 | 弩远程攻击 | ✅ 属性已修复 |
| RavagerEntity | 劫掠兽 | 冲撞攻击、破坏方块 | ⏳ 框架完成 |
| VexEntity | 恼鬼 | **穿墙飞行**、有限生命 | ✅ 完成 |
| WitchEntity | 女巫 | 药水攻击、治疗 | ✅ 完成 |

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

### 参考 MC 1.16.5

```
net.minecraft.entity.monster.VexEntity
├── tick() 行 62-71: 穿墙、无重力、有限生命
├── registerAttributes(): 最大生命值 14.0、攻击伤害 4.0
├── registerGoals(): AI 目标注册
└── MoveHelperController: 自定义飞行移动控制
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

## 相关文档

- [敌对生物模块](../README.md)
- [实体系统](../../README.md)
- [实体核心](../../../core/README.md)
