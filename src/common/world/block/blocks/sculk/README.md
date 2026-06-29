#幽匿方块(Sculk Blocks)

深暗之域的生物方块系列，包含幽匿脉络、幽匿感测体、幽匿催化体和幽匿尖啸体。

##目录结构

``` sculk /
├── README.md #本文档
├── SculkBlocks.hpp #所有幽匿方块类声明
└── SculkBlocks.cpp #所有幽匿方块类实现
```

        ##内部模块关系

    - `SculkSensorBlock` 继承 `Block` + `IWaterLoggable`，提供红石信号输出和振动激活逻辑 - `CalibratedSculkSensorBlock` 继承 `SculkSensorBlock`，增加
        FACING 方向过滤和更短活跃期 - `SculkCatalystBlock` 继承 `Block`，具有
            BLOOM 状态和比较器输出 - `SculkShriekerBlock` 继承 `Block` + `IWaterLoggable`，具有 SHRIEKING /
        CAN_SUMMON 状态

            方块实体位于 `src /
        common / world / blockentity / sculk /`，服务端振动集成位于 `src / server / world / blockentity / sculk /`。

        ##上下游外部依赖关系

        ## #依赖

    - `BlockStateProperties` — 方块状态属性（SCULK_SENSOR_PHASE,
    POWER_0_15,
    WATERLOGGED
        等） - `RedstoneSystem` — 红石信号更新通知 - `VibrationSystem` — 振动频率→共鸣事件映射（`getResonanceEventByFrequency`、`getRedstoneStrengthForDistance`） - `TickManager` — 方块
            tick 调度 - `BlockTags::
                VIBRATION_RESONATORS` — 共振方块标签（紫水晶块等） - `GameEvents` — 游戏事件常量（SCULK_SENSOR_TENDRILS_CLICKING
                    等）

    ## #被依赖

    - `SculkVibrationSystem` — 通过 `SculkSensorBlock::activate()` / `canActivate()` 调用方块逻辑 -
    红石系统 — 通过 `getWeakPower()` / `getComparatorInputOverride()` 读取信号

        ##容易踩的坑

    -
    **红石信号 vs 比较器输出 *
         *：红石粉信号基于振动距离（`POWER_0_15` 状态），比较器输出基于振动频率（`lastVibrationFrequency`），两者含义不同，仅在
     Active 状态时有输出
    - **校准感测体方向 **：`CalibratedSculkSensorBlock` 的 FACING 方向是输入面，红石信号只在非 FACING 方向输出 -
    **状态机时序 **：Inactive → Active(30 / 10tick) → Cooldown(10tick) → Inactive，总周期 40 / 20 tick -
    **canReceiveVibration 中 pos 参数 **：是振动源位置（非传感器位置），BLOCK_DESTROY
        / BLOCK_PLACE 仅在源位置等于传感器位置时过滤
