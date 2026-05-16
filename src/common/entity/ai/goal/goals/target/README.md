# 目标选择器 Goal (Target Goals)

目标选择器用于选择攻击目标，与行为目标 (Goal) 共同构成实体的 AI 系统。

## 目录结构

```
target/
├── TargetGoals.hpp    # 目标选择器基类和模板类定义
├── TargetGoals.cpp    # 目标选择器实现
└── README.md          # 本文档
```

## 核心类

### TargetGoal - 目标选择器基类

**职责**: 所有目标选择器的基类，提供目标验证和视线检查功能。

**互斥标志**: `Target`

**关键方法**:
- `shouldExecute()` - 判断是否应该执行
- `startExecuting()` - 开始执行，设置攻击目标
- `resetTask()` - 重置任务，清除攻击目标
- `isSuitableTarget()` - 检查目标是否可攻击

**目标验证规则**:
1. 目标必须存活
2. 目标不能是自己
3. 创造模式/观察者模式玩家不能作为目标

**成员变量**:
- `m_mob` - 拥有此目标的生物
- `m_target` - 当前目标
- `m_checkSight` - 是否需要视线检查
- `m_unseenTicks` - 看不到目标的计时器
- `MAX_UNSEEN_TICKS` - 最大记忆时间 (60 tick = 3秒)

---

## 目标选择器列表

### NearestAttackableTargetGoal<T> - 最近可攻击目标

**职责**: 选择最近的符合条件的目标进行攻击。

**模板参数**: `T` - 目标实体类型，必须是 `LivingEntity` 的子类

**构造函数**:
| 构造函数 | 说明 |
|----------|------|
| `NearestAttackableTargetGoal(mob, checkSight, chance)` | 基础构造函数 |
| `NearestAttackableTargetGoal(mob, checkSight, chance, predicate)` | 带自定义筛选谓词 |

**参数说明**:
- `mob` - 拥有此目标的生物
- `checkSight` - 是否需要视线检查
- `chance` - 检查概率倒数 (0 = 每tick检查, 10 = 每10tick检查一次)
- `predicate` - 自定义目标筛选函数

**使用示例**:
```cpp
// 攻击最近的玩家
m_targetSelector.addGoal(1, std::make_unique<NearestAttackableTargetGoal<Player>>(
    this, true, 0));

// 攻击最近的怪物（每10tick检查一次）
m_targetSelector.addGoal(2, std::make_unique<NearestAttackableTargetGoal<MobEntity>>(
    this, true, 10, [](const LivingEntity* entity) {
        const MonsterEntity* monster = dynamic_cast<const MonsterEntity*>(entity);
        return monster != nullptr;
    }));

// 攻击特定类型的实体（羊、兔子、狐狸）
m_targetSelector.addGoal(5, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
    this, true, 0, [](const LivingEntity* entity) {
        auto type = entity->legacyType();
        return type == LegacyEntityType::Sheep ||
               type == LegacyEntityType::Rabbit ||
               type == LegacyEntityType::Fox;
    }));
```

**已显式实例化的类型**:
- `LivingEntity`
- `MobEntity`
- `Player`
- `ChickenEntity`
- `TurtleEntity`
- `FoxEntity`
- `IronGolemEntity`
- `AbstractPiglinEntity`
- `VillagerEntity`
- `AbstractVillagerEntity`

---

### HurtByTargetGoal - 被攻击后反击

**职责**: 当实体被攻击时，记住攻击者并反击。

**构造函数**:
| 构造函数 | 说明 |
|----------|------|
| `HurtByTargetGoal(mob, alertAllies)` | 基础构造函数 |

**参数说明**:
- `alertAllies` - 是否警醒附近同类实体

**行为**:
1. 从 `m_mob->getLastHurtBy()` 获取攻击者
2. 检查时间戳避免重复触发
3. 设置攻击目标
4. 如果 `alertAllies = true`，警醒附近同类实体

**使用示例**:
```cpp
// 被攻击后反击（不警醒同伴）
m_targetSelector.addGoal(1, std::make_unique<HurtByTargetGoal>(this, false));

// 被攻击后反击（警醒同伴，如狼群）
m_targetSelector.addGoal(3, std::make_unique<HurtByTargetGoal>(this, true));
```

**参考**: `net.minecraft.entity.ai.goal.HurtByTargetGoal`

---

### OwnerHurtByTargetGoal - 主人被攻击时反击

**职责**: 当驯服动物的主人被攻击时，反击攻击者。

**适用**: 需要配合 `TameableEntity` 使用

**执行条件**:
1. 实体必须是 `TameableEntity`
2. 实体必须已驯服 (`isTamed()`)
3. 实体不能坐下 (`!isSitting()`)
4. 主人存在且有攻击者

**行为**:
1. 从 `owner->getLastHurtBy()` 获取攻击主人的人
2. 验证攻击者有效性
3. 设置攻击目标

**使用示例**:
```cpp
// 在 WolfEntity::registerGoals() 中
m_targetSelector.addGoal(1, std::make_unique<OwnerHurtByTargetGoal>(this));
```

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtByTargetGoal`

---

### OwnerHurtTargetGoal - 攻击主人正在攻击的目标

**职责**: 当驯服动物的主人攻击某实体时，也攻击该实体。

**适用**: 需要配合 `TameableEntity` 使用

**执行条件**:
1. 实体必须是 `TameableEntity`
2. 实体必须已驯服 (`isTamed()`)
3. 实体不能坐下 (`!isSitting()`)
4. 主人存在且有攻击目标

**行为**:
1. 从 `owner->getLastHurtTarget()` 获取主人攻击的目标
2. 验证目标有效性
3. 设置攻击目标

**使用示例**:
```cpp
// 在 WolfEntity::registerGoals() 中
m_targetSelector.addGoal(2, std::make_unique<OwnerHurtTargetGoal>(this));
```

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtTargetGoal`

