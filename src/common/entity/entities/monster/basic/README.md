# 敌对生物基类

MonsterEntity 是所有敌对生物（怪物）的基类。

## 继承

```
MobEntity
└── CreatureEntity
    └── MonsterEntity
        ├── AbstractSkeletonEntity
        ├── ZombieEntity
        ├── CreeperEntity
        ├── SpiderEntity
        ├── EndermanEntity
        ├── SlimeEntity
        └── AbstractIllagerEntity
```

## 特性

### 光照敏感
- `shouldBurnInDaylight()` - 是否在阳光下燃烧
- `isInDaylight()` - 检查是否暴露在阳光下
- 亡灵类怪物默认在阳光下燃烧

### 敌对行为
- `shouldAttack(LivingEntity*)` - 检查是否应该攻击目标
- 自动攻击玩家
- 被攻击后反击

### 属性
- FOLLOW_RANGE（跟随范围）
- ATTACK_DAMAGE（攻击伤害）
- ATTACK_SPEED（攻击速度）

## AI目标

| 优先级 | Goal | 说明 |
|--------|------|------|
| 0 | SwimGoal | 在水中游泳 |
| 1 | HurtByTargetGoal | 被攻击后反击 |
| 2 | NearestAttackableTargetGoal | 攻击最近目标 |
| 3 | MeleeAttackGoal | 近战攻击 |
| 5 | WaterAvoidingRandomWalkingGoal | 随机行走 |
| 6 | LookAtGoal | 看向玩家 |
| 7 | LookRandomlyGoal | 随机看向 |

## 子类需要实现

```cpp
class ZombieEntity : public MonsterEntity {
public:
    void registerGoals() override {
        MonsterEntity::registerGoals();
        m_goalSelector.addGoal(2, std::make_unique<MeleeAttackGoal>(this, 1.0, false));
        m_targetSelector.addGoal(1, std::make_unique<HurtByTargetGoal>(this));
        m_targetSelector.addGoal(2, std::make_unique<NearestAttackableTargetGoal<Player>>(this));
    }
};
```
