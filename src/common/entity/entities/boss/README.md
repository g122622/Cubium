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
    virtual std::string getBossName() const = 0;

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
- **免疫**: 火焰、溺水、凋零、其他凋灵伤害

```cpp
class WitherEntity : public MobEntity, public IRangedAttackMob {
public:
    // 阶段判断
    bool isInvulnerablePhase() const;  // 是否处于无敌阶段
    bool isCharged() const;            // 是否充能（生命值低于一半）

    // 无敌时间管理
    i32 getInvulTime() const;
    void setInvulTime(i32 time);
    void ignite();  // 开始生成序列（设置220 tick无敌时间）

    // 三头目标追踪（MC 1.16.5 数据参数同步）
    i32 getWatchedTargetId(i32 head) const;      // 获取头部目标实体ID（0=主头, 1=左头, 2=右头）
    void updateWatchedTargetId(i32 head, i32 targetId);  // 更新头部目标

    // 远程攻击
    void launchWitherSkullToEntity(i32 head, LivingEntity* target);
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    bool canRangedAttack() const override;  // 无敌阶段不能攻击

    // 头部位置计算
    f32 getHeadX(i32 head) const;
    f32 getHeadY(i32 head) const;
    f32 getHeadZ(i32 head) const;

    // 方块破坏
    void breakNearbyBlocks();  // 破坏凋灵周围 3x4x3 范围内的方块

    // 受伤处理
    bool hurt(DamageSource& source, f32 amount) override;  // 受伤后触发方块破坏

    // 生物属性
    CreatureAttribute getCreatureAttribute() const override;  // Undead
    bool isNonBoss() const override;  // false
};
```

#### 凋灵免疫伤害

凋灵对以下伤害类型免疫：

| 伤害类型 | 说明 |
|----------|------|
| 溺水 | `DamageType::Drown` |
| 凋零 | `DamageType::Wither` |
| 其他凋灵 | 来自其他凋灵实体的伤害 |
| 箭矢（充能时） | 充能状态（生命值≤50%）免疫箭矢 |

#### 方块破坏机制

凋灵的方块破坏有以下特点：

1. **触发条件**：受伤后触发，有 20 tick 冷却
2. **破坏范围**：凋灵周围 3x4x3 方块（x: -1~1, y: 0~3, z: -1~1）
3. **不可破坏方块**：使用 `BlockTags::WITHER_IMMUNE` 标签
4. **游戏规则**：受 `mobGriefing` 游戏规则控制
5. **音效反馈**：破坏方块后播放 `ENTITY_WITHER_BREAK_BLOCK`

#### WITHER_IMMUNE 方块标签

凋灵无法破坏以下方块：

| 方块 | 说明 |
|------|------|
| barrier | 屏障 |
| bedrock | 基岩 |
| end_portal | 末地传送门 |
| end_portal_frame | 末地传送门框架 |
| end_gateway | 末地折跃门 |
| command_block | 命令方块 |
| repeating_command_block | 循环命令方块 |
| chain_command_block | 连锁命令方块 |
| structure_block | 结构方块 |
| jigsaw | 拼图方块 |
| moving_piston | 移动中的活塞 |
| light | 光源方块 |

#### 三头目标追踪系统

凋灵的三个头独立追踪目标，通过 `EntityDataManager` 实现客户端-服务端数据同步：

| 头部 | 索引 | 行为 |
|------|------|------|
| 主头 | 0 | 追踪 `attackTarget`（AI 选择的主目标） |
| 左头 | 1 | 每 10-20 tick 搜索最近的非亡灵生物 |
| 右头 | 2 | 每 10-20 tick 搜索最近的非亡灵生物 |

**数据参数**:
- `HEAD_TARGET_1`: 主头目标实体ID
- `HEAD_TARGET_2`: 左头目标实体ID
- `HEAD_TARGET_3`: 右头目标实体ID

**追踪逻辑** (`updateHeadTargets()`):
1. 主头直接追踪 `attackTarget`
2. 侧头周期性搜索范围内非亡灵生物
3. **创造/旁观模式玩家不会被作为目标** (MC 1.16.5)
4. 找到目标后发射凋灵之首
5. 目标无效时清除追踪

**目标选择排除条件**:
- 亡灵生物（`CreatureAttribute::Undead`）
- 创造模式玩家（`Player::isCreative()`）
- 旁观者模式玩家（`Player::isSpectator()`）

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
    std::string name = boss->getBossName();
    // 渲染Boss生命条
}
```

## 参考

- MC 1.16.5 EnderDragonEntity
- MC 1.16.5 EnderDragonPartEntity
- MC 1.16.5 WitherEntity
- MC 1.16.5 DragonPhaseManager
