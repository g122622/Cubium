# 傀儡实体模块

本目录包含傀儡实体的实现，包括铁傀儡和雪傀儡。

## 目录结构

```
golem/
├── GolemEntity.hpp       # 傀儡基类头文件
├── GolemEntity.cpp       # 傀儡基类实现
├── SnowGolemEntity.hpp   # 雪傀儡实体头文件
├── SnowGolemEntity.cpp   # 雪傀儡实体实现
├── IronGolemEntity.hpp   # 铁傀儡实体头文件
├── IronGolemEntity.cpp   # 铁傀儡实体实现
└── README.md             # 本文件
```

## 文件介绍

### GolemEntity.hpp / GolemEntity.cpp

傀儡实体的抽象基类，继承自 `CreatureEntity` 并实现 `IAngerable` 接口。

**职责：**
- 定义傀儡的共同特性（保护、中立、强壮）
- 实现愤怒系统（`IAngerable` 接口）
- 管理攻击目标和愤怒时间

**继承链：**
```
Entity -> LivingEntity -> MobEntity -> CreatureEntity -> GolemEntity
```

**关键接口：**
- `setRevengeTarget()` - 设置复仇目标
- `isAngry()` - 是否处于愤怒状态
- `setAngry()` - 设置愤怒状态
- `updateAnger()` - 更新愤怒计时器

### SnowGolemEntity.hpp / SnowGolemEntity.cpp

雪傀儡实体，完整的 MC 1.16.5 实现。

**特性：**
- **投掷雪球**：向敌对生物投掷雪球攻击（`IRangedAttackMob`）
- **留下雪迹**：在寒冷生物群系（温度 < 0.8）行走时放置雪层
- **融化**：在高温生物群系（温度 > 1.0）或水中会融化（受到火焰伤害）
- **南瓜头**：可以用剪刀取下南瓜（`IShearable`）

**实现细节：**

| 功能 | 方法 | MC 1.16.5 参考 |
|------|------|----------------|
| 融化检查 | `willMelt()` | 温度 > 1.0 或在水中 |
| 融化伤害 | `tick()` | 每秒 1 点火焰伤害 |
| 雪层放置 | `placeSnowLayer()` | 温度 < 0.8，检查 mobGriefing |
| 远程攻击 | `attackEntityWithRangedAttack()` | 投掷雪球 |
| 剪切 | `shear()` | 掉落 CARVED_PUMPKIN |

**AI 目标：**

| 优先级 | 目标 | 说明 |
|--------|------|------|
| 1 | `RangedAttackGoal` | 投掷雪球攻击敌对生物 |
| 2 | `WaterAvoidingRandomWalkingGoal` | 避水随机行走 |
| 3 | `LookAtGoal` | 看向玩家 |
| 4 | `LookRandomlyGoal` | 随机看向 |
| 目标选择器 1 | `NearestAttackableTargetGoal<MobEntity>` | 自动攻击敌对生物 |

**属性值（MC 1.16.5）：**
- 生命值：4.0
- 移动速度：0.2
- 眼睛高度：1.7
- 宽度：0.7
- 高度：1.9

**常量：**
- `ATTACK_COOLDOWN = 20` ticks（1秒）
- `SNOWBALL_VELOCITY = 1.6f`
- `SNOWBALL_INACCURACY = 12.0f`
- `MELT_TEMPERATURE = 1.0f`
- `SNOW_TEMPERATURE = 0.8f`
- `MELT_DAMAGE_INTERVAL = 20` ticks
- `MELT_DAMAGE = 1.0f`

### IronGolemEntity.hpp / IronGolemEntity.cpp

铁傀儡实体，完整的 MC 1.16.5 实现。

**特性：**
- **保护村民**：攻击威胁村民的敌对生物
- **中立性**：对玩家中立（玩家创建的铁傀儡不攻击玩家）
- **近战攻击**：强力近战攻击并击飞敌人
- **举起手臂**：攻击时举起手臂动画
- **展示花朵**：偶尔向村民展示罂粟花
- **玩家创建**：可由玩家用方块建造生成

**实现细节：**

| 功能 | 方法 | MC 1.16.5 参考 |
|------|------|----------------|
| 近战攻击 | `attackEntityAsMob()` | 造成伤害并击飞 |
| 持花状态 | `setHoldingRose()` | 持花 400 ticks |
| 攻击检查 | `canAttackEntity()` | 不攻击苦力怕和玩家(玩家创建时) |
| 手臂状态 | `isArmsRaised()` | 攻击动画 |

**AI 目标：**

| 优先级 | 目标 | 说明 |
|--------|------|------|
| 0 | `SwimGoal` | 游泳 |
| 1 | `MeleeAttackGoal` | 近战攻击 (速度 1.0) |
| 2 | `MoveTowardsTargetGoal` | 向目标移动 (速度 0.9, 距离 32) |
| 5 | `ShowVillagerFlowerGoal` | 给村民展示花朵 |
| 7 | `LookAtGoal` | 看向玩家 (距离 6) |
| 8 | `LookRandomlyGoal` | 随机看向 |
| 目标选择器 2 | `HurtByTargetGoal` | 被攻击后反击 |
| 目标选择器 3 | `NearestAttackableTargetGoal<LivingEntity>` | 攻击敌对生物(排除苦力怕) |

