# 敌对生物模块

包含所有敌对生物（怪物）的实现。

## 目录结构

```
monster/
├── undead/         # 亡灵类（僵尸、骷髅、幻翼等）
├── arthropod/      # 节肢类（蜘蛛、蠹虫、末影螨等）
├── nether/         # 地狱生物（烈焰人、恶魂、猪灵等）
├── end/            # 末地生物（末影人、潜影贝）
├── basic/          # 基础怪物（苦力怕、史莱姆、巨人）
├── ocean/          # 海洋怪物（守卫者、远古守卫者）
└── illager/        # 灾厄村民（掠夺者、卫道士、唤魔者等）
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
                │   └── MagmaCubeEntity
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
```

**注意**: 上述继承链已按照 MC 1.16.5 修复：
- `MonsterEntity` -> `PatrollerEntity` -> `AbstractRaiderEntity` -> `AbstractIllagerEntity`
- `VindicatorEntity` 和 `PillagerEntity` 现在继承自 `AbstractIllagerEntity`
- `WitchEntity` 现在继承自 `AbstractRaiderEntity`

## 敌对行为

### 基础敌对目标优先级
| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | HurtByTargetGoal | 被攻击后反击 |
| 2 | NearestAttackableTargetGoal | 攻击最近目标 |
| 3 | MeleeAttackGoal | 近战攻击 |
| 4 | WaterAvoidingRandomWalkingGoal | 随机行走 |
| 5 | LookAtGoal | 看向玩家 |
| 6 | LookRandomlyGoal | 随机看向 |

## 子目录详细说明

### undead/ - 亡灵类
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| ZombieEntity | 僵尸 | 破门、召唤援军、转化为溺尸 |
| HuskEntity | 尸壳 | 沙漠僵尸、脱水效果 |
| DrownedEntity | 溺尸 | 水下僵尸、使用三叉戟 |
| ZombieVillagerEntity | 僵尸村民 | 虚弱药水+金苹果治愈、保留职业等级 |
| SkeletonEntity | 骷髅 | 远程攻击、阳光下燃烧 |
| StrayEntity | 流浪者 | 雪地骷髅、迟缓之箭 |
| WitherSkeletonEntity | 凋灵骷髅 | 凋灵效果攻击、高攻击力 |
| PhantomEntity | 幻翼 | 飞行攻击、夜间生成 |

### arthropod/ - 节肢类
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| SpiderEntity | 蜘蛛 | 攀爬墙壁、夜间攻击、光照检测 | ✅ shouldAttack() 光照检测已实现 |
| CaveSpiderEntity | 洞穴蜘蛛 | 中毒攻击、矿井生成 | ⏳ 框架完成 |
| SilverfishEntity | 蠹虫 | 躲在方块中、群体攻击 | ⏳ 框架完成 |
| EndermiteEntity | 末影螨 | 末影珍珠生成、末影人仇恨 | ⏳ 框架完成 |

### nether/ - 地狱生物
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| BlazeEntity | 烈焰人 | 火球攻击、飞行 | ✅ BlazeFireballAttackGoal 已实现 |
| GhastEntity | 恶魂 | 火球攻击、飞行、大碰撞箱 | ⏳ 框架完成 |
| MagmaCubeEntity | 岩浆怪 | 分裂、岩浆免疫 | ⏳ 框架完成 |
| PiglinEntity | 猪灵 | 交易、攻击玩家、惧怕灵魂火 | ⏳ 框架完成 |
| PiglinBruteEntity | 猪灵蛮兵 | 高攻击力、不交易 | ⏳ 框架完成 |
| HoglinEntity | 疣猪兽 | 可繁殖、转化为僵尸疣猪兽 | ⏳ 框架完成 |
| ZoglinEntity | 僵尸疣猪兽 | 攻击一切 | ⏳ 框架完成 |

#### 烈焰人 (BlazeEntity) 详细行为

**攻击模式** (MC 1.16.5):
1. **充能阶段**: 60 ticks (3秒)，进入燃烧状态
2. **火球阶段**: 连发最多 3 个小火球，每个间隔 6 ticks (0.3秒)
3. **冷却阶段**: 100 ticks (5秒)

**AI 目标**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 4 | BlazeFireballAttackGoal | 火球攻击 |
| 7 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
| 8 | LookAtGoal | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 被攻击反击，呼唤同伴 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击玩家 |

**属性值**:
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 20.0 |
| MOVEMENT_SPEED | 0.23 |
| ATTACK_DAMAGE | 6.0 |
| FOLLOW_RANGE | 48.0 |

### end/ - 末地生物
| 实体 | 说明 | 特殊行为 | 接口 |
|------|------|----------|------|
| EndermanEntity | 末影人 | 瞬移、搬方块、水伤 | IAngerable |
| ShulkerEntity | 潜影贝 | 贝壳防御、悬浮攻击 | - |

