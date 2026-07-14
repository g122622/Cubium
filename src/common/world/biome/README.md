#Biome 模块

本目录包含 Cubium 项目的生物群系（Biome）系统，负责生物群系定义、气候参数、生成和分布。该系统完整复刻了 Minecraft
            Java 1.21.11 的多噪声（MultiNoise）3D 生物群系生成算法。

        ##目录结构

``` biome /
├── Biome.hpp #生物群系定义类
├── Biome.cpp
├── BiomeClimate.hpp #生物群系气候参数（温度 /
        湿度 / 降水等）
├── BiomeClimate.cpp
├── BiomeIds.hpp #BiomeId 类型别名 +
    Biomes 命名空间常量
├── BiomeIds.cpp
├── BiomeAmbientSounds.hpp #生物群系环境音效配置
├── BiomeEffects.hpp #生物群系视觉效果（水色、雾色、天色等）— 仅头文件，无.cpp
├── BiomeGenerationSettings.hpp #生物群系生成设置（特征配置）
├── BiomeGenerationSettings.cpp
├── BiomeRegistry.hpp #生物群系注册表（单例）
├── BiomeRegistry.cpp
├── BiomeLoader.hpp #数据驱动 biome JSON 加载器（叠加 climate/effects/spawners/features 到已注册 Biome）
├── BiomeLoader.cpp
├── BiomeFactory.hpp #生物群系工厂函数声明
├── BiomeFactoryOverworld.cpp #主世界生物群系工厂函数
├── BiomeFactoryNether.cpp #下界生物群系工厂函数
├── BiomeFactoryEnd.cpp #末地及新生物群系工厂函数
├── BiomeTag.hpp #生物群系标签类型定义
├── BiomeTag.cpp
├── BiomeTags.hpp #原版生物群系标签常量（IS_OCEAN、IS_RIVER 等）
├── BiomeTags.cpp
├── BiomeTagLoader.hpp #生物群系标签加载器（从数据包读取标签定义）
├── BiomeTagLoader.cpp
├── BiomeSource.hpp #生物群系源基类(IBiomeSource)，兼容别名 BiomeSource
├── BiomeSource.cpp
├── BiomeManager.hpp #生物群系管理器（Voronoi 缩放查询）
├── BiomeManager.cpp
├── Biomes.hpp #生物群系系统聚合头文件
├── climate / #Climate 参数系统
│   ├── ParameterTypes.hpp #Parameter, TargetPoint, ParameterPoint, 常量, 辅助函数
│   ├── RTree.hpp #7 维参数空间 RTree 索引模板族
│   ├── ParameterList.hpp #ParameterList<T> 最近邻查找
│   ├── Sampler.hpp / Sampler.cpp #气候采样器（持有 6 个密度函数）
│   ├── SpawnFinder.hpp / SpawnFinder.cpp #出生点径向搜索
│   └── README.md
├── source /
        #生物群系源实现
│   ├── MultiNoiseBiomeSource.hpp #多噪声生物群系源（主世界、下界）
│   ├── MultiNoiseBiomeSource.cpp
│   ├── OverworldBiomeBuilder.hpp #主世界生物群系参数构建器
│   ├── OverworldBiomeBuilder.cpp
│   ├── NetherBiomeBuilder.hpp #下界生物群系参数构建器
│   ├── NetherBiomeBuilder.cpp
│   ├── EndBiomeSource.hpp #末地生物群系源
│   ├── EndBiomeSource.cpp
│   └── README.md
└── README.md #本文件
```

        ##内部模块关系

