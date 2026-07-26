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
| `shouldContinueExecuting()` | 判断是否应该继续执行（含视线记忆检查） |
| `startExecuting()` | 开始执行，设置攻击目标 |
| `resetTask()` | 重置任务，清除攻击目标 |
| `isSuitableTarget()` | 检查目标是否可攻击 |
| `setUnseenMemoryTicks(ticks)` | 设置视线记忆时间，返回 `*this` 支持链式调用 |

**视线记忆机制**:
- 当 `checkSight=true` 时，如果目标离开视线，`m_unseenTicks` 每tick递增
- 如果 `m_unseenTicks` 超过 `m_unseenMemoryTicks`（默认60tick=3秒），目标放弃追踪
- 目标重新进入视线时 `m_unseenTicks` 重置为0
- 调用 `setUnseenMemoryTicks(300)` 可将记忆时间延长到15秒（唤魔者、幻术师等使用）
- `HurtByTargetGoal` 在 `startExecuting()` 中自动将 `m_unseenMemoryTicks` 设为300

**目标验证规则**:
- 目标必须存活
- 目标不能是自己
- **目标的实体类型必须可攻击**（通过 `canAttackType()` 检查，对应 MC 原版 `Mob.canAttackType()`）
- 创造模式/观察者模式玩家不能作为目标
- 同盟实体不能作为目标（通过 `isAlliedTo()` 双向检查）

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
`LivingEntity`, `MobEntity`, `Player`, `ChickenEntity`, `RabbitEntity`, `TurtleEntity`, `FoxEntity`, `IronGolemEntity`, `AbstractPiglinEntity`, `VillagerEntity`, `AbstractVillagerEntity`, `EndermiteEntity`, `AxolotlEntity`

---

### HurtByTargetGoal - 被攻击后反击

当实体被攻击时，记住攻击者并反击。

**构造参数**:
| 参数 | 说明 |
|------|------|
| `mob` | 拥有此目标的生物 |
| `alertAllies` | 是否警醒附近同类实体 |
| `ignoreDamagePredicate` | 攻击者排除谓词（可选），返回 true 表示不对该攻击者反击（对应 MC toIgnoreDamage） |

**链式方法**:
| 方法 | 说明 |
|------|------|
| `setAlertOthers(ignoreAlertPredicate)` | 设置警醒盟友时的排除谓词，返回 `*this` 支持链式调用。调用此方法会自动启用 `alertAllies`（对应 MC setAlertOthers） |

**行为细节**:
- `shouldExecute()` 中检查 `ignoreDamagePredicate`：若攻击者匹配排除谓词，不进行反击
- `startExecuting()` 警醒盟友时，跳过与攻击者同盟的实体（通过 `isAlliedTo()` 检查），避免友军误伤
- `startExecuting()` 警醒盟友时，检查 `ignoreAlertPredicate`：若盟友匹配排除谓词，不警醒该盟友

**MC 原版映射**:
| MC Java | C++ |
|---------|-----|
| `HurtByTargetGoal(this, Guardian.class)` | `HurtByTargetGoal(this, true, [](auto* a) { return isGuardian(a); })` |
| `.setAlertOthers()` | `.setAlertOthers({})` 或构造时 `alertAllies=true` |
| `.setAlertOthers(ZombifiedPiglin.class)` | `.setAlertOthers([](auto* a) { return isZombifiedPiglin(a); })` |

**参考**: `net.minecraft.world.entity.ai.goal.target.HurtByTargetGoal`

---

### OwnerHurtByTargetGoal - 主人被攻击时反击

当驯服动物的主人被攻击时，反击攻击者。需配合 `TameableEntity` 使用。

**执行条件**:
- 实体必须是 `TameableEntity` 且已驯服
- 实体不能坐下
- 主人存在且有攻击者
- 目标通过 `isSuitableTarget()` 和 `wantsToAttack()` 检查

