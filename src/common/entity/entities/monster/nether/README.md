# 下界怪物模块

本目录包含下界维度中的怪物实体实现。

## 目录结构

```
src/common/entity/entities/monster/nether/
├── BlazeEntity.hpp/cpp      # 烈焰人实体（火球攻击、悬浮飞行、水伤害）
├── NetherEntities.hpp/cpp   # 其他下界怪物：恶魂、岩浆怪、猪灵、猪灵蛮兵、僵尸猪灵、疣猪兽、僵尸疣兽
└── README.md
```

## 内部模块关系

```
MonsterEntity (基类，来自 ../MonsterEntity.hpp)
    │
    ├── BlazeEntity ─────────────────── IRangedAttackMob (远程攻击接口)
    │
    ├── GhastEntity ─────────────────── GhastMovementController (恶魂移动控制器)
    │
    ├── MagmaCubeEntity ─────────────── SlimeEntity (来自 ../basic/SlimeEntity.hpp)
    │
    ├── AbstractPiglinEntity ────────── 猪灵基类，提供火焰免疫、主世界转化机制
    │   ├── PiglinEntity ────────────── ICrossbowUser (弩使用者接口) + IAngerable (愤怒接口)
    │   └── PiglinBruteEntity
    │
    ├── ZombifiedPiglinEntity ───────── 愤怒机制（激怒附近同类）
    │
    ├── HoglinEntity ────────────────── IFlinging (击退攻击接口)
    │
    └── ZoglinEntity ────────────────── IFlinging (击退攻击接口)
```

## 上下游外部依赖关系

### 上游依赖（本目录依赖的模块）

- `../MonsterEntity.hpp` - 敌对生物基类
- `../basic/SlimeEntity.hpp` - 史莱姆基类（岩浆怪继承）
- `../../interfaces/IRangedAttackMob.hpp` - 远程攻击接口（烈焰人）
- `../../interfaces/ICrossbowUser.hpp` - 弩使用者接口（猪灵）
- `../../interfaces/IAngerable.hpp` - 愤怒接口（猪灵实现IAngerable，管理愤怒计时器和攻击目标）
- `../../interfaces/IFlinging.hpp` - 击退攻击接口（疣猪兽、僵尸疣兽）
- `../../ai/goal/goals/special/BlazeFireballAttackGoal.hpp` - 烈焰人火球攻击目标
- `../../ai/goal/goals/special/GhastGoals.hpp` - 恶魂AI目标
- `../../ai/controller/GhastMovementController.hpp` - 恶魂移动控制器

### 下游依赖（依赖本目录的模块）

- `VanillaEntities.hpp` - 注册所有原版实体
- `ProjectileItemEntity.cpp` - 投掷物判断烈焰人火焰免疫
- `TargetGoals.cpp` - 目标选择器（猪灵相关目标）
- `WitherSkeletonEntity.cpp` - 凋灵骷髅（引用猪灵相关类型）

## 容易踩的坑

### 1. 岩浆怪继承史莱姆的AI注册

```cpp
// ❌ 错误：重复注册 AI 目标
void MagmaCubeEntity::registerGoals() {
    MonsterEntity::registerGoals();  // 错误！会导致重复注册
}

// ✅ 正确：调用直接父类
void MagmaCubeEntity::registerGoals() {
    SlimeEntity::registerGoals();  // 复用史莱姆 AI
}
```

### 2. 护甲属性注册顺序

岩浆怪的护甲属性必须在`registerAttributes()`中注册后才能在`setSlimeSize()`中设置值，否则会崩溃。

### 3. isClientSide() const 问题（已修复）

`IWorld::isClientSide()`已改为const方法，在const成员函数中可直接调用，无需`const_cast`。

### 4. 猪灵蛮兵 vs 普通猪灵的目标选择差异

猪灵蛮兵攻击玩家时不检查金装备，普通猪灵会检查。这是MC 1.16.5的正确行为。

### 5. 僵尸猪灵愤怒机制

僵尸猪灵被攻击后会激怒附近所有僵尸猪灵，需要在`HurtByTargetGoal`中设置`setCallsForHelp(true)`。

### 6. PiglinEntity 的 IAngerable 与 MobEntity::setAttackTarget 双状态一致性

PiglinEntity 同时继承 `IAngerable`（含虚函数 `setAttackTarget`）和 `MobEntity`（含 `setAttackTarget`）。
为避免双状态不一致，`MobEntity::setAttackTarget` 已改为虚函数，PiglinEntity 的 override 统一使用
`MobEntity::m_attackTarget`，不再声明独立的 `m_attackTarget` 成员。所有 IAngerable 实体
（GolemEntity、EndermanEntity、BeeEntity、PolarBearEntity、TameableEntity）也遵循同一模式。

**注意**：通过 `MobEntity*` 指针调用 `setAttackTarget` 时，虚函数派发会正确到达子类的 override，
确保愤怒状态与攻击目标始终同步。

### 7. isChild() 虚函数重写

HoglinEntity、ZoglinEntity 和 PiglinEntity 均拥有 `isBaby()` 方法来管理幼年状态，
但必须同时重写 `Entity::isChild()` 虚函数并委托给 `isBaby()`。
否则通过 `Entity*` 基类指针多态调用 `isChild()` 时将始终返回 `false`，
导致依赖 `isChild()` 的游戏逻辑（如幼年实体不激怒玩家、幼年碰撞箱缩放等）行为异常。

### 8. HoglinEntity 寻路权重与驱避物检测

HoglinEntity 重写了 `getPathWeight(f32 x, f32 y, f32 z)` 方法：
- 站在绯红菌岩（Crimson Nylium）上时返回 `10.0f`，偏好在该方块上生成和移动；
- 位置附近 8×4×8 范围内存在驱避物方块时返回 `-1.0f`，拒绝前往该区域；
- 其他方块返回 `0.0f`。

驱避物方块由 `BlockTags::HOGLIN_REPELLENTS` 标签定义，包含：
诡异菌(warped_fungus)、诡异菌岩(warped_nylium)、下界传送门(nether_portal)、重生锚(respawn_anchor)。

对应 MC 1.21.11 的 `HoglinSpecificSensor.findNearestRepellent` 逻辑，
本项目采用在 `getPathWeight()` 中直接扫描方块的方式实现（因 HoglinEntity 使用 Goal 系统而非 Brain 系统）。

### 9. ZoglinEntity canAttackType 类型过滤

ZoglinEntity 重写了 `MobEntity::canAttackType()`，对应 MC 原版 `Zoglin.isTargetable` 的类型过滤部分：
- 排除同类 `ZOGLIN`（僵尸疣兽不互相攻击）；
- 排除 `CREEPER`（苦力怕，与铁傀儡行为一致）；
- 其他类型委托给 `MonsterEntity::canAttackType()`（默认排除 `GHAST`）。

`registerGoals()` 中的 `TargetPredicate` 同样排除了 `ZOGLIN` 和 `CREEPER`，
与 `canAttackType()` 形成双重保障，确保目标选择器不会选中这两种实体。
测试位于 `tests/entity/CanAttackTypeTest.cpp`。