**属性值（MC 1.16.5）：**
- 生命值：100.0
- 移动速度：0.25
- 击退抗性：1.0
- 攻击伤害：7.0
- 眼睛高度：2.1
- 宽度：1.4
- 高度：2.7

**常量：**
- `ATTACK_DURATION = 10` ticks（攻击动画持续时间）
- `ATTACK_COOLDOWN = 20` ticks（攻击冷却）
- `ATTACK_DAMAGE = 7.0f`（基础攻击伤害）
- `KNOCKBACK = 1.5f`（击退力度）

**IAngerable 接口实现：**
- `getRevengeTarget()` / `setRevengeTarget()` - 复仇目标追踪
- `getRevengeTimer()` - 复仇计时器
- `isAngry()` / `setAngry()` - 愤怒状态
- `getAngerTime()` / `setAngerTime()` - 愤怒时间
- `MAX_ANGER_TIME = 600` ticks (30秒)

## 模块关系

```
                    ┌─────────────────┐
                    │   CreatureEntity │
                    └────────┬────────┘
                             │
                    ┌────────▼────────┐
                    │   GolemEntity    │
                    │  (IAngerable)    │
                    └────────┬────────┘
                             │
           ┌─────────────────┼─────────────────┐
           │                 │                 │
    ┌──────▼──────┐   ┌──────▼──────┐   ┌──────▼──────┐
    │SnowGolemEntity│   │IronGolemEntity│   │ (其他傀儡) │
    │(IShearable)  │   │              │   │             │
    │(IRangedAttack)│  │              │   │             │
    └──────────────┘   └──────────────┘   └─────────────┘
```

## 依赖项

### 外部依赖
- `IRangedAttackMob` - 远程攻击接口
- `IShearable` - 可剪切接口
- `IAngerable` - 愤怒接口
- `BiomeRegistry` - 生物群系注册表
- `VanillaBlocks` - 原版方块定义
- `BlockItemRegistry` - 方块物品注册表
- `SoundEvents` - 声音事件定义
- `GameRules` - 游戏规则系统

### AI 目标依赖
- `RangedAttackGoal` - 远程攻击目标
- `WaterAvoidingRandomWalkingGoal` - 避水随机行走
- `LookAtGoal` - 看向目标
- `LookRandomlyGoal` - 随机看向
- `NearestAttackableTargetGoal` - 最近攻击目标

## 使用方法

### 创建雪傀儡

```cpp
#include "entity/entities/passive/golem/SnowGolemEntity.hpp"

// 工厂方法创建
auto entity = SnowGolemEntity::create(world);
world->spawnEntity(std::move(entity));
```

### 检查南瓜状态

```cpp
SnowGolemEntity* golem = /* ... */;
if (golem->isShearable()) {
    // 可以剪下南瓜
    auto drops = golem->shear(player);
    // drops 包含 CARVED_PUMPKIN 物品
}
```

### 检查融化条件

```cpp
if (golem->willMelt()) {
    // 雪傀儡将在当前环境中融化
}
```

## 容易踩的坑

1. **继承链顺序**：`GolemEntity` 继承自 `CreatureEntity` 而非 `MobEntity`，与 MC 1.16.5 保持一致。

2. **温度检查**：`Biome::getTemperature(y)` 会考虑高度因素，不是简单的生物群系基础温度。

3. **雪层放置**：需要同时满足以下条件：
   - `mobGriefing` 游戏规则为 true
   - 实体存活
   - 不在客户端
   - 生物群系温度 < 0.8

4. **剪切物品**：通过 `BlockItemRegistry` 获取 CARVED_PUMPKIN 对应的物品，确保物品系统已初始化。

5. **远程攻击**：雪球使用 `entity::SnowballEntity`（在 `mc::entity` 命名空间），需要正确使用命名空间。

## 测试用例

相关测试位于：
- `tests/common/entity/entities/passive/golem/SnowGolemEntityTest.cpp`
- `tests/common/entity/IronGolemGoalsTest.cpp`

### 雪傀儡测试
- 南瓜头状态测试
- 剪切功能测试
- 融化条件测试
- 属性值测试
- 尺寸测试
- 攻击间隔测试
- 水敏感性测试
- 继承关系测试
- 声音事件测试

### 铁傀儡测试
- ShowVillagerFlowerGoal 构造和类型名测试
- MoveTowardsTargetGoal 构造和互斥标志测试
- ResetAngerGoal 愤怒状态测试
- IronGolemEntity 状态测试（手臂举起、持花、玩家创建标记）
- IronGolemEntity 攻击检查测试（苦力怕排除）
- IAngerable 接口测试（愤怒状态、复仇目标）

## 参考文档

- MC 1.16.5 `net.minecraft.entity.passive.SnowGolemEntity`
- MC 1.16.5 `net.minecraft.entity.passive.GolemEntity`
- MC Wiki: Snow Golem
