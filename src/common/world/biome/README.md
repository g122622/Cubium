# Biome 模块

本目录包含 Cubium 项目的生物群系（Biome）系统，负责生物群系定义、气候参数、生成和分布。该系统完整复刻了 Minecraft Java 1.21.11 的多噪声（MultiNoise）3D 生物群系生成算法。

## 目录结构

```
biome/
├── Biome.hpp                    # 生物群系定义类
├── Biome.cpp
├── BiomeClimate.hpp             # 生物群系气候参数（温度/湿度/降水等）
├── BiomeClimate.cpp
├── BiomeIds.hpp                 # BiomeId 类型别名 + Biomes 命名空间常量
├── BiomeIds.cpp
├── BiomeAmbientSounds.hpp       # 生物群系环境音效配置
├── BiomeEffects.hpp             # 生物群系视觉效果（水色、雾色、天色等）— 仅头文件，无 .cpp
├── BiomeGenerationSettings.hpp  # 生物群系生成设置（特征配置）
├── BiomeGenerationSettings.cpp
├── BiomeRegistry.hpp            # 生物群系注册表（单例）
├── BiomeRegistry.cpp
├── BiomeFactory.hpp             # 生物群系工厂函数声明
├── BiomeFactoryOverworld.cpp    # 主世界生物群系工厂函数
├── BiomeFactoryNether.cpp       # 下界生物群系工厂函数
├── BiomeFactoryEnd.cpp          # 末地及新生物群系工厂函数
├── BiomeTag.hpp                 # 生物群系标签类型定义
├── BiomeTag.cpp
├── BiomeTags.hpp                # 原版生物群系标签常量（IS_OCEAN、IS_RIVER 等）
├── BiomeTags.cpp
├── BiomeTagLoader.hpp           # 生物群系标签加载器（从数据包读取标签定义）
├── BiomeTagLoader.cpp
├── BiomeSource.hpp              # 生物群系源基类 (IBiomeSource)，兼容别名 BiomeSource
├── BiomeSource.cpp
├── Biomes.hpp                   # 生物群系系统聚合头文件
├── climate/                     # Climate 参数系统
│   ├── Climate.hpp              # Parameter, ParameterPoint, TargetPoint, ParameterList, Sampler
│   ├── Climate.cpp
│   └── README.md
├── source/                      # 生物群系源实现
│   ├── MultiNoiseBiomeSource.hpp  # 多噪声生物群系源（主世界、下界）
│   ├── MultiNoiseBiomeSource.cpp
│   ├── OverworldBiomeBuilder.hpp  # 主世界生物群系参数构建器
│   ├── OverworldBiomeBuilder.cpp
│   ├── NetherBiomeBuilder.hpp     # 下界生物群系参数构建器
│   ├── NetherBiomeBuilder.cpp
│   ├── EndBiomeSource.hpp         # 末地生物群系源
│   ├── EndBiomeSource.cpp
│   └── README.md
└── README.md                    # 本文件
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────┐
│                  ChunkGenerator                          │
│                       │                                 │
│                       ▼                                 │
│                IBiomeSource                              │
│                       │                                 │
│         ┌─────────────┼─────────────┐                   │
│         │             │             │                   │
│  MultiNoiseBiome  EndBiome  NetherBiome                 │
│         │             │             │                   │
│  OverworldBuilder  EndIslands  ParameterList             │
│         │                                               │
│  ParameterList<BiomeId>                                 │
│         │                                               │
│  Climate::Sampler ← NoiseRouter ← DensityFunctions     │
└─────────────────────────────────────────────────────────┘
```

## 外部依赖关系

### 上游依赖
- `common/core/Types.hpp` — 基础类型
- `common/world/gen/density/` — 密度函数管线（NoiseRouter）
- `common/world/chunk/` — 区块数据结构
- `common/world/block/` — 方块定义
- `common/world/spawn/` — 生物生成设置

### 下游依赖
- `common/world/gen/chunk/` — 区块生成器
- `server/world/` — 服务端世界管理
- `client/world/` — 客户端世界渲染

## 容易踩的坑

### 1. quart 坐标 vs 方块坐标
quart 坐标 = 方块坐标 / 4。`getNoiseBiome()` 接收 quart 坐标，`fillBiomeContainer()` 内部自动转换。