---

### NonTamedTargetGoal<T> - 未驯服状态目标选择

**职责**: 驯服动物在未驯服状态下选择攻击目标，驯服后不再执行。

**模板参数**: `T` - 目标实体类型，必须是 `LivingEntity` 的子类

**构造函数**:
| 构造函数 | 说明 |
|----------|------|
| `NonTamedTargetGoal(mob, checkSight)` | 基础构造函数 |
| `NonTamedTargetGoal(mob, checkSight, predicate)` | 带自定义筛选谓词 |

**执行条件**:
1. 实体必须是 `TameableEntity`
2. 实体必须**未驯服** (`!isTamed()`)

**使用示例**:
```cpp
// 未驯服的狼攻击幼海龟（不在水中）
m_targetSelector.addGoal(6, std::make_unique<NonTamedTargetGoal<TurtleEntity>>(
    this, true, [](const LivingEntity* entity) {
        const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
        return turtle != nullptr && turtle->isChild() && !turtle->isInWater();
    }));
```

**参考**: `net.minecraft.entity.ai.goal.NonTamedTargetGoal`

---

### ResetAngerGoal<T> - 重置愤怒目标

**职责**: 当 `UNIVERSAL_ANGER` 游戏规则启用时，检查并处理愤怒目标。

**适用**: 实现了 `IAngerable` 接口的实体（铁傀儡、末影人等）

**模板约束**: `T` 必须同时继承 `MobEntity` 和 `IAngerable`

**构造函数**:
| 构造函数 | 说明 |
|----------|------|
| `ResetAngerGoal(mob, alertOthers)` | 基础构造函数 |

**参数说明**:
- `alertOthers` - 是否警醒附近同类实体

**行为**:
1. 检查是否应该对玩家复仇
2. 重置愤怒状态
3. 清除攻击目标
4. 如果 `alertOthers = true`，重置附近同类实体的愤怒

**参考**: `net.minecraft.entity.ai.goal.ResetAngerGoal`

---

## 目标选择优先级

**优先级数值越小，优先级越高**。

典型优先级分配:

| 优先级 | 目标选择器 | 说明 |
|--------|------------|------|
| 1 | OwnerHurtByTargetGoal | 主人被攻击时反击（最高优先级） |
| 2 | OwnerHurtTargetGoal | 攻击主人正在攻击的目标 |
| 3 | HurtByTargetGoal | 被攻击后反击 |
| 4 | NearestAttackableTargetGoal | 最近目标选择（愤怒时攻击玩家） |
| 5 | NonTamedTargetGoal | 未驯服状态目标选择 |
| 6-7 | 其他目标选择器 | 根据实体类型分配 |
| 8 | ResetAngerGoal | 重置愤怒（最低优先级） |

---

## 狼实体目标选择器示例

狼实体的完整目标选择器配置，参考 MC 1.16.5:

```cpp
void WolfEntity::registerGoals()
{
    // ... 其他目标 ...

    // 目标选择器
    // 优先级 1: 主人被攻击时反击
    m_targetSelector.addGoal(1, std::make_unique<OwnerHurtByTargetGoal>(this));

    // 优先级 2: 攻击主人正在攻击的目标
    m_targetSelector.addGoal(2, std::make_unique<OwnerHurtTargetGoal>(this));

    // 优先级 3: 被攻击后反击，并呼叫同伴
    m_targetSelector.addGoal(3, std::make_unique<HurtByTargetGoal>(this, true));

    // 优先级 5: 未驯服时攻击羊、兔子、狐狸
    m_targetSelector.addGoal(5, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
        this, true, 0, [](const LivingEntity* entity) {
            auto type = entity->legacyType();
            return type == LegacyEntityType::Sheep ||
                   type == LegacyEntityType::Rabbit ||
                   type == LegacyEntityType::Fox;
        }));

    // 优先级 6: 未驯服时攻击幼海龟
    m_targetSelector.addGoal(6, std::make_unique<NonTamedTargetGoal<TurtleEntity>>(
        this, true, [](const LivingEntity* entity) {
            const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
            return turtle != nullptr && turtle->isChild() && !turtle->isInWater();
        }));

    // 优先级 7: 攻击骷髅类怪物
    m_targetSelector.addGoal(7, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
        this, false, 0, [](const LivingEntity* entity) {
            auto type = entity->legacyType();
            return type == LegacyEntityType::Skeleton ||
                   type == LegacyEntityType::Stray ||
                   type == LegacyEntityType::WitherSkeleton;
        }));
}
```

---

## 依赖关系

```
TargetGoal (基类)
├── NearestAttackableTargetGoal<T>
├── HurtByTargetGoal
├── OwnerHurtByTargetGoal
├── OwnerHurtTargetGoal
├── NonTamedTargetGoal<T>
└── ResetAngerGoal<T>
```

**外部依赖**:
- `LivingEntity` - 提供 `getLastHurtBy()`, `getLastHurtTarget()`, `lastHurtByTimestamp()`, `lastHurtTargetTimestamp()`
- `TameableEntity` - 提供 `isTamed()`, `isSitting()`, `getOwner()`
- `IAngerable` - 提供 `getRevengeTarget()`, `getRevengeTimer()`, `setAngry()`, `setAngerTime()`
- `EntityUtils::findClosestEntity<T>()` - 用于查找最近的目标

---

## 测试

相关测试文件:
- `tests/common/entity/GoalTests.cpp` - 目标选择器基础测试
- `tests/common/entity/entities/passive/tamable/WolfEntityTest.cpp` - 狼实体测试（包含目标选择器验证）
