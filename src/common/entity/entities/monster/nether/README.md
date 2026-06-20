# 下界怪物模块

本目录包含下界维度中的怪物实体实现。

## 目录结构

```
src/common/entity/entities/monster/nether/
├── BlazeEntity.hpp/cpp      # 烈焰人实体（火球攻击、飞行）
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
    │   ├── PiglinEntity ────────────── ICrossbowUser (弩使用者接口)
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
