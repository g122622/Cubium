# 幽匿方块 (Sculk Blocks)

深暗之域的生物方块系列，包含幽匿脉络、幽匿感测体、幽匿催化体和幽匿尖啸体。

## 目录结构

```
sculk/
├── README.md          # 本文档
├── SculkBlocks.hpp    # 所有幽匿方块类声明
└── SculkBlocks.cpp    # 所有幽匿方块类实现
```

## 内部模块关系

- `SculkBlock` 继承 `Block`，普通幽匿方块
- `SculkVeinBlock` 继承 `Block`，幽匿脉络
- `SculkSensorBlock` 继承 `Block` + `IWaterLoggable`，提供红石信号输出和振动激活逻辑
- `CalibratedSculkSensorBlock` 继承 `SculkSensorBlock`，增加 FACING 方向过滤和更短活跃期
- `SculkCatalystBlock` 继承 `Block`，具有 BLOOM 状态和比较器输出
- `SculkShriekerBlock` 继承 `Block` + `IWaterLoggable`，具有 SHRIEKING / CAN_SUMMON 状态

方块实体位于 `src/common/world/blockentity/sculk/`，服务端振动集成位于 `src/server/world/blockentity/sculk/`。

## 上下游外部依赖关系

### 依赖

- `BlockStateProperties` — 方块状态属性（SCULK_SENSOR_PHASE, POWER_0_15, WATERLOGGED 等）
- `RedstoneSystem` — 红石信号更新通知
- `VibrationSystem` — 振动频率→共鸣事件映射（`getResonanceEventByFrequency`、`getRedstoneStrengthForDistance`）
- `TickManager` — 方块 tick 调度
- `BlockTags::VIBRATION_RESONATORS` — 共振方块标签（紫水晶块等）
- `GameEvents` — 游戏事件常量（SCULK_SENSOR_TENDRILS_CLICKING、SHRIEK 等）
- `WorldEvents` — 世界事件常量（SCULK_SHRIEK = 3007 粒子效果等）

### 被依赖

- `SculkVibrationSystem` — 通过 `SculkSensorBlock::activate()` / `canActivate()` 调用方块逻辑
- `SculkShriekerHelper` — 通过 `SculkShriekerBlock::shriek()` 激活尖啸体效果
- 红石系统 — 通过 `getWeakPower()` / `getComparatorInputOverride()` 读取信号

## SculkShriekerBlock 激活流程

幽匿尖啸体的完整激活流程如下：

1. **触发**：非潜行实体踩上方块 → `onEntityWalk()` 发出 `SHRIEK` 游戏事件
2. **振动传播**：`SHRIEK` 事件传播到附近尖啸体 → `SculkShriekerVibrationUser::onReceiveVibration()` → `SculkShriekerHelper::tryShriek()`
3. **激活**：`tryShriek()` 解析玩家、检查条件后调用 `SculkShriekerBlock::shriek()`
4. **尖啸效果**：`shriek()` 设置 SHRIEKING=true、调度 90 tick 后 tick、播放粒子效果、发出 SHRIEK 事件
5. **状态转换**：90 tick 后 `tick()` 将 SHRIEKING 设回 false，设置 `shriekingFinished` 标志
6. **响应**：`SculkVibrationManager::tickAll()` 检测标志 → `SculkShriekerHelper::checkShriekingFinished()` → `tryRespond()`
7. **召唤/警告**：`tryRespond()` 尝试召唤监守者、播放警告声音、对附近玩家施加黑暗效果

### SculkShriekerBlock 方法说明

| 方法 | 说明 | 对齐 MC Java |
|------|------|-------------|
| onEntityWalk() | 实体踩上方块时发出 SHRIEK 事件 | SculkShriekerBlock.stepOn() |
| tick() | SHRIEKING 状态到期后转回非 SHRIEKING | SculkShriekerBlock.tick() |
| onBlockRemoved() | SHRIEKING 状态下被移除时仍触发响应 | SculkShriekerBlock.preRemoveSideEffects() |
| shriek() | 设置 SHRIEKING 状态、播放粒子、发出 SHRIEK 事件 | SculkShriekerBlockEntity.shriek() |

SHRIEKING_TICKS = 90（4.5 秒），对齐 MC Java。

## 容易踩的坑

- **红石信号 vs 比较器输出**：红石粉信号基于振动距离（`POWER_0_15` 状态），比较器输出基于振动频率（`lastVibrationFrequency`），两者含义不同，仅在 Active 状态时有输出
- **校准感测体方向**：`CalibratedSculkSensorBlock` 的 FACING 方向是输入面，红石信号只在非 FACING 方向输出
- **状态机时序**：Inactive → Active(30/10 tick) → Cooldown(10 tick) → Inactive，总周期 40/20 tick
- **canReceiveVibration 中 pos 参数**：是振动源位置（非传感器位置），BLOCK_DESTROY/BLOCK_PLACE 仅在源位置等于传感器位置时过滤
- **尖啸体 SHRIEKING 不可重入**：已在 SHRIEKING 状态的尖啸体不会重复激活（`onEntityWalk` 和 `tryShriek` 均有检查）
- **尖啸体 tick 与响应分离**：`SculkShriekerBlock::tick()` 在 mc_common 层设置 `shriekingFinished` 标志，`SculkShriekerHelper::checkShriekingFinished()` 在 mc_server 层检测并执行响应逻辑
