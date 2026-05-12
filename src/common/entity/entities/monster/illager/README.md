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
    Summon,     // 召唤（唤魔者召唤恼鬼）
    Attack,     // 攻击（唤魔者尖牙）
    Vanish,     // 消失（幻术师分身）
    Blindness   // 失明（幻术师失明攻击）
};
```

### 方法

- `isSpellcasting()`: 是否在施法
- `getSpellType()`: 获取法术类型
- `getSpellTicks()`: 获取施法 tick
- `getSpellCooldown()`: 获取施法冷却

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

## 相关文档

- [敌对生物模块](../README.md)
- [实体系统](../../README.md)
- [实体核心](../../../core/README.md)
