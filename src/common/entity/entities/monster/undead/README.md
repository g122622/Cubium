# 亡灵生物模块

包含所有亡灵类敌对生物的实现。亡灵生物共享以下特性：
- 在阳光下燃烧（部分变种除外）
- 受到治疗药水伤害
- 受到伤害药水治疗
- 免疫中毒和凋零效果
- 被亡灵杀手附魔造成额外伤害

## 目录结构

```
undead/
├── AbstractSkeletonEntity.hpp/cpp  # 骷髅基类（远程/近战切换）
├── SkeletonEntity.hpp/cpp          # 骷髅（弓箭远程攻击）
├── StrayEntity.hpp/cpp             # 流浪者（迟缓之箭）
├── WitherSkeletonEntity.hpp/cpp    # 凋灵骷髅（石剑近战、凋零效果）
├── ZombieEntity.hpp/cpp            # 僵尸基类（溺水转化、召唤援军）
├── HuskEntity.hpp/cpp              # 尸壳（沙漠变种、脱水攻击）
├── DrownedEntity.hpp/cpp           # 溺尸（水中生成、三叉戟远程攻击、日间避阳、夜间登陆）
└── ZombieVillagerEntity.hpp/cpp    # 僵尸村民（治愈系统）
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── MonsterEntity
                ├── AbstractSkeletonEntity (骷髅基类)
                │   ├── SkeletonEntity
                │   ├── StrayEntity
                │   └── WitherSkeletonEntity
                └── ZombieEntity (僵尸基类)
                    ├── HuskEntity
                    ├── DrownedEntity
                    └── ZombieVillagerEntity
```

## 内部模块关系

- **AbstractSkeletonEntity**：骷髅系基类，实现 `IRangedAttackMob` 接口，采用 `setCombatTask()` 模式动态选择远程/近战目标。目标选择器注册了铁傀儡攻击目标和幼年海龟攻击目标（BABY_ON_LAND_SELECTOR 过滤：仅攻击 `isChild() && !isInWater()` 的海龟）
- **ZombieEntity**：僵尸系基类，实现溺水转化、增援召唤、破门能力、婴儿状态、生成初始化
- **ZombieVillagerEntity**：继承 ZombieEntity，实现治愈系统（铁栏杆/床加速、力量效果加速）

各实体通过重写父类方法实现差异化行为：
- `shouldBurnInDaylight()`：HuskEntity/DrownedEntity 返回 false
- `shouldDrown()`：ZombieVillagerEntity 返回 false，DrownedEntity 返回 false
- `canSpawnInLiquids()`：DrownedEntity 返回 true（增援生成时允许在水中生成），其他僵尸返回 false
- `setCombatTask()`：WitherSkeletonEntity 重写使用近战，其他骷髅使用远程
- `finalizeSpawn()`：ZombieEntity 覆写，设置破门能力（概率 = specialMultiplier × 0.1）、万圣节南瓜头（10月31日 25% 概率）、属性修饰符（随机增援概率、击退抗性、跟随范围、领袖僵尸判定）；DrownedEntity 覆写，在父类之后随机决定是否手持三叉戟（约 6.25%）
- `populateDefaultEquipmentSlots()`：ZombieEntity 覆写，Hard 难度 5%/其他 1% 概率生成铁剑或铁锹

## 上下游外部依赖关系

### 上游依赖

| 依赖模块 | 用途 |
|---------|------|
| `entity/core` | Entity, LivingEntity, MobEntity, CreatureEntity, MonsterEntity 基类 |
| `entity/interfaces` | IRangedAttackMob 远程攻击接口 |
| `entity/ai/goal` | AI 目标系统（MeleeAttackGoal, RangedBowAttackGoal, BreakDoorGoal 等） |
| `entity/attribute` | Attributes 属性系统 |
| `entity/damage` | DamageSource 伤害系统 |
| `entity/effect` | 药水效果系统（饥饿、凋零、迟缓等） |
| `entity/entities/villager` | VillagerData 村民数据（职业、等级、经验） |
| `world/` | IWorld, BlockPos, BlockState 等世界接口 |

### 下游依赖

| 依赖方 | 用途 |
|-------|------|
| `entity/core/VanillaEntities` | 实体类型注册 |
| `world/spawn` | 怪物生成系统 |
| `client/renderer/entity` | 实体渲染器 |