### basic/ - 基础怪物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| CreeperEntity | 苦力怕 | 爆炸、高压形态 |
| SlimeEntity | 史莱姆 | 分裂、跳跃攻击 |
| PhantomEntity | 幻翼 | 飞行攻击 |

### ocean/ - 海洋怪物
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| GuardianEntity | 守卫者 | 尖刺攻击、激光攻击、水中检测 | ✅ isInWater() 已实现 |
| ElderGuardianEntity | 远古守卫者 | 挖掘疲劳Boss、50格范围效果 | ✅ 挖掘疲劳效果已实现 |

### illager/ - 灾厄村民
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| AbstractIllagerEntity | 灾厄村民基类 | 手臂姿势状态 |
| VindicatorEntity | 卫道士 | 斧头近战攻击 |
| EvokerEntity | 唤魔者 | 尖牙攻击、召唤恼鬼 |
| IllusionerEntity | 幻术师 | 分身、失明攻击 |
| PillagerEntity | 掠夺者 | 弩远程攻击 |
| RavagerEntity | 劫掠兽 | 冲撞攻击、破坏方块 |
| VexEntity | 恼鬼 | 穿墙飞行、小碰撞箱 |
| WitchEntity | 女巫 | 药水攻击、治疗 |

## 属性值对齐状态

| 实体 | 属性 | MC 1.16.5 | 项目 | 状态 |
|------|------|-----------|------|------|
| VindicatorEntity | ATTACK_DAMAGE | 5.0 | 5.0 | 已修复 |
| VindicatorEntity | FOLLOW_RANGE | 12.0 | 12.0 | 已修复 |
| PillagerEntity | FOLLOW_RANGE | 32.0 | 32.0 | 已修复 |
| VexEntity | ATTACK_DAMAGE | 4.0 | 4.0 | 已修复 |
| EvokerEntity | MOVEMENT_SPEED | 0.5 | 0.5 | 已修复 |
| HoglinEntity | ATTACK_DAMAGE | 6.0 | 6.0 | 已修复 |
| ZoglinEntity | ATTACK_DAMAGE | 6.0 | 6.0 | 已修复 |
| PiglinBruteEntity | ATTACK_DAMAGE | 7.0 | 7.0 | 已修复 |

## 实现状态

所有目录下的实体均已实现基础框架。

### 已完成修复
- 灾厄村民继承层次已按 MC 1.16.5 对齐
- 属性值已按 MC 1.16.5 源码校准
- `AbstractRaiderEntity` 添加了 `RaiderState` 状态系统
- `AbstractIllagerEntity` 添加了 `ArmPose` 手臂姿势系统

### 待完成工作
- 各实体 AI 目标实现
- 投掷物实体（火球、箭矢等）
- DataParameter 网络同步

## 远程攻击

```cpp
// 远程攻击目标（骷髅）
class SkeletonEntity : public AbstractSkeletonEntity, public IRangedAttackMob {
public:
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override {
        // 发射箭矢
        auto arrow = spawnArrow(target, charge);
        world().spawnEntity(std::move(arrow));
    }
};
```

## 特殊行为

### 苦力怕
- 接近玩家后爆炸
- 被闪电击中变为高压苦力怕
- 怕猫/豹猫

### 末影人
- 被玩家注视时攻击
- 可以瞬移
- 搬运方块

### 史莱姆
- 死亡分裂成小史莱姆
- 只在特定区块生成

## 声音分类

- `MonsterEntity` 统一使用 `SoundCategory::Hostile`，保证敌对生物发声时进入正确的音量混音通道。
- 这条分类会被 `Entity::playSound(...)` 自动携带到世界层，不需要各个怪物再手工传递。

## 继承链修复记录

### 2026-05-02 灾厄村民继承层次修复

修复前的错误继承：
```
MonsterEntity -> AbstractIllagerEntity (错误!)
MonsterEntity -> PatrollerEntity -> AbstractIllagerEntity (错误!)
AbstractRaiderEntity -> PatrollerEntity (循环!)
```

修复后的正确继承（对齐 MC 1.16.5）：
```
MonsterEntity
  └── PatrollerEntity
        └── AbstractRaiderEntity
              ├── AbstractIllagerEntity
              │     ├── SpellcastingIllagerEntity
              │     │     ├── EvokerEntity
              │     │     └── IllusionerEntity
              │     ├── VindicatorEntity
              │     └── PillagerEntity
              ├── WitchEntity
              └── RavagerEntity
```

关键改动：
- `PatrollerEntity` 现在直接继承 `MonsterEntity`
- `AbstractRaiderEntity` 继承 `PatrollerEntity`
- `AbstractIllagerEntity` 继承 `AbstractRaiderEntity`
- `WitchEntity` 继承 `AbstractRaiderEntity`
- `VindicatorEntity` 和 `PillagerEntity` 继承 `AbstractIllagerEntity`
