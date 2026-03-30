# Boss实体 (Boss Entities)

本目录包含Boss级怪物的实现。

## 目录结构

```
boss/
├── EnderDragonEntity.hpp/cpp  # 末影龙 + EnderDragonPartEntity
├── WitherEntity.hpp/cpp       # 凋灵
└── README.md                  # 本文档
```

## 继承层次

```
Entity
└── LivingEntity
    └── MobEntity
        └── BossEntity           # Boss基类
            ├── EnderDragonEntity # 末影龙
            │   └── EnderDragonPartEntity  # 龙部件（碰撞体）
            └── WitherEntity      # 凋灵
```

## 实体列表

| 实体 | 生命值 | 特性 |
|------|--------|------|
| EnderDragonEntity | 200 | 多阶段AI，飞行，龙息，末地Boss |
| WitherEntity | 300 | 三头Boss，凋灵之首，地狱Boss |

## 核心类设计

### BossEntity

Boss实体基类，提供：
- Boss名称显示
- 生命条渲染
- Boss战状态管理

```cpp
class BossEntity : public MobEntity {
public:
    // Boss名称（显示在生命条上）
    virtual String getBossName() const = 0;

    // 生命条显示范围
    virtual f32 getHealthBarRange() const { return 100.0f; }

    // 是否显示生命条
    bool shouldDisplayHealthBar() const;

    // 生命条颜色
    virtual u32 getHealthBarColor() const { return 0xFF0000; }

    // Boss战状态
    bool inBossFight() const;
    void setBossFight(bool fighting);
};
```

### EnderDragonEntity

末影龙，末地Boss：

- **生命值**: 200
- **尺寸**: 16×8 方块
- **阶段AI**: 盘旋、突袭、降落、栖息、死亡
- **攻击**: 龙息、冲撞、龙火球
- **部件**: 6个碰撞部件（头、颈、身、尾、左翼、右翼）

```cpp
class EnderDragonEntity : public BossEntity {
public:
    enum class Phase : u8 {
        HoldingPattern,   // 盘旋
        StrafePlayer,     // 突袭玩家
        LandingApproach,  // 准备降落
        Landing,          // 降落
        Takeoff,          // 起飞
        Sitting,          // 栖息（在传送门上）
        ChargingPlayer,   // 冲撞
        Dying,            // 死亡
        Hover            // 悬停
    };

    // 阶段
    Phase phase() const;
    void setPhase(Phase phase);

    // 攻击
    void breathAttack();        // 龙息
    void chargeAttack();        // 冲撞
    void dragonFireballAttack(); // 龙火球

    // 重生
    static void respawnDragon(IWorld* world, BlockCoord portalPos);
};
```

#### 末影龙阶段

| 阶段 | 描述 |
|------|------|
| HoldingPattern | 围绕末地盘旋 |
| StrafePlayer | 飞向玩家发射火球 |
| LandingApproach | 准备降落到传送门 |
| Landing | 降落到传送门 |
| Takeoff | 从传送门起飞 |
| Sitting | 在传送门上休息，发射龙息 |
| ChargingPlayer | 冲向玩家 |
| Dying | 死亡动画 |
| Hover | 悬停 |

### EnderDragonPartEntity

末影龙的碰撞部件：

```cpp
class EnderDragonPartEntity : public Entity {
public:
    enum class Part : u8 {
        Head,       // 头部
        Neck,       // 颈部
        Body,       // 身体
        Tail,       // 尾部
        WingLeft,   // 左翼
        WingRight   // 右翼
    };

    void updatePosition(f32 offsetX, f32 offsetY, f32 offsetZ, f32 size);
};
```

### WitherEntity

凋灵，地狱Boss：

- **生命值**: 300
- **尺寸**: 0.9×3.5 方块
- **阶段**: 无敌生成、充能、攻击
- **攻击**: 凋灵之首（普通/蓝色）
- **免疫**: 火焰、溺水、凋零

```cpp
class WitherEntity : public BossEntity, public IRangedAttackMob {
public:
    enum class Phase : u8 {
        Invulnerable,  // 无敌阶段（生成中）
        Charging,      // 充能阶段
        Attacking      // 攻击阶段
    };

    // 阶段
    Phase phase() const;
    bool isInvulnerablePhase() const;
    bool isCharging() const;

    // 攻击
    void shootWitherSkull(LivingEntity* target, bool isBlue = false);
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // 三个头的独立目标
    LivingEntity* getHeadTarget() const;
    LivingEntity* getLeftHeadTarget() const;
    LivingEntity* getRightHeadTarget() const;
};
```

#### 凋灵阶段

| 阶段 | 描述 | 持续时间 |
|------|------|----------|
| Invulnerable | 生成动画，恢复生命 | 220 ticks (11秒) |
| Charging | 充能，发射蓝色凋灵之首 | 20 ticks (1秒) |
| Attacking | 正常攻击 | 持续直到生命值低于50% |

#### 凋灵之首

| 类型 | 效果 |
|------|------|
| 普通凋灵之首 | 爆炸威力1，凋零效果 |
| 蓝色凋灵之首 | 爆炸威力1，破坏方块，凋零效果 |

## 使用示例

### 生成末影龙

```cpp
auto dragon = std::make_unique<EnderDragonEntity>(LegacyEntityType::Unknown, id);
dragon->setPosition(0, 128, 0);  // 末地中心上方
dragon->setPhase(EnderDragonEntity::Phase::HoldingPattern);
world->spawnEntity(std::move(dragon));
```

### 生成凋灵

```cpp
auto wither = std::make_unique<WitherEntity>(LegacyEntityType::Unknown, id);
wither->setPosition(x, y + 3, z);
wither->setHealth(1);  // 从1点生命开始
wither->setPhase(WitherEntity::Phase::Invulnerable);
world->spawnEntity(std::move(wither));

// 220 ticks 后凋灵会自动爆炸并切换到攻击阶段
```

### Boss生命条渲染

```cpp
// 在渲染循环中
if (boss->shouldDisplayHealthBar()) {
    f32 healthPercent = boss->health() / boss->maxHealth();
    u32 color = boss->getHealthBarColor();
    String name = boss->getBossName();
    // 渲染Boss生命条
}
```

## 参考

- MC 1.16.5 EnderDragonEntity
- MC 1.16.5 EnderDragonPartEntity
- MC 1.16.5 WitherEntity
- MC 1.16.5 DragonPhaseManager