## 容易踩的坑

### 骷髅类战斗目标切换

1. **setCombatTask() 调用时机**：`registerGoals()` 只注册非战斗目标，`setCombatTask()` 在构造函数末尾、`finalizeSpawn()`、装备变更（`setEquipment`）时调用，根据装备动态添加战斗目标
2. **战斗目标优先级**：战斗目标使用优先级 4（`COMBAT_GOAL_PRIORITY`），确保非战斗目标（游泳、看向等）优先执行
3. **装备检查逻辑**：`setCombatTask()` 通过 `getWeaponHoldingHand()` 检查主手/副手是否持有弓，持弓注册 `RangedBowAttackGoal`，不持弓注册 `MeleeAttackGoal`
4. **战斗目标所有权**：每次 `setCombatTask()` 调用时创建新的 Goal 对象并通过 `make_unique` 转移所有权给 `GoalSelector`，使用 `removeGoalsOfType<>()` 移除旧目标。不要使用 `unique_ptr` 成员 + `addGoal(priority, ptr.get())` 的模式，因为这会导致双重所有权（GoalSelector 和 unique_ptr 同时拥有同一个 Goal 对象）
5. **canUseNonMeleeWeapon()**：判断物品是否为远程武器，默认检查 `UseAction::Bow`，凋灵骷髅重写返回 false
6. **WitherSkeletonEntity 特殊处理**：凋灵骷髅重写 `setCombatTask()` 强制使用近战，`canUseNonMeleeWeapon()` 返回 false
7. **远程箭矢伤害**：骷髅类使用 `arrow->setBaseDamageFromMob(charge)` 设置箭矢伤害，公式为 `power * 2.0 + triangle(difficulty * 0.11, 0.57425)`，与 AbstractArrowEntity 基类保持一致。不调用 `applyBowEnchantments()`，因为生物射出的箭矢不应有弓类附魔效果。
8. **难度相关攻击间隔**：`setCombatTask()` 根据游戏难度调整 `RangedBowAttackGoal` 的最小攻击间隔：
   - 困难难度：使用 `getHardAttackInterval()`（普通骷髅/流浪者 = 20 ticks，沼骸骷髅 = 50 ticks）
   - 其他难度：使用 `getAttackInterval()`（普通骷髅/流浪者 = 40 ticks，沼骸骷髅 = 70 ticks）
   - 子类可重写 `getHardAttackInterval()` / `getAttackInterval()` 提供不同间隔值
   - 对应 MC 原版 `AbstractSkeleton.reassessWeaponGoal()` 中的 `bowGoal.setMinAttackInterval()` 逻辑

### 僵尸类溺水转化

4. **shouldDrown() 重写**：HuskEntity 返回 true（先变僵尸再变溺尸），DrownedEntity 返回 false（已溺尸），ZombieVillagerEntity 返回 false（不会变溺尸）
5. **convertToDrowned() 数据保留**：必须保留位置、生命值比例、装备、婴儿状态、自定义名称、持久化状态，清空原僵尸装备防止死亡掉落
6. **转化时间常量**：水下 600 ticks（30秒）开始转化，转化持续 300 ticks（15秒）

### 僵尸类增援系统

15. **增援入口**：`trySummonReinforcements(LivingEntity* explicitTarget = nullptr)` 是增援逻辑的唯一公共入口。`hurt()` 通过此方法触发增援，传入伤害来源实体作为显式目标。不带参数调用时使用当前攻击目标（`attackTarget()`）
16. **增援前置条件**：困难模式（`DifficultyHelper::canZombieReinforce()`）、增援概率属性（`zombie.spawn_reinforcements`）、`doMobSpawning` 游戏规则
17. **增援生成逻辑**（`_trySpawnReinforcement()`）：50 次随机位置尝试，偏移公式 = `nextInt(7,40) * nextInt(-1,1)`（MC 1.21.11 原版，偏移可为 0），使用 `EntitySpawnPlacementRegistry::canSpawnEntity()` 检查生成位置有效性（对应 MC 的 `SpawnPlacements.isSpawnPositionOk()` + `checkSpawnRules()`），附近无玩家检查（7格）、实体碰撞检查、AABB 液体检查（`IWorld::containsAnyLiquid()`，溺尸通过 `canSpawnInLiquids()` 豁免）
18. **属性修饰符**：每次增援成功后，召唤者获得 `reinforcement_caller_charge`（-0.05 Addition，累加），被召唤者获得 `reinforcement_callee_charge`（-0.05 Addition），防止连锁增援

