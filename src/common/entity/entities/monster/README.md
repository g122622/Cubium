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
| SilverfishEntity | 蠹虫 | 躲在方块中、群体攻击 | ✅ 基础AI目标已实现，召唤同伴预留 |
| EndermiteEntity | 末影螨 | 末影珍珠生成、自动消失 | ✅ 消失逻辑和AI目标已完整实现 |

#### 末影螨 (EndermiteEntity) 详细实现

**消失逻辑** (MC 1.16.5):
- 非持久化末影螨在 2400 ticks（2分钟）后自动消失
- 使用 `MobEntity::isNoDespawnRequired()` 检查持久化状态
- 命名牌命名的末影螨不会消失

**AI 目标**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳 |
| 2 | MeleeAttackGoal | 近战攻击玩家 |
| 3 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
| 7 | LookAtGoal | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 受伤反击 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

**属性值**:
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 8.0 |
| MOVEMENT_SPEED | 0.25 |
| ATTACK_DAMAGE | 2.0 |
| EXPERIENCE_VALUE | 3 |

#### 蠹虫 (SilverfishEntity) 详细实现

**AI 目标**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳 |
| 4 | MeleeAttackGoal | 近战攻击玩家 |
| 5 | WaterAvoidingRandomWalkingGoal | 避水随机行走 |
| 7 | LookAtGoal | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal(setCallsForHelp=true) | 受伤反击并呼唤同伴 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

**预留功能**:
- `m_summonCooldown`: 召唤同伴倒计时
- `notifySummonCooldown()`: 受伤时触发召唤冷却
- TODO: SilverfishHideInStoneGoal（藏入石头）
- TODO: SilverfishSummonOthersGoal（召唤同伴）

**属性值**:
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 8.0 |
| MOVEMENT_SPEED | 0.25 |
| ATTACK_DAMAGE | 1.0 |
| EXPERIENCE_VALUE | 5 |

### nether/ - 地狱生物
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| BlazeEntity | 烈焰人 | 火球攻击、飞行 | ✅ BlazeFireballAttackGoal 已实现 |
| GhastEntity | 恶魂 | 火球攻击、飞行、大碰撞箱 | ✅ shootFireball() 已实现 |
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
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| EndermanEntity | 末影人 | 瞬移、搬方块、水伤 | ⏳ 框架完成 |
| ShulkerEntity | 潜影贝 | 贝壳防御、悬浮攻击、瞬移 | ✅ 完整实现 |

#### 潜影贝 (ShulkerEntity) 详细实现

潜影贝是生活在末地城市的敌对生物，具有独特的贝壳防御机制和悬浮攻击能力。

**核心特性**:
| 特性 | 值 |
|------|-----|
| 宽度/高度 | 1.0f / 1.0f |
| 最大生命值 | 30.0 |
| 移动速度 | 0.0 (不移动) |
| 跟随范围 | 18.0 |
| 护甲加成 | 闭合时 +20 |
| 经验值 | 5 |

**贝壳状态**:
```cpp
enum class ShellState : u8 {
    Closed = 0,  // 闭合
    Opening = 1, // 正在打开
    Open = 2,    // 打开
    Closing = 3  // 正在关闭
};
```

**AI 目标**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | LookAtGoal | 看向玩家(8格) |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 被攻击反击，呼唤同伴 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

**攻击机制** (MC 1.16.5):
- 发射 ShulkerBulletEntity 追踪子弹
- 子弹命中造成 4 点伤害 + 10 秒漂浮效果
- 攻击冷却: 20-70 ticks

**瞬移机制** (MC 1.16.5):
- 受伤后血量低于 50% 有 25% 概率瞬移
- 瞬移范围: ±8 格
- 瞬移尝试次数: 5 次
- 需要找到有效的附着方块

**防御机制**:
- 闭合时免疫投射物伤害（箭矢、三叉戟、火球等）
- 闭合时获得 +20 护甲加成
- 贝壳开合动画时间: 20 ticks

