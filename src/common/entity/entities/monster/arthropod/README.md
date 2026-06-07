# 节肢类怪物模块

包含所有节肢类敌对生物的实现。

## 目录结构

```
arthropod/
├── SpiderEntity.hpp/cpp      # 蜘蛛实体，光照敏感攻击、攀爬能力
├── CaveSpiderEntity.hpp/cpp  # 洞穴蜘蛛，继承蜘蛛，中毒攻击
├── EndermiteEntity.hpp/cpp   # 末影螨和蠹虫实体定义
└── README.md                 # 本文档
```

## 内部模块关系

```
MonsterEntity (敌对生物基类)
    └── SpiderEntity (蜘蛛)
            └── CaveSpiderEntity (洞穴蜘蛛)
    └── EndermiteEntity (末影螨)
    └── SilverfishEntity (蠹虫)
```

- **SpiderEntity**：提供攀爬能力（`isClimbing()`）和光照敏感攻击逻辑（`shouldAttack()`）
- **CaveSpiderEntity**：继承 SpiderEntity，添加中毒攻击，覆盖 `attackEntityAsMob()`
- **EndermiteEntity**：独立实体，实现消失逻辑（非持久化个体 2400 ticks 后消失）
- **SilverfishEntity**：独立实体，实现召唤同伴和藏入石头行为

## 上下游外部依赖关系

### 依赖的模块

- `entity/ai/goal/`：AI 目标系统（SwimGoal、MeleeAttackGoal、LookAtGoal、HurtByTargetGoal 等）
- `entity/ai/goal/goals/special/SilverfishGoals.hpp`：蠹虫专用目标（SilverfishHideInStoneGoal、SilverfishSummonOthersGoal）
- `entity/attribute/Attributes.hpp`：属性系统（MAX_HEALTH、MOVEMENT_SPEED、ATTACK_DAMAGE）
- `entity/core/MobEntity.hpp`：Mob 基类（持久化检查 `isNoDespawnRequired()`）
- `world/IWorld.hpp`：世界接口（光照查询 `getLightSubtracted()`）
- `entity/damage/DamageSource.hpp`：伤害来源

### 被依赖的模块

- `entity/core/EntityRegistry.hpp`：实体注册（工厂方法 `create()`）
- `world/spawn/`：生物生成系统

## 容易踩的坑

### 光照检测单位混淆

`IWorld::getLightSubtracted()` 返回的是 0-15 的光照等级，而 `SpiderEntity::shouldAttack()` 检查的是 `lightLevel < 7`。MC 中光照等级 < 7 对应"足够黑暗"。不要混淆光照等级（0-15）和亮度值（0.0-1.0），后者是 `getBrightness()` 返回的。

### SpiderAttackGoal 中的亮度判断

`SpiderAttackGoal::shouldContinueExecuting()` 使用的是 `getBrightness()`（返回 0.0-1.0），判断条件是 `brightness >= 0.5F`。这与 `shouldAttack()` 中的 `lightLevel < 7` 是一致的（7/15 ≈ 0.47），但代码中的判断逻辑略有不同，修改时需注意两个方法的行为一致性。

### 末影螨消失逻辑

消失逻辑在 `tick()` 中执行，检查 `isNoDespawnRequired()`。只有非持久化的末影螨才会消失（如末影人瞬移生成的）。命名牌命名的末影螨不会消失，因为它会设置持久化标记。

### 蠹虫召唤同伴的触发时机

`SilverfishEntity::hurt()` 中检查 `source.isEntitySource() || source.isMagic()` 才触发召唤。这意味着摔落伤害、窒息伤害等不会触发召唤同伴。修改伤害来源判断时需谨慎。

### CaveSpiderEntity 的中毒持续时间

中毒持续时间取决于游戏难度，默认值 `m_poisonDuration = 7` 是普通难度的值。`attackEntityAsMob()` 中需要从世界获取难度来决定实际持续时间：
- 简单：不中毒
- 普通：7 秒
- 困难：15 秒

### 攀爬状态同步

`SpiderEntity::tick()` 中设置 `m_climbing = collidedHorizontally()`。这意味着攀爬状态每帧都会更新，依赖于 `collidedHorizontally()` 的正确性。如果实体碰撞检测有问题，攀爬行为也会异常。

### 蜘蛛不在阳光下燃烧

`SpiderEntity::shouldBurnInDaylight()` 返回 `false`，这是蜘蛛特有的行为。亡灵类怪物（僵尸、骷髅）会燃烧，但蜘蛛不会。创建新节肢类实体时需要明确决定是否燃烧。

### registerAttributes() 需要显式调用

在构造函数中，`registerAttributes()` 需要显式调用，因为基类构造函数中调用不会派发到子类。代码中有注释说明这一点。新增实体时务必遵循此模式。
