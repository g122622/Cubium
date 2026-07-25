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
2. **消耗值上限**：必须限制在 40.0 以防止溢出，超过 4.0 时消耗饱和度或饥饿值。
3. **生命恢复条件**：快速恢复需 `foodLevel >= 20 && saturation > 0`；慢速恢复需 `foodLevel >= 18`；都需要检查玩家是否有饥饿效果。
4. **难度检查**：不同难度的饥饿伤害行为不同（和平无伤害、简单最低 10 生命、普通最低 1 生命、困难可饿死）。
5. **和平模式**：和平模式下饥饿值会自动恢复，但仍会消耗饱和度。
6. **计时器分离**：`m_foodTimer` 用于生命恢复，`m_starveTimer` 用于饥饿伤害，两者独立运行。
7. **prevFoodLevel**：用于 UI 动画同步，每 tick 开始时更新。

## 参考

- MC 1.16.5 `net.minecraft.util.FoodStats`
- MC 1.16.5 `net.minecraft.entity.player.PlayerEntity` (tick 方法中的饥饿处理)
