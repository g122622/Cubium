# 食物/饥饿系统

本目录实现了玩家的饥饿和饱食度系统。

## 文件说明

| 文件 | 职责 |
|------|------|
| `FoodStats.hpp/cpp` | 饥饿系统核心类，管理饥饿值、饱和度、消耗值和生命恢复 |
| `README.md` | 本文件 |

## 饥饿系统机制

### 核心属性

| 属性 | 范围 | 说明 |
|------|------|------|
| `foodLevel` | 0-20 | 饥饿值，显示为 10 个鸡腿 |
| `saturationLevel` | 0-foodLevel | 饱和度，先于饥饿值消耗 |
| `exhaustionLevel` | 0-40 | 累积消耗值，每 4.0 消耗 1 饱和度或饥饿值 |
| `foodTimer` | - | 计时器，用于生命恢复和饥饿伤害 |

### 进食机制

进食时饥饿值和饱和度的计算公式：
```
foodLevel = min(foodLevel + food, 20)
saturationLevel = min(saturationLevel + food * saturationModifier * 2.0, foodLevel)
```

**注意**：`saturationModifier` 不是直接的饱和度值，而是修正系数。
例如苹果（food=4, modifier=0.3）提供 saturation = 4 * 0.3 * 2 = 2.4 饱和度。

### 消耗机制

活动消耗累积到 `exhaustionLevel`：
- 每累积 4.0 消耗值，消耗 1 饱和度或饥饿值
- 优先消耗饱和度
- 和平模式下不消耗饥饿值

### 生命恢复

#### 快速恢复（饱和度恢复）
- 条件：`foodLevel >= 20` 且 `saturation > 0` 且无饥饿效果
- 速度：每 10 ticks（0.5秒）
- 恢复量：`saturation / 6` 点生命
- 消耗：等量饱和度

#### 慢速恢复（饥饿值恢复）
- 条件：`foodLevel >= 18` 且无饥饿效果
- 速度：每 80 ticks（4秒）
- 恢复量：1.0 点生命
- 消耗：6.0 消耗值

### 饥饿伤害

- 条件：`foodLevel <= 0`
- 速度：每 80 ticks（4秒）
- 伤害量：1.0 点
- 难度限制：
  - 和平：不造成伤害
  - 简单：生命值最低 10
  - 普通：生命值最低 1
  - 困难：可以饿死

### 和平模式特殊处理

- 每 20 ticks（1秒）恢复 1 点生命
- 每 10 ticks（0.5秒）恢复 1 点饥饿值

## 消耗值来源

| 活动 | 消耗值 |
|------|--------|
| 普通跳跃 | 0.05 |
| 疾跑跳跃 | 0.2 |
| 疾跑（每米） | 0.1 |
| 游泳（每米） | 0.01 |
| 水下行走（每米） | 0.01 |
| 水面行走（每米） | 0.01 |
| 攻击实体 | 0.1 |
| 受到伤害 | 根据伤害源 |
| 快速恢复 | 使用的饱和度 |
| 慢速恢复 | 6.0 |

## 与 MC 1.16.5 的对齐

本实现严格遵循 MC 1.16.5 的 FoodStats 逻辑：
- 参考：`net.minecraft.util.FoodStats`
- 参考：`net.minecraft.entity.player.PlayerEntity` (tick 方法中的饥饿处理)

## 使用方法

```cpp
// 进食
player.foodStats().addStats(4, 0.3f);  // 苹果

// 增加消耗值
player.foodStats().addExhaustion(0.1f);  // 疾跑 1 米

// 每刻更新（在 Player::tick 中调用）
player.foodStats().tick(player, difficulty, naturalRegeneration);
```

## 依赖关系

```
FoodStats
  ├── Player (前向声明)
  └── Difficulty (Types.hpp)
```

## 测试用例

- `tests/common/test_entity.cpp` - 包含 FoodStats 完整测试套件：
  - `TEST(Player, Food)` - 基础饥饿值/饱和度测试
  - `TEST(FoodStats, ExhaustionConsumption)` - 消耗值累积触发饱和度/饥饿值消耗
  - `TEST(FoodStats, SaturationCalculation)` - 饱和度计算公式验证
  - `TEST(FoodStats, NeedsFood)` - needsFood() 条件测试
  - `TEST(FoodStats, FoodTimer)` - 食物计时器测试
  - `TEST(FoodStats, PrevFoodLevel)` - UI同步测试
  - `TEST(FoodStats, ExhaustionCap)` - 消耗值上限测试
  - `TEST(FoodStats, Serialization)` - 序列化/反序列化测试
  - `TEST(FoodStats, FastRegeneration)` - 快速生命恢复测试（foodLevel=20, saturation>0）
  - `TEST(FoodStats, SlowRegeneration)` - 慢速生命恢复测试（foodLevel>=18）
  - `TEST(FoodStats, StarvationDamage)` - 饥饿伤害测试（foodLevel=0）
  - `TEST(FoodStats, StarvationDamageEasyMode)` - 简单模式饥饿伤害（最低10点生命）
  - `TEST(FoodStats, StarvationDamageNormalMode)` - 普通模式饥饿伤害（最低1点生命）
  - `TEST(FoodStats, PeacefulMode)` - 和平模式自动恢复测试
  - `TEST(FoodStats, PeacefulModeNoStarvation)` - 和平模式无饥饿伤害测试
  - `TEST(FoodStats, NoRegenerationWithHungerEffect)` - 饥饿效果阻止恢复测试
  - `TEST(FoodStats, NaturalRegenerationDisabled)` - 禁用自然恢复测试

## 容易踩的坑

1. **饱和度计算公式**：`saturation += food * modifier * 2.0`，不是直接使用 modifier 值
2. **消耗值上限**：必须限制在 40.0 以防止溢出
3. **生命恢复条件**：需要检查玩家是否有饥饿效果
4. **难度检查**：不同难度的饥饿伤害行为不同