### 2. 多噪声系统的 6 个气候参数
| 参数 | 范围 | 说明 |
|------|------|------|
| Temperature | [-1, 1] | 温度 |
| Humidity | [-1, 1] | 湿度 |
| Continentalness | [-1.2, 1.0] | 大陆度（海洋→内陆） |
| Erosion | [-1, 1] | 侵蚀度 |
| Depth | [-1, 1] | 深度（表面/地下） |
| Weirdness | [-1, 1] | 奇异度（山谷/山峰） |

每个生物群系注册时定义一组 Climate Parameter 范围，系统通过最近邻匹配（fitness 计算）确定最匹配的生物群系。

### 3. OverworldBiomeBuilder 的 pick 方法使用 Parameter 而非 f64
`pickMiddleBiome` 等方法接收 `Climate.Parameter`（奇异度范围），通过 `weirdness.max >= 0` 判断正/负奇异度来选择变体生物群系。

### 4. 13 个奇异度切片
`addInlandBiomes()` 注册 13 个奇异度切片，覆盖完整的 [-1, 1] 范围：
- 4 个 MidSlice、4 个 HighSlice、2 个 Peaks、2 个 LowSlice、1 个 Valleys
- Valleys 切片负责河流/冻河/沼泽的生成

### 5. 中央岛屿判定使用区块坐标
`EndBiomeSource::isInCentralIsland()` 使用区块坐标（blockX >> 4），不是方块坐标。4096 = 64²（64 个区块半径）。

### 6. Parameter::span 支持两种重载
- `Parameter::span(f32 min, f32 max)` — 从浮点值创建范围
- `Parameter::span(const Parameter& first, const Parameter& second)` — 从两个参数的 min/max 创建范围

### 7. BiomeContainer 已移除
原 `BiomeContainer.hpp/cpp` 已移除，生物群系存储现由 `IChunk` 内部管理。

### 8. 已删除的文件和接口
- **BiomeEffects.cpp** — 已删除，`BiomeEffects.hpp` 现为 header-only（所有方法内联）
- **Biome::Category** — 已删除，生物群系分类改由 BiomeTags 系统替代
- **isOceanOrRiverBiome()** — 已删除，改用 `BiomeTags::IS_OCEAN` / `BiomeTags::IS_RIVER` 判断
- **Biome::temperature()** — 已移除，请使用 `biome.climate().temperature` 替代

### 9. BiomeTag 标签系统
`BiomeTag`/`BiomeTags`/`BiomeTagLoader` 提供基于标签的生物群系分类，替代了旧的 `Biome::Category` 枚举和硬编码判断函数（如 `isOceanOrRiverBiome`）：
- `BiomeTag.hpp/cpp` — 标签类型定义，每个标签对应一组生物群系 ID
- `BiomeTags.hpp/cpp` — 原版标签常量（`IS_OCEAN`、`IS_RIVER`、`IS_MOUNTAIN` 等）
- `BiomeTagLoader.hpp/cpp` — 从数据包加载标签定义的加载器

### 10. shouldFreeze/shouldSnow 与 doesWaterFreeze/doesSnowGenerate 的区别
- `shouldFreeze(world, x, y, z, seaLevel, checkNeighbors)` — 完整实现，需要 IWorld，检查温度、光照、流体类型、邻居水域
- `shouldSnow(world, x, y, z, seaLevel)` — 完整实现，需要 IWorld，检查降水类型、温度、光照、方块状态，并通过 `SnowBlock::canSurviveAt` 检查下方方块是否支撑雪层
- `doesWaterFreeze(x, y, z, seaLevel)` — 仅温度判断，不需要 IWorld，适用于生成阶段（无光照/方块状态可用时）
- `doesSnowGenerate(x, y, z, seaLevel)` — 仅温度判断，不需要 IWorld，适用于 SurfaceRules 等生成阶段

**shouldFreeze/shouldSnow 已集成调用：SnowAndFreezeFeature（TopLayerModification 生成阶段）、ServerWorld::tickPrecipitation()（运行时逐 tick 结冰和降雪）、LakeFeature（湖泊冻结，checkNeighbors=false）。**