```
┌─────────────────────────────────────────────────────────┐
│ ChunkGenerator                          │
│                       │                                 │
│                       ▼                                 │
│ BiomeManager ←─────────────────────────┐  │
│                       │ Voronoi 缩放查询              │  │
│                       ▼                               │  │
│ IBiomeSource ──────────────────────────┘  │
│                       │                                 │
│         ┌─────────────┼─────────────┐                   │
│         │             │             │                   │
│ MultiNoiseBiome EndBiome NetherBiome                 │
│         │             │             │                   │
│ OverworldBuilder EndIslands ParameterList             │
│         │                                               │
│ ParameterList<BiomeId>                                 │
│         │                                               │
│ Climate::Sampler ← NoiseRouter ← DensityFunctions     │
└─────────────────────────────────────────────────────────┘
```

        ## #BiomeManager 说明 BiomeManager 将 quart 分辨率的噪声生物群系（4x4x4 网格）通过 Voronoi
        缩放提升到方块分辨率：
    - `getBiome(x, y, z)` — 方块级查询，带 LCG fiddling 使边界自然
    - `getNoiseBiomeAtQuart(qx, qy, qz)` — quart 级直接查询，无缩放 - `obfuscateSeed(worldSeed)` — SHA -
    256 哈希种子，防止逆向世界种子

    ##外部依赖关系

    ## #上游依赖
    - `common / core / Types.hpp` — 基础类型 - `common / world / gen / density /` — 密度函数管线（NoiseRouter）
    - `common / world / chunk /` — 区块数据结构 - `common / world / block /` — 方块定义
    - `common / world / spawn /` — 生物生成设置

        ## #下游依赖
    - `common / world / gen / chunk /` — 区块生成器 - `server / world /` — 服务端世界管理
    - `client / world /` — 客户端世界渲染

        ##容易踩的坑

        ## #1. quart 坐标 vs 方块坐标 quart 坐标 = 方块坐标 /
        4。`getNoiseBiome()` 接收 quart 坐标，`fillBiomeContainer()` 内部自动转换。

        ## #2. 多噪声系统的 6 个气候参数
    | 参数 | 范围 | 说明 | | -- -- --| -- -- --| -- -- --| | Temperature | [-1, 1] | 温度 | | Humidity | [-1, 1] | 湿度
    | | Continentalness | [-1.2, 1.0] | 大陆度（海洋→内陆） | | Erosion | [-1, 1] | 侵蚀度 | | Depth |
    [-1, 1] | 深度（表面 / 地下） | | Weirdness | [-1, 1] | 奇异度（山谷 / 山峰） |

    每个生物群系注册时定义一组 Climate Parameter 范围，系统通过最近邻匹配（fitness 计算）确定最匹配的生物群系。

        ## #3. OverworldBiomeBuilder 的 pick 方法使用 Parameter 而非 f64
`pickMiddleBiome` 等方法接收 `Climate.Parameter`（奇异度范围），通过 `weirdness.max
        >= 0` 判断正 /
                负奇异度来选择变体生物群系。

                ## #4. 13 个奇异度切片
`addInlandBiomes()` 注册 13 个奇异度切片，覆盖完整的[-1, 1] 范围：
            - 4 个 MidSlice、4 个 HighSlice、2 个 Peaks、2 个 LowSlice、1 个 Valleys -
            Valleys 切片负责河流 / 冻河 /
                沼泽的生成

                ## #5. 中央岛屿判定使用区块坐标

## #6. getFlowerFeatureIds() 与 addFlowerFeature()
`BiomeGenerationSettings::getFlowerFeatureIds()` 返回通过 `addFlowerFeature()` 添加的花卉 placed_feature `ResourceLocation` 列表。`addFlowerFeature()` 仅追加到独立花卉列表，不再调用 `addPlacedFeature`，调用方（`BiomeLoader::applyFeatures`）负责保证花卉 placed_feature 同时通过 `addPlacedFeature` 登记到对应阶段。`BiomeLoader::applyFeatures` 解析 features 二维数组时，对底层 configured_feature 为 `ConfiguredFlowerFeature` 的条目同时调用两者。GrassBlock::grow 骨粉催花时从花卉列表随机选取 placed_feature id，经 `PlacedFeatureRegistry` 解析后取 `feature()` 拿到 `ConfiguredFlowerFeature`，再从其配置中随机选择花朵方块状态。调用 `clear()` 会清空花卉特征列表。

