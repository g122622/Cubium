# 幽匿方块实体 (Sculk Block Entities)

幽匿感测体和幽匿尖啸体的方块实体实现，负责振动检测数据的持久化和游戏逻辑。

## 文件结构

```
sculk/
├── README.md                       # 本文档
├── SculkSensorBlockEntity.hpp      # 幽匿感测体方块实体
├── SculkSensorBlockEntity.cpp
├── SculkShriekerBlockEntity.hpp    # 幽匿尖啸体方块实体
└── SculkShriekerBlockEntity.cpp
```

服务端振动系统集成文件位于 `src/server/world/blockentity/sculk/`：
- `SculkVibrationSystem.hpp/.cpp` — SculkVibrationManager、SculkVibrationSystem、SculkSensorVibrationUser、SculkShriekerVibrationUser

## 架构

### VibrationSystem 集成

由于方块实体位于 mc_common（不能依赖 mc_server），而 VibrationSystem::Listener 的 handleGameEvent() 需要依赖 mc_server 中的代码，因此 VibrationSystem 的集成采用"附件"模式：

- **mc_common 层**：SculkSensorBlockEntity / SculkShriekerBlockEntity 持有 VibrationSystem::Data，提供序列化/反序列化，但不持有 Listener 或 User
- **mc_server 层**：SculkVibrationSystem（VibrationSystem 子类）持有 User 和 Listener，通过引用访问方块实体的 Data；SculkVibrationManager 管理附件的生命周期

SculkVibrationManager 在 ServerWorld 中的三个关键时机介入：
1. `ServerWorld::setBlockEntity()` — 创建 VibrationSystem 附件，注册 Listener 到 GameEventListenerRegistry
2. `ServerWorld::tickBlockEntities()` — 驱动 VibrationSystem::Ticker::tick()
3. `ServerWorld::removeBlockEntity()` — 注销 Listener，销毁附件

对齐 MC Java:
- LevelChunk.addAndRegisterBlockEntity() → SculkVibrationManager::registerSculkSensor / registerSculkShrieker
- LevelChunk.removeBlockEntity() → SculkVibrationManager::unregisterSculkBlockEntity
- Block.getTicker() → VibrationSystem.Ticker.tick() → SculkVibrationManager::tickAll

### 振动接收后的完整激活流程

1. **SculkSensorVibrationUser::onReceiveVibration()** — 振动到达时：
   - 更新 `lastVibrationFrequency`
   - 检查 `canActivate`（Phase 必须为 Inactive）
   - 计算红石信号强度 `getRedstoneStrengthForDistance(distance, radius)`
   - 调用 `SculkSensorBlock::activate()` 激活感测体

2. **SculkSensorBlock::activate()** — 激活逻辑：
   - 设置方块状态：`SCULK_SENSOR_PHASE = Active`，`POWER = redstoneStrength`
   - 调度 tick（普通感测体30tick，校准感测体10tick）
   - 通知邻居红石信号变化
   - 触发共振事件（相邻紫水晶块）
   - 发出 `SCULK_SENSOR_TENDRILS_CLICKING` 游戏事件

3. **SculkSensorBlock::tick()** — 状态转换：
   - Active → `deactivate()` → Cooldown（POWER=0，调度10tick）
   - Cooldown → Inactive（播放停止声音，可再次被激活）

4. **SculkSensorBlock::onBlockRemoved()** — 方块移除时如果处于 Active 状态，通知邻居红石信号归零

### 比较器输出

- **红石粉信号**：基于振动距离计算（1-15），存储在 `POWER_0_15` 方块状态中
- **比较器信号**：基于振动频率（1-15），从 BlockEntity 的 `lastVibrationFrequency` 读取，仅在 Active 状态时输出

### 振动行为

- **SculkSensorVibrationUser**：检测所有频率的振动（频率>0），潜行可规避特定事件，接收后更新 lastVibrationFrequency，可触发 AvoidVibration 进度
- **SculkShriekerVibrationUser**：仅响应 SHRIEK 事件，接收后递增警告等级，不可触发 AvoidVibration 进度

### 校准幽匿感测体 (CalibratedSculkSensorBlock)

- ACTIVE_TICKS = 10（普通感测体为30）
- FACING 方向为输入面，红石信号仅在非 FACING 方向输出
- canReceiveVibration 中根据背面红石信号过滤振动频率

## 序列化格式

与 MC 原版兼容的 NBT/JSON 序列化格式：

#### SculkSensorBlockEntity

| 键名 | 类型 | 说明 |
|------|------|------|
| `listener` | compound | VibrationSystem.Data 振动系统数据 |
| `last_vibration_frequency` | int | 最后接收的振动频率(0-15) |

#### SculkShriekerBlockEntity

| 键名 | 类型 | 说明 |
|------|------|------|
| `listener` | compound | VibrationSystem.Data 振动系统数据 |
| `warning_level` | int | 警告等级(0-4) |

#### VibrationSystem.Data(listener)

| 键名 | 类型 | 说明 |
|------|------|------|
| `event` | compound | 当前传播的振动信息（可选） |
| `selector` | compound | 振动选择器 |
| `event_delay` | int | 传播剩余时间（tick） |

注意：`reloadVibrationParticle` 不序列化，反序列化时硬编码为 `true`。

#### VibrationInfo(event)

| 键名 | 类型 | 说明 |
|------|------|------|
| `game_event` | string | 事件 ID（如 "minecraft:step"） |
| `distance` | float | 振动传播距离 |
| `pos` | list\<double\> | 振动源位置[x, y, z] |
| `source` | long | 源实体 ID（可选） |

## 待实现

- SculkShriekerBlockEntity：警告等级达到阈值时召唤监守者（WardenEntity）
