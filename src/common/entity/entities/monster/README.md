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
MobEntity
└── CreatureEntity
    └── MonsterEntity (敌对生物基类)
        ├── AbstractSkeletonEntity (骷髅基类)
        │   ├── SkeletonEntity
        │   ├── StrayEntity
        │   └── WitherSkeletonEntity
        ├── ZombieEntity (僵尸)
        │   ├── HuskEntity
        │   ├── DrownedEntity
        │   ├── ZombieVillagerEntity
        │   └── ZombifiedPiglinEntity
        ├── SpiderEntity (蜘蛛)
        │   └── CaveSpiderEntity
        ├── CreeperEntity (苦力怕)
        ├── EndermanEntity (末影人)
        ├── SlimeEntity (史莱姆)
        │   └── MagmaCubeEntity
        └── AbstractIllagerEntity (灾厄村民基类)
            ├── VindicatorEntity
            ├── EvokerEntity
            ├── IllusionerEntity
            └── PillagerEntity
```

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
| ZombieVillagerEntity | 僵尸村民 | 可治愈 |
| ZombifiedPiglinEntity | 僵尸猪灵 | 中立、群体仇恨 |
| SkeletonEntity | 骷髅 | 远程攻击、阳光下燃烧 |
| StrayEntity | 流浪者 | 雪地骷髅、迟缓之箭 |
| WitherSkeletonEntity | 凋灵骷髅 | 凋灵效果攻击、高攻击力 |
| PhantomEntity | 幻翼 | 飞行攻击、夜间生成 |

### arthropod/ - 节肢类
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| SpiderEntity | 蜘蛛 | 攀爬墙壁、夜间攻击 |
| CaveSpiderEntity | 洞穴蜘蛛 | 中毒攻击、矿井生成 |
| SilverfishEntity | 蠹虫 | 躲在方块中、群体攻击 |
| EndermiteEntity | 末影螨 | 末影珍珠生成、末影人仇恨 |

### nether/ - 地狱生物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| BlazeEntity | 烈焰人 | 火球攻击、飞行 |
| GhastEntity | 恶魂 | 火球攻击、飞行、大碰撞箱 |
| MagmaCubeEntity | 岩浆怪 | 分裂、岩浆免疫 |
| PiglinEntity | 猪灵 | 交易、攻击玩家、惧怕灵魂火 |
| PiglinBruteEntity | 猪灵蛮兵 | 高攻击力、不交易 |
| HoglinEntity | 疣猪兽 | 可繁殖、转化为僵尸疣猪兽 |
| ZoglinEntity | 僵尸疣猪兽 | 攻击一切 |

### end/ - 末地生物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| EndermanEntity | 末影人 | 瞬移、搬方块、水伤 |
| ShulkerEntity | 潜影贝 | 贝壳防御、悬浮攻击 |

### basic/ - 基础怪物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| CreeperEntity | 苦力怕 | 爆炸、高压形态 |
| SlimeEntity | 史莱姆 | 分裂、跳跃攻击 |
| PhantomEntity | 幻翼 | 飞行攻击 |

### ocean/ - 海洋怪物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| GuardianEntity | 守卫者 | 尖刺攻击、激光攻击 |
| ElderGuardianEntity | 远古守卫者 | 挖掘疲劳Boss |

### illager/ - 灾厄村民
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| AbstractIllagerEntity | 灾厄村民基类 | - |
| VindicatorEntity | 卫道士 | 斧头近战攻击 |
| EvokerEntity | 唤魔者 | 尖牙攻击、召唤恼鬼 |
| IllusionerEntity | 幻术师 | 分身、失明攻击 |
| PillagerEntity | 掠夺者 | 弩远程攻击 |
| RavagerEntity | 劫掠兽 | 冲撞攻击、破坏方块 |
| VexEntity | 恼鬼 | 穿墙飞行、小碰撞箱 |
| WitchEntity | 女巫 | 药水攻击、治疗 |

## 实现状态

所有目录下的实体均已实现基础框架。

### 远程攻击

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
