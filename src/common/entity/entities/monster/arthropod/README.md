# 节肢类怪物模块

包含所有节肢类敌对生物的实现。

## 目录结构

```
arthropod/
├── SpiderEntity.hpp/cpp      # 蜘蛛
├── CaveSpiderEntity.hpp/cpp  # 洞穴蜘蛛
├── EndermiteEntity.hpp/cpp   # 末影螨、蠹虫
└── README.md                 # 本文档
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── CreatureEntity
            └── MonsterEntity
                ├── SpiderEntity (蜘蛛)
                │   └── CaveSpiderEntity (洞穴蜘蛛)
                ├── EndermiteEntity (末影螨)
                └── SilverfishEntity (蠹虫)
```

## 实体详解

### 蜘蛛 (SpiderEntity)

蜘蛛是一种可以攀爬墙壁的节肢类怪物，只在黑暗中攻击玩家。

**特性**：
- 可攀爬垂直墙壁
- 只在光照等级 < 7 时攻击
- 白天中立，夜间敌对
- 中毒效果免疫（节肢类特性）
- 不在阳光下燃烧

**AI 目标 (MC 1.16.5 已实现)**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳 |
| 3 | LeapAtTargetGoal | 跳向目标（力度0.4F） |
| 4 | SpiderAttackGoal | 近战攻击（带光照条件检测） |
| 5 | WaterAvoidingRandomWalkingGoal | 避水随机行走（速度0.8D） |
| 6 | LookAtGoal | 看向玩家（8格） |
| 6 | LookRandomlyGoal | 随机看向 |

**目标选择 (MC 1.16.5 已实现)**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal | 被攻击后反击 |
| 2 | SpiderTargetGoal\<Player\> | 攻击玩家（黑暗条件） |
| 3 | SpiderTargetGoal\<IronGolem\> | 攻击铁傀儡（黑暗条件，待启用） |

**属性值**：
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 16.0 |
| MOVEMENT_SPEED | 0.3 |
| ATTACK_DAMAGE | 2.0 |

**光照检测**：
```cpp
bool shouldAttack(LivingEntity* target) const {
    if (m_world != nullptr) {
        u8 lightLevel = m_world->getLightSubtracted(pos, 0);
        if (lightLevel < 7) {
            return MonsterEntity::shouldAttack(target);
        }
        return false;  // 明亮处不攻击
    }
    return MonsterEntity::shouldAttack(target);
}
```

### 洞穴蜘蛛 (CaveSpiderEntity)

洞穴蜘蛛是蜘蛛的变种，体型更小，攻击会造成中毒效果。

**与蜘蛛的区别**：
- 更小的碰撞箱（0.7 × 0.5 vs 1.4 × 0.9）
- 攻击造成中毒效果：
  - 简单难度：无中毒
  - 普通难度：7秒中毒I
  - 困难难度：15秒中毒I
- 只在废弃矿井生成
- 继承蜘蛛的光照敏感攻击特性

**中毒攻击实现**：
```cpp
bool CaveSpiderEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法执行基础攻击
    if (!SpiderEntity::attackEntityAsMob(target)) {
        return false;
    }
    
    // 根据难度应用中毒效果
    Difficulty difficulty = m_world->difficulty();
    i32 poisonDuration = 0;
    if (difficulty == Difficulty::Normal) {
        poisonDuration = 7; // 7秒
    } else if (difficulty == Difficulty::Hard) {
        poisonDuration = 15; // 15秒
    }
    
    if (poisonDuration > 0) {
        target.addEffect(EffectInstance(EffectType::Poison, poisonDuration * 20, 0));
    }
    return true;
}
```

### 末影螨 (EndermiteEntity)

末影螨是一种小型敌对生物，有概率在末影人瞬移时生成。

**消失逻辑** (MC 1.16.5):
- 非持久化末影螨在 **2400 ticks（2分钟）** 后自动消失
- 使用 `MobEntity::isNoDespawnRequired()` 检查持久化状态
- 命名牌命名的末影螨不会消失