### 僵尸村民治愈系统

7. **治愈时间**：基础时间 3600-6000 ticks（3-5分钟随机），使用 `startConverting()` 时可指定时间或 -1 随机
8. **加速检测范围**：4x4x4 范围内的铁栏杆和床，每个有 30% 概率加速，最多检测 14 个方块
9. **床加速特性**：所有 16 种颜色的床都有效，占用状态不影响，头部和脚部都有效
10. **消失规则**：正在治愈或有经验的僵尸村民不能消失，见 `canDespawn()` 实现
11. **治愈时装备处理**（对应 MC Java 的 `dropPreservedEquipment` 逻辑）：
    - 使用 `MobEntity::dropPreservedEquipment()` 方法，谓词为"物品无绑定诅咒"
    - 有绑定诅咒的装备 → 保留在实体上，后续转移到村民的对应槽位
    - 无绑定诅咒且掉落概率 > 1.0（保留状态，如南瓜头）→ 在实体位置掉落物品
    - 无绑定诅咒且掉落概率 ≤ 1.0（默认 8.5%）→ 物品静默消失
    - 万圣节南瓜头掉落概率为 0.0，既不会掉落也不会转移

### 属性与尺寸

11. **婴儿僵尸**：通过 `isBaby()` 检查，尺寸和眼睛高度不同，速度有 50% 加成
12. **步进高度**：DrownedEntity 步进高度为 1.0f，可走上完整方块

### 数据同步

13. **ZombieVillagerEntity 数据参数**：`CONVERTING_PARAM`、`VILLAGER_TYPE_PARAM`、`VILLAGER_PROFESSION_PARAM`、`VILLAGER_LEVEL_PARAM` 需要在 `registerData()` 中注册
14. **客户端同步**：`syncMetadataFromDataManager()` 需要从 DataManager 读取数据更新本地状态

### 溺尸游泳状态追踪

对应 MC 1.21.11 `DrownedEntity.updateSwimming` 与 `DrownedRenderer.getArmPose`：

- **`DrownedEntity::updateSwimming()`** 覆盖（服务端 tick 中调用）：按 `areEyesInWater() && isInWater() && wantsToSwim() && !isRiding()` 条件设置 `EntityFlags::Swimming` 位（经 `DATA_FLAGS_PARAM` slot 0 同步到客户端）。与普通僵尸/玩家的 `updateSwimming`（基于 Pose）不同，溺尸直接基于环境条件判定。
- **`DrownedEntity::isVisuallySwimming()`** 覆盖：返回 `isSwimming() && !isRiding()`（基于 Swimming 标志位，不依赖 Pose，对应 MC 1.21.11 `Drowned.isVisuallySwimming`）。
- **客户端 `ClientEntity`**：`syncMetadataFromDataManager` 读取 `DATA_FLAGS_PARAM` 的 Swimming 位 → `setSwimming`；每帧 `updateSwimAmount` 渐变 `m_swimAmount`（±0.09/tick），`getInterpolatedSwimAmount(partialTicks)` 提供 prev/cur 插值。
- **渲染管道**：`EntityRendererManager::_applyDrownedTridentPose` 在 `_applyZombieState` 之前根据 `isAggressive()` 设置右臂 `ThrowSpear` 姿态；`_applyZombieState` 推送 `setSwimAnimation(swimAmount)` 并重新调用 `setAngles`，`DrownedModel::setAngles` 据此执行游泳手臂/腿部/头部覆盖（详见 `src/client/renderer/trident/entity/model/monster/README.md` 的溺尸游泳动画管道章节）。
- **未实现项**：MC 1.21.11 `DrownedRenderer.setupRotations` 的游泳身体倾斜（渲染器层整体 X 轴旋转）暂留 TODO，见 `MonsterVariantRenderers.hpp` 中 DrownedRenderer 类注释。