**行为细节**:
- 调用 `wantsToAttack(target, owner)` 过滤攻击目标，狼不会攻击苦力怕/恶魂/盔甲架/已驯服动物等

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtByTargetGoal`

---

### OwnerHurtTargetGoal - 攻击主人正在攻击的目标

当驯服动物的主人攻击某实体时，也攻击该实体。需配合 `TameableEntity` 使用。

**执行条件**:
- 实体必须是 `TameableEntity` 且已驯服
- 实体不能坐下
- 主人存在且有攻击目标
- 目标通过 `isSuitableTarget()` 和 `wantsToAttack()` 检查

**行为细节**:
- 调用 `wantsToAttack(target, owner)` 过滤攻击目标，狼不会攻击苦力怕/恶魂/盔甲架/已驯服动物等

**参考**: `net.minecraft.entity.ai.goal.OwnerHurtTargetGoal`

---

### NonTamedTargetGoal<T> - 未驯服状态目标选择

驯服动物在未驯服状态下选择攻击目标，驯服后不再执行。

**模板参数**: `T` - 目标实体类型，必须是 `LivingEntity` 的子类

**执行条件**:
- 实体必须是 `TameableEntity`
- 实体必须**未驯服**

**已显式实例化的类型**: `LivingEntity`, `MobEntity`, `TurtleEntity`, `RabbitEntity`

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

**执行条件**:
- `UNIVERSAL_ANGER` 游戏规则必须启用，否则 `shouldExecute()` 直接返回 false

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
- `entity/core/Entity.hpp` - `isAlliedTo()` 队伍联盟判断
- `entity/core/LivingEntity.hpp` - `getLastHurtBy()`, `getLastHurtTarget()`, `lastHurtByTimestamp()`, `lastHurtTargetTimestamp()`
- `entity/core/MobEntity.hpp` - `setAttackTarget()`, `canSee()`, `getAttributeValue()`
- `entity/entities/passive/tamable/TameableEntity.hpp` - `isTamed()`, `isSitting()`, `getOwner()`, `wantsToAttack()`
- `entity/interfaces/IAngerable.hpp` - `getRevengeTarget()`, `getRevengeTimer()`, `setAngry()`, `setAngerTime()`
- `world/IWorld.hpp` - 世界查询、游戏规则（`UNIVERSAL_ANGER`）
- `util/math/random/Random.hpp` - 随机数生成

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

### 6. 子类替换父类 HurtByTargetGoal

**场景**: DrownedEntity 继承 ZombieEntity 的 `registerGoals()`，但需要不同的 `HurtByTargetGoal` 配置（MC 原版 Drowned 使用 `HurtByTargetGoal(this, Drowned.class).setAlertOthers(ZombifiedPiglin.class)`，而 Zombie 使用 `HurtByTargetGoal(this).setAlertOthers(ZombifiedPiglin.class)`）。

**解决**: 先调用父类 `registerGoals()`，再通过 `GoalSelector::removeGoalsOfType<HurtByTargetGoal>()` 移除父类注册的版本，然后添加子类专用版本：

```cpp
void DrownedEntity::registerGoals()
{
    ZombieEntity::registerGoals();  // 先注册父类所有目标

    // 移除父类的 HurtByTargetGoal
    m_targetSelector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 添加溺尸专用的 HurtByTargetGoal（排除同类溺尸，不警醒僵尸猪灵）
    auto hurtByTarget = std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true,
        [](const LivingEntity* attacker) -> bool {
            return attacker != nullptr && attacker->entityType() == entity::VanillaEntityTypeKeys::DROWNED;
        });
    hurtByTarget->setAlertOthers([](const LivingEntity* ally) -> bool {
        return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
    });
    m_targetSelector.addGoal(1, std::move(hurtByTarget));
}
```

### 7. alertOthers 同类型过滤机制

**重要**: `startExecuting()` 中的 alertOthers 逻辑使用 `ally->entityType() == m_mob->entityType()` 进行同类型过滤，这与 MC Java 的 `getEntitiesOfClass(this.mob.getClass(), ...)` 行为一致——只警醒与被攻击实体**完全相同类型**的实体。

**影响**:
- `setAlertOthers(ZombifiedPiglin 排除谓词)` 在 ZombieEntity 上：由于 typeId 过滤只返回同为 Zombie 的实体，ZombifiedPiglin 本身不会被选中，因此排除谓词在此场景下不会被触发。这在 MC Java 中也是同样的行为。
- 但在 ZombifiedPiglinEntity 自身的 alertOthers 中，排除谓词是有意义的：当一只僵尸猪灵被攻击时，`getEntitiesOfClass(ZombifiedPiglin.class)` 会返回其他僵尸猪灵，排除谓词可以用于跳过特定类型的盟友。

**未来改进**: typeId 只能匹配单一类型，而 MC Java 的 getClass() 在子类场景下有细微差异（例如 Drowned 是 Zombie 的子类，`getEntitiesOfClass(Zombie.class)` 不返回 Drowned）。如需更精确的类匹配，可引入 `getClassId()` 方法替代 typeId 比较。

### 8. ResetAngerGoal 模板约束

**问题**: 使用非 `IAngerable` 类型实例化 `ResetAngerGoal<T>` 导致编译错误。

**解决**: 确保实体类同时继承 `MobEntity` 和实现 `IAngerable` 接口。

### 7. OwnerHurtByTargetGoal/OwnerHurtTargetGoal 驯服检查

**问题**: 非驯服动物使用这些目标时 `shouldExecute()` 始终返回 `false`。

**原因**: 内部使用 `dynamic_cast<TameableEntity*>` 检查，非驯服动物转换失败。

**解决**: 仅在 `TameableEntity` 子类中使用这些目标。

### 8. isSuitableTarget 会拒绝同盟实体

**问题**: 期望某目标可攻击但 `isSuitableTarget()` 返回 false，且目标存活、非自己、非创造模式。

**原因**: `isSuitableTarget()` 现在通过 `isAlliedTo()` 检查队伍联盟关系，同盟实体会被拒绝。

**解决**: 如果需要攻击同盟实体，在自定义谓词中处理，或确认队伍配置正确。

### 9. wantsToAttack 过滤导致驯服动物不攻击

**问题**: OwnerHurtByTargetGoal/OwnerHurtTargetGoal 不攻击某些目标（如苦力怕、已驯服动物）。

**原因**: 这两个目标调用 `TameableEntity::wantsToAttack()` 进行攻击过滤。WolfEntity 重写了此方法，永远拒绝苦力怕/恶魂/盔甲架，且不攻击已驯服的同类和驯服动物。

**解决**: 这是 MC 原版行为，不是 bug。如需自定义过滤逻辑，在 TameableEntity 子类中重写 `wantsToAttack()`。

### 10. ResetAngerGoal 需要 UNIVERSAL_ANGER 规则

**问题**: ResetAngerGoal 的 `shouldExecute()` 始终返回 false。

**原因**: 必须启用 `UNIVERSAL_ANGER` 游戏规则，否则目标不会执行。

**解决**: 确认世界 `UNIVERSAL_ANGER` 游戏规则已启用。

### 11. setUnseenMemoryTicks 仅在 checkSight=true 时生效

**问题**: 设置了 `setUnseenMemoryTicks(300)` 但实体似乎没有延长追踪时间。

**原因**: `m_unseenMemoryTicks` 仅在 `checkSight=true` 时生效。当 `checkSight=false` 时，`shouldContinueExecuting()` 中的视线检查分支不会执行，因此记忆时间不生效。

**解决**: 确认目标选择器构造时传入了 `checkSight=true`。注意：MC 原版中唤魔者/幻术师对村民和铁傀儡的目标设置了 `setUnseenMemoryTicks(300)` 但 `checkSight=false`，此时视线记忆不生效，这是预期行为——设置它只是为了与 MC 原版代码保持一致。

### 12. chance 与 unseenMemoryTicks 的区别

**常见错误**: 将 `chance` 参数当作视线记忆时间使用。

- `chance`（构造参数）控制目标搜索频率：`chance=300` 表示每300tick才检查一次是否有新目标
- `unseenMemoryTicks`（`setUnseenMemoryTicks` 方法）控制失去视线后继续追踪的时间

两者语义完全不同：`chance` 影响目标获取，`unseenMemoryTicks` 影响目标保持。

### 13. 测试中直接构造的实体必须补 setTypeId

**问题**: 单元测试里 `std::make_unique<TestPigEntity>(...)` 直接构造实体后，`HurtByTargetGoal::shouldExecute()` 返回 false，`isSuitableTarget()` 也拒绝目标，但目标存活、非自身、非同盟、非创造模式，原因不明显。

**原因**: `isSuitableTarget()` 调 `target->entityType()`，若返回 `nullptr` 则直接判失败。`entityType()` 是懒查询，依赖 `m_typeId` 非空——而 `m_typeId` 仅在 `EntityType::create()`（注册表工厂）里通过 `setTypeId(m_name)` 设置。直接 `make_unique` 绕过工厂，`m_typeId` 默认空，`entityType()` 返回 `nullptr`，`canAttackType` 检查提前失败。生产路径下实体都经工厂创建，无此问题。

**解决**: 测试夹具里直接构造的实体，构造后补一句 `entity->setTypeId(entity::EntityTypeKeys::PIG);`（按实际类型）对齐生产路径。`alertAllies` 的同类匹配 `ally->entityType() == m_mob->entityType()` 同样依赖此点，盟友实体也需补。