### 中央岛屿判定使用区块坐标
`EndBiomeSource::isInCentralIsland()` 使用区块坐标（`blockX >> 4`），不是方块坐标。4096 = 64²（64 个区块半径）。

    ## #6. Parameter::span 支持两种重载
    - `Parameter::span(f32 min, f32 max)` — 从浮点值创建范围
    - `Parameter::span(const Parameter& first, const Parameter& second)` — 从两个参数的 min /
        max 创建范围

        ## #7. BiomeContainer 已移除 原 `BiomeContainer.hpp
        /
        cpp` 已移除，生物群系存储现由 `IChunk` 内部管理。

### 8. 已删除的文件和接口
- **BiomeEffects.cpp** — 已删除， 现为 header-only（所有方法内联）
- **Biome::Category** — 已删除，生物群系分类改由 BiomeTags 系统替代
- **isOceanOrRiverBiome()** — 已删除，改用  /  判断
- **Biome::temperature()** — 已移除，请使用  替代

### 9. BiomeTag 标签系统
// 提供基于标签的生物群系分类，替代了旧的  枚举和硬编码判断函数（如 ）：
-  — 标签类型定义，每个标签对应一组生物群系 ID
-  — 原版标签常量（、、 等）
-  — 从数据包加载标签定义的加载器

### 10. shouldFreeze/shouldSnow 与 doesWaterFreeze/doesSnowGenerate 的区别
-  — 完整实现，需要 IWorld，检查温度、光照、流体类型、邻居水域
-  — 完整实现，需要 IWorld，检查降水类型、温度、光照、方块状态，并通过  检查下方方块是否支撑雪层
-  — 仅温度判断，不需要 IWorld，适用于生成阶段（无光照/方块状态可用时）
-  — 仅温度判断，不需要 IWorld，适用于 SurfaceRules 等生成阶段

**shouldFreeze/shouldSnow 已集成调用：SnowAndFreezeFeature（TopLayerModification 生成阶段）、ServerWorld::tickPrecipitation()（运行时逐 tick 结冰和降雪）、LakeFeature（湖泊冻结，checkNeighbors=false）。**

### 11. getPrecipitationAt 降水类型判定

根据生物群系的降水设置和高度调整后的温度，返回指定位置的降水类型：

- 如果生物群系 `hasPrecipitation() == false`，返回 `Precipitation::None`
- 如果高度调整后的温度 < 0.15（即 `shouldFreeze()` 返回 true），返回 `Precipitation::Snow`
- 否则返回 `Precipitation::Rain`

**注意**：`BiomeClimate` 中的 `Precipitation precipitation` 字段已重构为 `bool hasPrecipitation`。`Precipitation` 枚举（`None`/`Rain`/`Snow`）仍作为 `getPrecipitationAt()` 的返回类型使用，但不再作为 `BiomeClimate` 的字段。设置降水属性使用 `setHasPrecipitation(bool)` 而非旧的 `setPrecipitation()`。

**调用场景**：`tickPrecipitation()` 在降水 tick 中调用此方法确定表面方块位置的降水类型，然后调用 `Block::handlePrecipitation()` 将降水事件传递给方块（如炼药锅填充水、避雷针雷暴激活等）。

### 12. BiomeLoader 数据驱动叠加策略
BiomeLoader 从数据包 biome JSON 加载 climate/effects/spawners/spawn_costs/carvers/features，**叠加**到 BiomeFactory 已构造的 Biome 上（保留 depth/scale/surface blocks 等非 JSON 字段）。必须按以下顺序在 `MinecraftServer::initializeRegistries` 调用：PlacementRegistry::initialize → FeatureTypeRegistry::initializeBuiltinFeatureTypes → ConfiguredFeatureLoader → PlacedFeatureLoader → ConfiguredCarverLoader → BiomeRegistry::initialize → BiomeLoader。BiomeLoader 内置 65 项 biome 名→BiomeId 映射表（含 11 个 1.18+ 重命名如 stony_shore=StoneShore）；数据包 biome 名无映射或 BiomeId 未注册时 warn+skip。placed_feature/carver id 未在对应 Registry 注册时 warn+skip（世界仍可生成）。

顶层 `creature_spawn_probability` 字段（[0,1]，snowy_plains/badlands/ice_spikes 等为 0.03-0.07，其余默认 0.1）由 `resolveCreatureSpawnProbability` 解析到 `MobSpawnInfo.creatureSpawnProbability`，是动物生成概率的**唯一数据来源**（对应原版 `MobSpawnSettings.getCreatureProbability()`）。`applySpawners` 用全新 MobSpawnInfo 覆盖时会保留 BiomeFactory 工厂方法（如 createSnowy 的 0.07）的设定作为默认值，再用 JSON 字段覆盖；无 `spawners` 字段时仍独立应用到现有 spawnInfo。`spawners` 条目校验 weight>0、minCount<=maxCount、minCount>=0，不合法条目 warn+skip。`Biome::creatureSpawnProbability()` 仅代理 `spawnInfo().getCreatureSpawnProbability()`，无独立字段。