```cpp
void EndermiteEntity::tick() {
    m_prevRenderYawOffset = m_renderYawOffset;
    m_renderYawOffset = yaw();
    
    MonsterEntity::tick();
    
    // 消失逻辑（MC 1.16.5 livingTick）
    if (!isNoDespawnRequired()) {
        m_lifetime++;
        if (m_lifetime >= DESPAWN_TIME) {  // 2400 ticks
            remove();
        }
    }
}
```

**AI 目标**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳 |
| 2 | MeleeAttackGoal(1.0, false) | 近战攻击玩家 |
| 3 | WaterAvoidingRandomWalkingGoal(1.0) | 避水随机行走 |
| 7 | LookAtGoal(8.0f) | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal(false) | 受伤反击 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

**属性值**：
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 8.0 |
| MOVEMENT_SPEED | 0.25 |
| ATTACK_DAMAGE | 2.0 |
| EXPERIENCE_VALUE | 3 |

**生成规则**：
- 末影人瞬移时有 5% 概率生成
- 自然生成时检查 5 格内是否有玩家（只在无玩家时生成）

### 蠹虫 (SilverfishEntity)

蠹虫是一种小型敌对生物，生成于要塞的怪物蛋方块中，受伤时会召唤更多蠹虫。

**特殊行为**：
- 受伤时有概率召唤周围虫蚀方块中的蠹虫
- 可以躲入石头方块（怪物蛋）
- 受到攻击时呼唤同伴

**AI 目标**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | SwimGoal | 游泳 |
| 4 | MeleeAttackGoal(1.0, false) | 近战攻击玩家 |
| 5 | WaterAvoidingRandomWalkingGoal(1.0) | 避水随机行走 |
| 7 | LookAtGoal(8.0f) | 看向玩家 |
| 8 | LookRandomlyGoal | 随机看向 |

**目标选择器**：
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | HurtByTargetGoal(true) | 受伤反击并呼唤同伴 |
| 2 | NearestAttackableTargetGoal<Player> | 攻击最近玩家 |

**属性值**：
| 属性 | 值 |
|------|-----|
| MAX_HEALTH | 8.0 |
| MOVEMENT_SPEED | 0.25 |
| ATTACK_DAMAGE | 1.0 |
| EXPERIENCE_VALUE | 5 |

**预留功能**：
- `m_summonCooldown`: 召唤同伴倒计时
- `notifySummonCooldown()`: 受伤时触发召唤冷却（20 ticks）
- TODO: SilverfishHideInStoneGoal（藏入石头）
- TODO: SilverfishSummonOthersGoal（召唤同伴）

## 共同特性

### 节肢类特性
所有节肢类怪物共享以下特性：
- 受到节肢杀手附魔额外伤害
- 受到节肢杀手攻击时获得缓慢效果
- 生物类型：`CreatureAttribute::Arthropod`

### 阳光燃烧
- 蜘蛛和洞穴蜘蛛在阳光下燃烧
- 末影螨和蠹虫**不**在阳光下燃烧（`setBurnsInDaylight(false)`）

## 文件说明

| 文件 | 职责 |
|------|------|
| SpiderEntity.hpp/cpp | 蜘蛛实体定义、光照检测AI、攀爬逻辑 |
| CaveSpiderEntity.hpp/cpp | 洞穴蜘蛛实体定义、中毒攻击 |
| EndermiteEntity.hpp/cpp | 末影螨和蠹虫实体定义、消失逻辑、AI目标注册 |

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.SpiderEntity`
- MC 1.16.5 `net.minecraft.entity.monster.CaveSpiderEntity`
- MC 1.16.5 `net.minecraft.entity.monster.EndermiteEntity`
- MC 1.16.5 `net.minecraft.entity.monster.SilverfishEntity`
