# 敌对生物模块 (Monster)

包含所有敌对生物（怪物）的实现，是 Minecraft 中主动攻击玩家的生物类别。

## 目录结构

```
monster/
├── MonsterEntity.hpp/cpp          # 敌对生物基类（阳光燃烧、生成条件、敌对行为）
├── undead/                        # 亡灵类（僵尸、骷髅、幻翼等）
│   ├── AbstractSkeletonEntity.hpp/cpp  # 骷髅基类（远程/近战切换）
│   ├── SkeletonEntity.hpp/cpp          # 骷髅
│   ├── StrayEntity.hpp/cpp             # 流浪者
│   ├── WitherSkeletonEntity.hpp/cpp    # 凋灵骷髅
│   ├── ZombieEntity.hpp/cpp            # 僵尸基类（溺水转化、召唤援军）
│   ├── HuskEntity.hpp/cpp              # 尸壳
│   ├── DrownedEntity.hpp/cpp           # 溺尸
│   ├── ZombieVillagerEntity.hpp/cpp    # 僵尸村民（治愈系统）
│   └── README.md
├── arthropod/                     # 节肢类（蜘蛛、蠹虫、末影螨等）
│   ├── SpiderEntity.hpp/cpp            # 蜘蛛（攀爬、光照敏感攻击）
│   ├── CaveSpiderEntity.hpp/cpp        # 洞穴蜘蛛（中毒攻击）
│   ├── EndermiteEntity.hpp/cpp         # 末影螨（自动消失）
│   └── README.md
├── nether/                        # 地狱生物（烈焰人、恶魂、猪灵等）
│   ├── BlazeEntity.hpp/cpp            # 烈焰人（火球攻击）
│   ├── NetherEntities.hpp/cpp         # 恶魂、岩浆怪、猪灵、疣猪兽等
│   └── README.md
├── end/                           # 末地生物（末影人、潜影贝）
│   ├── EndermanEntity.hpp/cpp         # 末影人（瞬移、搬方块）
│   ├── ShulkerEntity.hpp/cpp          # 潜影贝（贝壳防御、悬浮攻击）
│   └── README.md
├── basic/                         # 基础怪物（苦力怕、史莱姆、巨人、幻翼）
│   ├── CreeperEntity.hpp/cpp          # 苦力怕（爆炸、高压形态）
│   ├── SlimeEntity.hpp/cpp            # 史莱姆（分裂机制）
│   ├── GiantEntity.hpp/cpp            # 巨人（无AI）
│   ├── PhantomEntity.hpp/cpp          # 幻翼（飞行攻击）
│   └── README.md
├── ocean/                         # 海洋怪物（守卫者、远古守卫者）
│   ├── GuardianEntity.hpp/cpp         # 守卫者（激光攻击）
│   ├── ElderGuardianEntity.hpp/cpp    # 远古守卫者（挖掘疲劳）
│   └── README.md
├── illager/                       # 灾厄村民（掠夺者、卫道士、唤魔者等）
│   ├── AbstractIllagerEntity.hpp/cpp  # 灾厄村民基类（手臂姿势）
│   ├── AbstractRaiderEntity.hpp/cpp   # 袭击者基类（袭击参与）
│   ├── PatrollerEntity.hpp/cpp        # 巡逻者基类（巡逻目标）
│   ├── SpellcastingIllagerEntity.hpp/cpp  # 施法灾厄村民基类
│   ├── EvokerEntity.hpp/cpp           # 唤魔者（尖牙攻击、召唤恼鬼）
│   ├── IllusionerEntity.hpp/cpp       # 幻术师（分身、失明攻击）
│   ├── IllagerEntities.hpp/cpp        # 掠夺者、卫道士
│   ├── RavagerEntity.hpp/cpp          # 劫掠兽（冲撞攻击）
│   ├── VexEntity.hpp/cpp              # 恼鬼（穿墙飞行）
│   ├── WitchEntity.hpp/cpp            # 女巫（药水攻击）
│   └── README.md
├── breeze/                        # 旋风人（1.20+）
│   ├── BreezeEntity.hpp/cpp           # 旋风人（风弹攻击、偏转投射物、滑行）
│   └── README.md
└── README.md
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── MonsterEntity (敌对生物基类)
                ├── AbstractSkeletonEntity (骷髅基类)
                │   ├── SkeletonEntity
                │   ├── StrayEntity
                │   └── WitherSkeletonEntity
                ├── ZombieEntity (僵尸)
                │   ├── HuskEntity
                │   ├── DrownedEntity
                │   └── ZombieVillagerEntity
                ├── SpiderEntity (蜘蛛)
                │   └── CaveSpiderEntity
                ├── CreeperEntity (苦力怕)
                ├── EndermanEntity (末影人)
                ├── SlimeEntity (史莱姆)
                │   └── MagmaCubeEntity (在 nether/ 目录)
                └── PatrollerEntity (巡逻者基类)
                    └── AbstractRaiderEntity (袭击者基类)
                        ├── AbstractIllagerEntity (灾厄村民基类)
                        │   ├── SpellcastingIllagerEntity (施法灾厄村民基类)
                        │   │   ├── EvokerEntity
                        │   │   └── IllusionerEntity
                        │   ├── VindicatorEntity
                        │   └── PillagerEntity
                        ├── WitchEntity
                        └── RavagerEntity

VexEntity (恼鬼) 独立继承自 MonsterEntity
PhantomEntity (幻翼) 继承自 FlyingEntity（非直接继承 MonsterEntity）
```

## 内部模块关系