**颜色系统**:
- 支持 16 种颜色（紫色为默认）
- 可通过 NBT 或命令设置颜色

**参考**: MC 1.16.5 ShulkerEntity

### basic/ - 基础怪物
| 实体 | 说明 | 特殊行为 |
|------|------|----------|
| CreeperEntity | 苦力怕 | 爆炸、高压形态 |
| SlimeEntity | 史莱姆 | 分裂、跳跃攻击 |
| PhantomEntity | 幻翼 | 飞行攻击 |

### ocean/ - 海洋怪物
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| GuardianEntity | 守卫者 | 激光攻击、目标选择(玩家+鱿鱼)、尖刺动画 | ✅ 完整实现：GuardianAttackGoal、NearestAttackableTargetGoal |
| ElderGuardianEntity | 远古守卫者 | 挖掘疲劳Boss、50格范围效果 | ✅ 挖掘疲劳效果已实现 |

#### 守卫者 (GuardianEntity) 详细实现

**目标选择配置** (MC 1.16.5):
- 使用 `NearestAttackableTargetGoal<LivingEntity>` 搜索攻击目标
- 自定义谓词筛选: 只攻击 Player 和 Squid
- 距离筛选: 目标必须距离 > 3 格
- 视线检查: 需要 `canSee()` 通过

**行为目标优先级**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 4 | GuardianAttackGoal | 激光攻击 (充能60tick + 冷却20tick) |
| 7 | RandomWalkingGoal | 随机漫步 (间隔80tick) |
| 8 | LookAtGoal | 看向玩家(8格) / 看向同类守卫者(12格,1%) |
| 9 | LookRandomlyGoal | 随机看向 |

**激光攻击流程**:
1. `shouldExecute()` - 检查 attackTarget() 或 selectTarget()
2. `startExecuting()` - 初始化充能状态
3. `tick()` - 充能 60 tick 后发射激光
4. `performLaserAttack()` - 造成 4.0 魔法伤害

**参考**: `src/common/entity/entities/monster/ocean/README.md`

### illager/ - 灾厄村民
| 实体 | 说明 | 特殊行为 | 实现状态 |
|------|------|----------|---------|
| AbstractIllagerEntity | 灾厄村民基类 | 手臂姿势状态 | ✅ 完成 |
| VindicatorEntity | 卫道士 | 斧头近战攻击 | ✅ 属性已修复 |
| EvokerEntity | 唤魔者 | 尖牙攻击、召唤恼鬼 | ✅ 属性已修复 |
| IllusionerEntity | 幻术师 | 分身、失明攻击 | ⏳ 框架完成 |
| PillagerEntity | 掠夺者 | 弩远程攻击 | ✅ 属性已修复 |
| RavagerEntity | 劫掠兽 | 冲撞攻击、破坏方块 | ⏳ 框架完成 |
| VexEntity | 恼鬼 | **穿墙飞行**、有限生命 | ✅ 完成 |
| WitchEntity | 女巫 | 药水攻击、治疗 | ✅ 完成 |

#### 恼鬼 (VexEntity) 详细实现

恼鬼是由唤魔者召唤的小型飞行敌对生物，具有独特的穿墙能力。

**核心特性**:
| 特性 | 值 |
|------|-----|
| 宽度/高度 | 0.4f / 0.8f |
| 最大生命值 | 14.0 |
| 攻击伤害 | 4.0 (铁剑) |
| 飞行能力 | canFly() = true |
| 日光燃烧 | shouldBurnInDaylight() = false |

**穿墙实现** (MC 1.16.5 VexEntity.tick() 行 62-71):
```cpp
void VexEntity::tick() {
    setNoClip(true);          // 启用穿墙
    MonsterEntity::tick();
    setNoClip(false);         // 关闭穿墙
    setNoGravity(true);       // 始终无重力

    // 有限生命机制
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;
        if (m_lifeTime <= 0) {
            m_lifeTime = 20;  // 重置防止连续伤害
            hurt(DamageSources::starve(), 1.0f);
        }
    }
}
```

详细文档见 `illager/README.md`

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
