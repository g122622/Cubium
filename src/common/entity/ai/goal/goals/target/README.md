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

所有目标选择器的基类，提供目标验证和视线检查功能。

**互斥标志**: `Target`

**关键方法**:
| 方法 | 说明 |
|------|------|
| `shouldExecute()` | 判断是否应该执行 |
| `startExecuting()` | 开始执行，设置攻击目标 |
| `resetTask()` | 重置任务，清除攻击目标 |
| `isSuitableTarget()` | 检查目标是否可攻击 |

**目标验证规则**:
- 目标必须存活
- 目标不能是自己
- 创造模式/观察者模式玩家不能作为目标

---

## 目标选择器列表

### NearestAttackableTargetGoal<T> - 最近可攻击目标

选择最近的符合条件的目标进行攻击。

**模板参数**: `T` - 目标实体类型，必须是 `LivingEntity` 的子类

**构造参数**:
| 参数 | 说明 |
|------|------|
| `mob` | 拥有此目标的生物 |
| `checkSight` | 是否需要视线检查 |
| `chance` | 检查概率倒数 (0 = 每tick检查, 10 = 每10tick检查一次) |
| `predicate` | 自定义目标筛选函数（可选） |

**已显式实例化的类型**:
`LivingEntity`, `MobEntity`, `Player`, `ChickenEntity`, `TurtleEntity`, `FoxEntity`, `IronGolemEntity`, `AbstractPiglinEntity`, `VillagerEntity`, `AbstractVillagerEntity`, `EndermiteEntity`

---

### HurtByTargetGoal - 被攻击后反击

当实体被攻击时，记住攻击者并反击。

**构造参数**:
| 参数 | 说明 |
|------|------|
| `mob` | 拥有此目标的生物 |
| `alertAllies` | 是否警醒附近同类实体 |

**参考**: `net.minecraft.entity.ai.goal.HurtByTargetGoal`

---

### OwnerHurtByTargetGoal - 主人被攻击时反击

当驯服动物的主人被攻击时，反击攻击者。需配合 `TameableEntity` 使用。

**执行条件**:
- 实体必须是 `TameableEntity` 且已驯服
- 实体不能坐下
- 主人存在且有攻击者

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtByTargetGoal`

---

### OwnerHurtTargetGoal - 攻击主人正在攻击的目标

当驯服动物的主人攻击某实体时，也攻击该实体。需配合 `TameableEntity` 使用。

**执行条件**:
- 实体必须是 `TameableEntity` 且已驯服
- 实体不能坐下
- 主人存在且有攻击目标

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtTargetGoal`

---

### NonTamedTargetGoal<T> - 未驯服状态目标选择

驯服动物在未驯服状态下选择攻击目标，驯服后不再执行。

**模板参数**: `T` - 目标实体类型，必须是 `LivingEntity` 的子类

**执行条件**:
- 实体必须是 `TameableEntity`
- 实体必须**未驯服**

**已显式实例化的类型**: `LivingEntity`, `MobEntity`, `TurtleEntity`

**参考**: `net.minecraft.entity.ai.goal.NonTamedTargetGoal`

---

### ResetAngerGoal<T> - 重置愤怒目标

当 `UNIVERSAL_ANGER` 游戏规则启用时，检查并处理愤怒目标。用于实现了 `IAngerable` 接口的实体（铁傀儡、末影人等）。

**模板约束**: `T` 必须同时继承 `MobEntity` 和 `IAngerable`

**构造参数**:
| 参数 | 说明 |
|------|------|
| `mob` | 拥有此目标的实体 |
| `alertOthers` | 是否警醒附近同类实体 |

**已显式实例化的类型**: `EndermanEntity`

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

## 内部模块关系

```
TargetGoal (基类)
├── NearestAttackableTargetGoal<T>
├── HurtByTargetGoal
├── OwnerHurtByTargetGoal
├── OwnerHurtTargetGoal
├── NonTamedTargetGoal<T>
└── ResetAngerGoal<T>
```

---

## 上下游外部依赖关系

### 本目录依赖

- `entity/ai/goal/Goal.hpp` - 目标基类
- `entity/core/EntityUtils.hpp` - `findClosestEntity<T>()` 查找最近目标
- `entity/core/LivingEntity.hpp` - `getLastHurtBy()`, `getLastHurtTarget()`, `lastHurtByTimestamp()`, `lastHurtTargetTimestamp()`
- `entity/core/MobEntity.hpp` - `setAttackTarget()`, `canSee()`, `getAttributeValue()`
- `entity/entities/passive/tamable/TameableEntity.hpp` - `isTamed()`, `isSitting()`, `getOwner()`
- `entity/interfaces/IAngerable.hpp` - `getRevengeTarget()`, `getRevengeTimer()`, `setAngry()`, `setAngerTime()`
- `util/math/random/Random.hpp` - 随机数生成
- `world/IWorld.hpp` - 世界查询

### 被依赖

目标选择器被各种实体在 `registerGoals()` 中使用，主要使用者包括：
- `WolfEntity` - 狼（完整的驯服动物目标选择）
- `IronGolemEntity` - 铁傀儡
- `EndermanEntity` - 末影人（愤怒机制）
- `BeeEntity` - 蜜蜂
- 各种怪物实体（僵尸、骷髅、掠夺者等）

---

## 容易踩的坑

### 1. 优先级数值混淆

**问题**: 以为数值越大优先级越高。

**正确**: 数值越小优先级越高。
```cpp
// 高优先级用小数值
m_targetSelector.addGoal(1, ownerHurtByTarget);  // 最高优先级
m_targetSelector.addGoal(8, resetAngerGoal);     // 最低优先级
```

### 2. 目标类型未显式实例化

**问题**: 使用 `NearestAttackableTargetGoal<SomeEntity>` 时链接报错。

**解决**: 在 `TargetGoals.cpp` 末尾添加显式实例化：
```cpp
template class NearestAttackableTargetGoal<SomeEntity>;
```

### 3. TargetPredicate 返回值语义

**问题**: 谓词返回 `true` 的语义不明确。

**正确**: 返回 `true` 表示该实体**可以作为目标**，返回 `false` 表示**排除该实体**。

### 4. 视线检查与性能

**问题**: `checkSight=true` 时每帧进行视线检测，性能开销大。

**建议**: 高频检查的目标（如 `chance=0`）考虑设置 `checkSight=false`，或适当增大 `chance` 值。

### 5. HurtByTargetGoal 时间戳检查

**问题**: 同一次攻击可能触发多次 `startExecuting()`。

**原因**: `shouldExecute()` 只检查时间戳变化，但 `resetTask()` 会重置 `m_timestamp=0`，导致下次检查可能再次触发。

**注意**: 不要在子类中重置 `m_timestamp`。

### 6. ResetAngerGoal 模板约束

**问题**: 使用非 `IAngerable` 类型实例化 `ResetAngerGoal<T>` 导致编译错误。

**解决**: 确保实体类同时继承 `MobEntity` 和实现 `IAngerable` 接口。

### 7. OwnerHurtByTargetGoal/OwnerHurtTargetGoal 驯服检查

**问题**: 非驯服动物使用这些目标时 `shouldExecute()` 始终返回 `false`。

**原因**: 内部使用 `dynamic_cast<TameableEntity*>` 检查，非驯服动物转换失败。

**解决**: 仅在 `TameableEntity` 子类中使用这些目标。
