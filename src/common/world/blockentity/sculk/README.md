# 幽匿方块实体 (Sculk Block Entities)

幽匿感测体和幽匿尖啸体的方块实体实现，负责振动检测数据的持久化和游戏逻辑。

## 文件结构

```
sculk/
├── README.md                    # 本文档
├── SculkSensorBlockEntity.hpp   # 幽匿感测体方块实体
├── SculkSensorBlockEntity.cpp
├── SculkShriekerBlockEntity.hpp # 幽匿尖啸体方块实体
└── SculkShriekerBlockEntity.cpp
```

## 架构

### VibrationSystem 集成

两个方块实体都继承 `VibrationSystem`（多重继承），实现振动检测：

- **SculkSensorBlockEntity**：检测所有频率的振动，输出红石信号
- **SculkShriekerBlockEntity**：仅响应 `SHRIEK` 事件，递增警告等级

### 序列化格式

与 MC 原版兼容的 NBT/JSON 序列化格式：

#### SculkSensorBlockEntity

| 键名               | 类型     | 说明                                       |
|--------------------|----------|------------------------------------------|
| `listener`         | compound | VibrationSystem.Data 振动系统数据            |
| `last_vibration_frequency` | int | 最后接收的振动频率 (0-15)                   |

#### SculkShriekerBlockEntity

| 键名            | 类型     | 说明                                  |
|----------------|----------|-------------------------------------|
| `listener`     | compound | VibrationSystem.Data 振动系统数据       |
| `warning_level` | int     | 警告等级 (0-4)                        |

#### VibrationSystem.Data (listener)

| 键名          | 类型     | 说明                                              |
|--------------|----------|-------------------------------------------------|
| `event`      | compound | 当前传播的振动信息（可选）                          |
| `selector`   | compound | 振动选择器                                        |
| `event_delay` | int     | 传播剩余时间（tick）                              |

注意：`reloadVibrationParticle` 不序列化，反序列化时硬编码为 `true`。

#### VibrationInfo (event)

| 键名          | 类型          | 说明                          |
|--------------|--------------|-----------------------------|
| `game_event` | string       | 事件 ID（如 "minecraft:step"） |
| `distance`   | float        | 振动传播距离                   |
| `pos`        | list<double> | 振动源位置 [x, y, z]          |
| `source`     | long         | 源实体 ID（可选）             |

## 待实现

- SculkSensorBlockEntity：振动接收后触发方块状态变化（ACTIVE_PHASE）和红石信号更新
- SculkShriekerBlockEntity：警告等级达到阈值时召唤监守者（WardenEntity）
- 服务端集成：在 ServerWorld::setBlockEntity() 中注册 VibrationSystem::Listener 到 GameEventListenerRegistry，
  在 ServerWorld::tickBlockEntities() 中驱动 VibrationSystem::Ticker::tick()，
  在 ServerWorld::removeBlockEntity() 中注销 Listener。
  参见服务端文件 SculkSensorBlockEntityServer.cpp 和 SculkShriekerBlockEntityServer.cpp 中的 TODO 注释。
