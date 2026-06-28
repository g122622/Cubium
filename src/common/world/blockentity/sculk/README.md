#幽匿方块实体(Sculk Block Entities)

幽匿感测体和幽匿尖啸体的方块实体实现，负责振动检测数据的持久化和游戏逻辑。

##文件结构

``` sculk /
├── README.md #本文档
├── SculkSensorBlockEntity.hpp #幽匿感测体方块实体
├── SculkSensorBlockEntity.cpp
├── SculkShriekerBlockEntity.hpp #幽匿尖啸体方块实体
└── SculkShriekerBlockEntity.cpp
```

                服务端振动系统集成文件位于 `src /
                server / world / blockentity /
                sculk /`： - `SculkVibrationSystem.hpp /
                                 .cpp` — SculkVibrationManager、SculkVibrationSystem、
                                     SculkSensorVibrationUser、SculkShriekerVibrationUser

                                 ##架构

                                 ## #VibrationSystem 集成

                                     由于方块实体位于 mc_common（不能依赖 mc_server），而 VibrationSystem::Listener 的
                                     handleGameEvent() 需要依赖 mc_server 中的代码，因此 VibrationSystem 的集成采用
                                 "附件"模式：

            - **mc_common 层**：SculkSensorBlockEntity
                / SculkShriekerBlockEntity 持有 VibrationSystem::Data， 提供序列化 / 反序列化，但不持有 Listener 或 User
            -
            **mc_server 层**：SculkVibrationSystem（VibrationSystem 子类）持有 User 和 Listener， 通过引用访问方块实体的
                Data；SculkVibrationManager 管理附件的生命周期

                SculkVibrationManager 在 ServerWorld 中的三个关键时机介入： 1. `ServerWorld::setBlockEntity()` — 创建
                VibrationSystem 附件，注册 Listener 到 GameEventListenerRegistry
                2. `ServerWorld::tickBlockEntities()` — 驱动
                VibrationSystem::Ticker::tick() 3. `ServerWorld::removeBlockEntity()` — 注销 Listener，销毁附件

                对齐 MC Java : -LevelChunk.addAndRegisterBlockEntity() → SculkVibrationManager::registerSculkSensor
                / registerSculkShrieker
            - LevelChunk.removeBlockEntity() → SculkVibrationManager::unregisterSculkBlockEntity -
            Block.getTicker() → VibrationSystem.Ticker.tick() → SculkVibrationManager::tickAll

            ## #振动行为

            - **SculkSensorVibrationUser**：检测所有频率的振动（频率
        > 0），潜行可规避特定事件， 接收后更新 lastVibrationFrequency，可触发 AvoidVibration 进度 -
            **SculkShriekerVibrationUser**：仅响应 SHRIEK 事件，接收后递增警告等级， 不可触发 AvoidVibration 进度

                ## #序列化格式

                与 MC 原版兼容的 NBT
                /
                JSON 序列化格式：

                ####SculkSensorBlockEntity

    | 键名 | 类型 | 说明 | | -- -- -- -- -- -- -- -- -- --| -- -- -- -- --|
    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --| | `listener` | compound
    | VibrationSystem.Data 振动系统数据 | | `last_vibration_frequency` | int | 最后接收的振动频率(0 - 15) |

    ####SculkShriekerBlockEntity

    | 键名 | 类型 | 说明 | | -- -- -- -- -- -- -- --| -- -- -- -- --|
    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -| | `listener` | compound | VibrationSystem.Data 振动系统数据
    | | `warning_level` | int | 警告等级(0 - 4) |

    ####VibrationSystem.Data(listener)

    | 键名 | 类型 | 说明 | | -- -- -- -- -- -- --| -- -- -- -- --|
    -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -| | `event` | compound
    | 当前传播的振动信息（可选） | | `selector` | compound | 振动选择器 | | `event_delay` | int | 传播剩余时间（tick） |

    注意：`reloadVibrationParticle` 不序列化，反序列化时硬编码为 `true`。

    ####VibrationInfo(event)

    | 键名 | 类型 | 说明 | | -- -- -- -- -- -- --| -- -- -- -- -- -- --| -- -- -- -- -- -- -- -- -- -- -- -- -- -- -|
    | `game_event` | string | 事件 ID（如 "minecraft:step"） | | `distance` | float | 振动传播距离 |
    | `pos` | list<double> | 振动源位置[x, y, z] | | `source` | long | 源实体 ID（可选） |

    ##待实现

        - SculkSensorBlockEntity：振动接收后触发方块状态变化（ACTIVE_PHASE）和红石信号更新
        - SculkShriekerBlockEntity：警告等级达到阈值时召唤监守者（WardenEntity）
