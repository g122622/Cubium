# 海洋怪物模块 (Ocean Monsters)

包含海洋生物群系中的敌对生物实现。

## 目录结构

```
ocean/
├── GuardianEntity.hpp/cpp       # 守卫者实体
└── ElderGuardianEntity.hpp/cpp  # 远古守卫者实体
```

## 守卫者 (GuardianEntity)

守卫者是海底神殿的守卫者，使用激光攻击玩家和鱿鱼。

### 实现状态
| 功能 | 状态 | 说明 |
|------|------|------|
| 基础属性 | ✅ | MAX_HEALTH=30, MOVEMENT_SPEED=0.3, ATTACK_DAMAGE=4.0, FOLLOW_RANGE=16.0 |
| 激光攻击 | ✅ | GuardianAttackGoal 已实现完整攻击流程 |
| 目标选择 | ✅ | 使用 NearestAttackableTargetGoal 攻击玩家和鱿鱼 |
| 尖刺动画 | ✅ | 40 tick 周期切换尖刺状态 |
| AI 目标 | ✅ | 完整实现 registerGoals() |

### AI 目标配置

**行为目标选择器 (goalSelector)**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 4 | GuardianAttackGoal | 激光攻击目标 |
| 7 | RandomWalkingGoal | 随机漫步 (间隔 80 tick) |
| 8 | LookAtGoal(Player, 8格) | 看向玩家 |
| 8 | LookAtGoal(Guardian/ElderGuardian, 12格, 1%) | 看向同类守卫者 |
| 9 | LookRandomlyGoal | 随机看向 |

**目标选择器 (targetSelector)**:
| 优先级 | Goal | 说明 |
|--------|------|------|
| 1 | NearestAttackableTargetGoal<LivingEntity> | 搜索最近的可攻击目标 |

### 目标选择谓词

守卫者使用自定义谓词筛选攻击目标，参考 MC 1.16.5 `GuardianEntity.TargetPredicate`:

```cpp
m_targetSelector.addGoal(1, std::make_unique<NearestAttackableTargetGoal<LivingEntity>>(
    this,
    true,    // checkSight - 需要视线检查
    10,      // chance - 每10tick检查一次
    [this](const LivingEntity* candidate) -> bool {
        // 1. 类型筛选: 只攻击玩家或鱿鱼
        auto typeId = candidate->typeId();
        bool isPlayer = (typeId == entity::EntityTypeIdNumber::PLAYER);
        bool isSquid = (typeId == entity::EntityTypeIdNumber::SQUID);
        if (!isPlayer && !isSquid) return false;

        // 2. 玩家模式检查: 创造模式和观察者模式不能被攻击
        if (isPlayer) {
            const Player* player = dynamic_cast<const Player*>(candidate);
            if (player && (player->isCreative() || player->isSpectator())) {
                return false;
            }
        }

        // 3. 距离筛选: 必须距离 > 3 格
        f64 distSq = this->distanceSqTo(*candidate);
        if (distSq <= 9.0) return false;  // 3.0 * 3.0 = 9.0

        return true;
    }
));
```

### GuardianAttackGoal 激光攻击

**常量**:
| 常量 | 值 | 说明 |
|------|-----|------|
| CHARGE_DURATION | 60 tick | 充能时间 (3秒) |
| COOLDOWN_DURATION | 20 tick | 冷却时间 (1秒) |
| ATTACK_RANGE | 15.0f | 激光攻击范围 |
| LASER_DAMAGE | 4.0f | 激光伤害 |

**攻击流程**:
1. `shouldExecute()` - 检查 `attackTarget()` 或调用 `selectTarget()` 搜索目标
2. `startExecuting()` - 初始化充能状态，设置目标实体ID
3. `tick()` - 更新充能时间，看向目标
4. `performLaserAttack()` - 充能完成后造成魔法伤害

**目标搜索逻辑** (`selectTarget()`):
- 使用 `EntityUtils::findClosestEntity<LivingEntity>()` 搜索最近目标
- 目标类型筛选: Player 或 Squid
- 距离筛选: 距离平方 > 9.0 (> 3格)
- 视线检查: `MobEntity::canSee()`

### 继承关系

```
Entity
└── LivingEntity
    └── MobEntity
        └── MonsterEntity
            └── GuardianEntity
                └── ElderGuardianEntity (远古守卫者)
```

## 远古守卫者 (ElderGuardianEntity)

远古守卫者是守卫者的变种，拥有更高的生命值和特殊能力。

### 属性差异
| 属性 | 守卫者 | 远古守卫者 |
|------|--------|------------|
| MAX_HEALTH | 30.0 | 80.0 |
| ATTACK_DAMAGE | 4.0 | 更高 |

### 特殊能力
- 挖掘疲劳效果 (50格范围)
- Boss 战机制

## 测试文件

| 测试文件 | 说明 |
|----------|------|
| tests/entity/GuardianAttackGoalTest.cpp | 14 个测试用例验证目标选择常量和配置 |

## 参考

- MC 1.16.5 `net.minecraft.entity.monster.GuardianEntity`
- MC 1.16.5 `net.minecraft.entity.ai.goal.Goal`