- **MonsterEntity** 是所有敌对生物的基类，提供阳光燃烧、生成位置检查、敌对目标选择等基础设施
- **undead/**：亡灵类共享治疗药水伤害、伤害药水治疗、免疫中毒等特性
- **arthropod/**：节肢类共享蜘蛛的攀爬能力、光照敏感攻击逻辑
- **nether/**：下界生物多数具有火焰免疫，猪灵系有复杂的交易/仇恨机制
- **end/**：末地生物有独特的行为（瞬移、贝壳防御）
- **basic/**：独立怪物，无子类或简单继承
- **ocean/**：守卫者系有独特的激光攻击和目标选择（玩家+鱿鱼）
- **illager/**：灾厄村民有完整的继承层次（巡逻→袭击→灾厄村民→施法者），参与袭击系统

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖模块 | 用途 |
|---------|------|
| `entity/core` | Entity, LivingEntity, MobEntity, CreatureEntity 基类 |
| `entity/interfaces` | IMob, IAngerable, IRangedAttackMob, ICrossbowUser, IFlinging 等接口 |
| `entity/ai/goal` | AI 目标系统（SwimGoal, MeleeAttackGoal, LookAtGoal 等） |
| `entity/ai/goal/goals/special` | 特殊 AI 目标（EndermanGoals, BlazeFireballAttackGoal, GuardianAttackGoal 等） |
| `entity/attribute` | Attributes 属性系统 |
| `entity/damage` | DamageSource 伤害系统 |
| `entity/effect` | 药水效果系统 |
| `world/` | IWorld, BlockPos, BlockState 等世界接口 |
| `core/Types` | 基础类型定义 |

### 下游依赖（依赖本模块）

| 依赖方 | 用途 |
|-------|------|
| `entity/core/VanillaEntities` | 实体类型注册 |
| `world/spawn` | 生物生成系统（怪物生成规则、光照检查） |
| `world/village/raid` | 袭击系统（灾厄村民参与袭击） |
| `client/renderer/entity` | 实体渲染器 |
| `server/network` | 实体状态同步 |

## 容易踩的坑

### 继承链与接口

1. **VexEntity 不继承 AbstractIllagerEntity**：恼鬼独立继承自 MonsterEntity，由唤魔者召唤，不能作为灾厄村民处理
2. **WitchEntity 不继承 AbstractIllagerEntity**：女巫继承 AbstractRaiderEntity，不是灾厄村民的子类
3. **PhantomEntity 继承链**：`Entity → LivingEntity → MobEntity → FlyingEntity → PhantomEntity`，不是直接继承 MonsterEntity
4. **MagmaCubeEntity 在 nether/ 目录**：继承自 basic/ 的 SlimeEntity，跨目录继承

### AI 目标注册

5. **registerGoals() 调用链**：子类必须调用直接父类的 `registerGoals()`，不能跳级调用 `MonsterEntity::registerGoals()`
6. **MonsterEntity 不注册 HurtByTargetGoal**：MC 原版 Monster 类不注册此目标，由各子类按需自行注册。不同怪物有不同的被攻击反击行为（如僵尸不警醒僵尸猪灵、灾厄村民不反击同类、溺尸不反击同类溺尸等）
7. **战斗目标动态切换**：AbstractSkeletonEntity 使用 `setCombatTask()` 根据装备动态选择远程/近战目标，不是在 `registerGoals()` 中静态注册
8. **优先级数字越小越优先**：0 是最高优先级，不能搞反
9. **DrownedEntity 替换父类 HurtByTargetGoal**：DrownedEntity::registerGoals() 先调用父类再通过 `removeGoalsOfType<HurtByTargetGoal>()` 移除父类注册的版本，然后添加溺尸专用的版本（排除同类溺尸 + 不警醒僵尸猪灵）

### 光照与生成

10. **光照等级 vs 亮度值**：`getLightSubtracted()` 返回 0-15 光照等级，`getBrightness()` 返回 0.0-1.0 亮度值。怪物生成条件是光照等级 < 7（约 0.47 亮度）
11. **canMonsterSpawnInLight vs canMonsterSpawn**：前者检查光照，后者不检查（用于刷怪笼）
12. **阳光燃烧检查**：`shouldBurnInDaylight()` 默认返回 `true`，蜘蛛等非亡灵类怪物需要重写返回 `false`

### 特殊行为

13. **末影人投射物伤害瞬移**：末影人对投射物伤害尝试瞬移躲避（最多 64 次），成功则不受伤。需要在 `hurt()` 中处理
14. **潜影贝护甲计算**：闭合时额外 +20 护甲，伤害计算时需考虑此状态
15. **史莱姆分裂时机**：分裂必须在 `remove()` 中执行，而不是 `die()` 中
16. **僵尸溺水转化**：`convertToDrowned()` 需要保留位置、生命值比例、装备、婴儿状态、自定义名称、持久化状态
17. **女巫药水逻辑**：`tick()` 中自动检测是否需要喝药水，喝药水期间 `canRangedAttack()` 返回 false
18. **恼鬼穿墙实现**：`tick()` 中先 `setNoClip(true)`，调用父类 tick，再 `setNoClip(false)`

### 袭击系统

19. **Raid 指针管理**：`AbstractRaiderEntity` 持有 `Raid*` 指针，参与袭击时设置，离开时清空，需注意悬空指针
20. **死亡通知顺序**：`die()` 中必须先通知 Raid 再调用父类 die()，顺序很重要

### 目录与子目录 README

各子目录（undead/、arthropod/、nether/、end/、basic/、ocean/、illager/）都有独立的 README.md，详细说明了各自模块的特性。修改这些模块时务必先阅读对应的 README。
