# 食物/饥饿系统

本目录实现了玩家的饥饿和饱食度系统。

## 目录结构

```
src/common/entity/food/
├── FoodStats.hpp       # 饥饿系统核心类，管理饥饿值、饱和度、消耗值和生命恢复
├── FoodStats.cpp       # 饥饿系统实现
└── README.md           # 本文档
```

## 内部模块关系

本目录只有一个核心类 `FoodStats`，无内部模块划分。

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/core/Types.hpp` - 基础类型定义（i32, f32, Difficulty）
- `common/core/Result.hpp` - 结果类型（用于 NBT 序列化）
- `entity/entities/player/Player.hpp` - 玩家实体（前向声明）
- `entity/combat/DifficultyHelper.hpp` - 难度辅助（饥饿伤害最小生命值）
- `entity/damage/DamageSource.hpp` - 伤害来源（饥饿伤害）
- `entity/effect/EffectType.hpp` - 效果类型（饥饿效果检查）

### 下游依赖（依赖本模块）

- `entity/entities/player/Player.hpp` - Player 类持有 FoodStats 实例
- `item/food/FoodItem.hpp` - 食物物品使用完成后调用 `addStats()`
- `world/storage/player/PlayerDataManager.cpp` - 玩家数据存取
- `server/world/player/ServerPlayer.hpp` - 服务端玩家饥饿同步

## 容易踩的坑

1. **饱和度计算公式**：`saturation += food * modifier * 2.0`，不是直接使用 modifier 值。例如苹果（food=4, modifier=0.3）提供 4 * 0.3 * 2 = 2.4 饱和度。
2. **消耗值上限**：必须限制在 40.0 以防止溢出，严格大于 4.0 时消耗 1 点饱和度或饥饿值（一次 tick 最多扣一次，对齐 vanilla `if` 而非 `while`）。
3. **生命恢复条件**：快速恢复需 `foodLevel >= 20 && saturation > 0`；慢速恢复需 `foodLevel >= 18`；两者均受 `NATURAL_REGENERATION` 游戏规则门控，但**不查 Hunger 效果**——Hunger 效果仅加速 exhaustion 累积，不阻止回血（对齐 vanilla FoodData.tick）。
4. **难度检查**：不同难度的饥饿伤害行为不同（和平无伤害、简单最低 10 生命、普通最低 1 生命、困难可饿死）。
5. **和平模式**：和平难度无特例分支——foodLevel 恒 20（消耗不扣）、saturation 会被消耗，走通用满饱快回血/慢回血分支（对齐 vanilla，旧实现的"每 20 tick 回 1 HP + 每 10 tick 回 1 foodLevel"特例已移除）。
6. **单一计时器**：`m_foodTimer` 对齐 vanilla 单 `tickTimer` 语义，共享回血/饿死/else-reset 三分支（状态切换时进度归零）。经 `foodTimer()/setFoodTimer()` 序列化为 `foodTickTimer`，与 vanilla NBT 字段一致。旧的独立 `m_starveTimer` 已移除。
7. **prevFoodLevel**：用于 UI 动画同步，每 tick 开始时更新。

## 参考

- MC Java 1.21.11 `net.minecraft.world.food.FoodData`（tick 方法，权威实现）
- MC Java 1.21.11 `net.minecraft.world.food.FoodConstants`（常量定义）
- MC Java 1.21.11 `net.minecraft.world.entity.player.Player`（causeFoodExhaustion / isHurt）
