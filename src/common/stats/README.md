#统计常量

定义 Minecraft 1.16.5 中所有自定义统计的资源位置常量，供 `Player::awardCustomStat()` 使用。

        ##目录结构

``` src
        / common / stats /
├── Stats.hpp #自定义统计常量（与 MC Java Stats.java 对应）
└── README.md
```

                   ##内部模块关系

                   无，本目录仅包含常量定义。

                   ##上下游外部依赖关系

                       ** 上游依赖：* *
        - `common / resource / ResourceLocation.hpp` -
    资源位置类型

            ** 下游依赖：* *
        - `common / entity / entities / player / Player.hpp` - `awardCustomStat()` 虚方法
    - `server / player / ServerPlayer.hpp` - `awardCustomStat()` 重写实现
    - `server / stats / StatisticsManager.hpp` - `incrementCustom()` 底层存储
    - 各种方块（BarrelBlock、BrewingStandBlock 等） -
    调用 `player.awardCustomStat()`

    ##容易踩的坑

    - **常量类型**：所有常量为 `inline constexpr const char*`，使用时需通过 `ResourceLocation(stats::XXX)` 构造 -
    **注册对应**：此处的常量字符串必须与 `server / stats / StatRegistry.cpp` 中注册的完全一致 -
    **统计服务端**：`awardCustomStat` 在客户端 Player 基类中为空实现，仅 ServerPlayer 实际更新统计
