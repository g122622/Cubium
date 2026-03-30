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
