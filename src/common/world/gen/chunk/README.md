# 区块生成模块 (Chunk Generation)

本模块实现 Minecraft 1.21 风格的区块生成器，所有维度（主世界、下界、末地）统一使用 NoiseChunkGenerator 通过密度函数管线生成地形。已对齐 MC 1.21.11 Java 源码。

## 目录结构

```
chunk/
├── IChunkGenerator.hpp         # 区块生成器接口和 WorldGenRegion 定义
├── IChunkGenerator.cpp         # 接口实现和 BaseChunkGenerator 基类
├── NoiseChunkGenerator.hpp     # 噪声地形生成器（主世界/下界/末地统一）
├── NoiseChunkGenerator.cpp     # 噪声地形生成实现
├── DebugChunkGenerator.hpp     # 调试模式生成器（展示所有方块状态）
├── DebugChunkGenerator.cpp     # 调试世界生成实现
├── FlatChunkGenerator.hpp      # 平坦世界生成器
├── FlatChunkGenerator.cpp      # 平坦世界生成实现
├── NoiseColumn.hpp             # 噪声列类型（getBaseColumn 返回值）
└── README.md                   # 本文档
```

## 内部模块关系

```
IChunkGenerator (接口)
       ↑
BaseChunkGenerator (基类，提供默认生物群系填充、结构生成空实现、生物生成)
       ↑
   ┌───┴───────────────────┬──────────────────────┐
NoiseChunkGenerator    DebugChunkGenerator    FlatChunkGenerator
   │                       │                       │
   └───────────────────────┴───────────────────────┘
                       │
                WorldGenRegion (生成区域视图)
```

- **IChunkGenerator**：定义区块生成流水线接口（结构起点→结构引用→生物群系→噪声→地表→雕刻→特性→生物），新增 `getBaseColumn`、`getGenDepth`、`getMinY`、`findNearestMapStructure` 虚方法
- **BaseChunkGenerator**：提供默认实现，子类只需重写核心方法
- **NoiseChunkGenerator**：统一的地形生成器，通过不同的 NoiseRouter 配置支持所有维度
- **DebugChunkGenerator**：调试模式生成器，在 Y=60 层放置屏障基座，Y=70 层展示所有方块状态网格，使用 FixedBiomeSource(Plains)
- **FlatChunkGenerator**：平坦世界生成器，逐层填充方块，使用 FlatLevelGeneratorSettings 配置
- **WorldGenRegion**：提供有限的世界视图，按当前 `ChunkStep` 的累积依赖构建动态方阵区域，并用 `directDependencies()` 校验区块访问阶段。新增 `ensureCanWrite`、`setCurrentlyGenerating`/`clearCurrentlyGenerating` 方法
- **NoiseColumn**：MC 1.21 NoiseColumn 类型，表示一条垂直列的方块状态，用于 `getBaseColumn()` 返回值

### 维度配置

| 维度 | NoiseRouter | 生物群系源 | 参数 |
|------|------------|-----------|------|
| 主世界 | `NoiseRouterData::overworld(seed)` | `MultiNoiseBiomeSource::createOverworld()` | BlendedNoise(0.25, 0.125, 80, 160, 8) |
| 下界 | `NoiseRouterData::nether(seed)` | `MultiNoiseBiomeSource::createNether()` | BlendedNoise(0.25, 0.375, 80, 60, 8) |
| 末地 | `NoiseRouterData::end(seed)` | `EndBiomeSource(seed)` | BlendedNoise(0.25, 0.25, 80, 160, 4) |
| 平坦 | N/A | `FixedBiomeSource(Plains)` | N/A |

## 关键算法对齐要点

### 1. applyCarvers 生物群系采样

```cpp
// MC 1.21.11: 直接查询噪声生物群系源，不经过 Voronoi 缩放
BiomeId biome = m_biomeSource->getNoiseBiome(originBlockX >> 2, 0, originBlockZ >> 2);
```

### 2. spawnInitialMobs 生物群系采样 Y 坐标

```cpp
// MC 1.21.11: 在最大建造高度 - 1 处采样
const i32 sampleY = region.getMaxBuildHeight() - 1;
```

### 3. generateStructureStarts 种子算法

```cpp
// MC 1.21.11: 使用 setLargeFeatureSeed 而非 setLargeFeatureWithSalt
math::JavaLegacyRandom rng;
rng.setLargeFeatureSeed(static_cast<i64>(m_seed), chunkX, chunkZ);
```

### 4. getHeight 列采样

```cpp
// MC 1.21: iterateNoiseColumn 使用 selectCellYZ 而非 selectCellXYZ
// 因为 advanceCellX(0) 已经设置了 X 方向的 slice 数据
noiseChunk->selectCellYZ(cellY, 0);
```

### 5. Cell 大小从 NoiseSettings 读取

```cpp
// MC 1.21: cellWidth = sizeHorizontal * 4, cellHeight = sizeVertical * 4
m_cellWidth = m_settings.noise.sizeHorizontal * 4;
m_cellHeight = m_settings.noise.sizeVertical * 4;
```

### 6. WorldGenRegion 初始化

```cpp
// MC 1.21.11: WorldGenRegion 必须从世界获取种子、tick、时间、难度
context.region->setSeed(m_world->seed());
context.region->setCurrentTick(m_world->currentTick());
context.region->setDayTime(m_world->dayTime());
context.region->setHardcore(m_world->isHardcore());
context.region->setDifficulty(m_world->difficulty());
```

### 7. Beardifier 结构地形适配

当前 `_buildBeardifier` 通过跨区块结构引用收集 Beardifier 数据，与 MC 1.21.11 对齐。

## 上下游依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `chunk/ChunkPrimer` | 区块生成中间状态存储 |
| `chunk/ChunkStatus` | 区块生成阶段状态机 |
| `biome/BiomeSource` | 生物群系源（MC 1.18+ 多噪声采样） |
| `gen/density/NoiseRouter` | MC 1.21 密度函数管线 |
| `gen/density/NoiseChunk` | MC 1.21 cell-based 噪声插值 |
| `gen/density/Beardifier` | 结构地形平滑密度函数 |
| `gen/carver/` | 雕刻器（洞穴、峡谷） |
| `gen/structure/StructureManager` | 结构生成管理 |
| `gen/feature/FeatureRegistry` | 特性注册表 |
| `gen/surface/SurfaceRules` | MC 1.21 地表规则系统 |
| `gen/spawn/WorldGenSpawner` | 初始生物生成 |
| `gen/settings/DimensionSettings` | 维度配置 |
| `gen/settings/FlatLevelGeneratorSettings` | 平坦世界层配置 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `server/world/ServerWorld` | 服务端世界管理，创建和调用区块生成器 |
| `world/chunk/ChunkLoadTicketManager` | 区块加载管理，触发区块生成任务 |
| `world/dimension/Dimension` | 维度创建，构建区块生成器实例 |

## 容易踩的坑

### 1. 世界坐标 vs 本地坐标

- **WorldGenRegion** 使用世界坐标
- **ChunkPrimer** 使用本地坐标 (0-15)
- 转换：`localX = worldX & 15`, `localZ = worldZ & 15`

### 2. 区块索引计算

WorldGenRegion 的区块索引布局：
```cpp
// 索引 = (relZ + radius) * (radius * 2 + 1) + (relX + radius)
// relX, relZ 范围: [-radius, radius]
// 中心区块索引 = radius * (radius * 2 + 1) + radius
```

### 3. 生成阶段顺序

必须按 ChunkStatus 顺序调用：`EMPTY → STRUCTURE_STARTS → STRUCTURE_REFERENCES → BIOMES → NOISE → SURFACE → CARVERS → FEATURES → INITIALIZE_LIGHT → LIGHT → SPAWN → FULL`

### 4. 随机种子确定性

种子计算必须与 MC 一致：
```cpp
math::JavaLegacyRandom rng;
rng.setLargeFeatureSeed(static_cast<i64>(m_seed), chunkX, chunkZ);
```

### 5. WorldGenRegion 窗口越界

`WorldGenRegion` 按当前 `ChunkStep` 的累积依赖构建窗口，缺失区块或请求阶段超过该距离允许阶段时会在 `getIChunk()` / `getTopBlockY()` 等路径触发断言。不要将窗口外查询视为"高度 0"，应修复区域半径或调用点。

### 6. isDebugGenerator() 标识

`DebugChunkGenerator::isDebugGenerator()` 返回 `true`，用于 `ServerWorld::isDebugWorld()` 检测调试世界，此时方块放置/破坏、计划刻、天气更新、红石更新等均被禁用。

### 7. 密度函数管线

`NoiseChunkGenerator` 使用 MC 1.21 密度函数管线，通过 `NoiseRouter` 和 `NoiseChunk` 实现 cell-based 三线性插值。`RandomState` 统一持有 `NoiseRouter`、`SurfaceSystem`、随机工厂等资源。`m_randomState` 必须在构造时初始化，各生成阶段通过 `MC_ASSERT_RELEASE` 保证其有效性。

### 8. SurfaceRules 系统

地表生成使用 MC 1.21 的 `SurfaceSystem` 和 `SurfaceRules`，替代旧版 `SurfaceBuilder` 按生物群系路由的方式。基岩生成由 SurfaceRules 规则驱动。所有维度均使用此系统。

### 9. buildSurface 的 noiseChunk 空检查

当 `NoiseChunk` 指针为空时（例如使用了不正确的 ChunkStatus 顺序），`buildSurface` 会输出 `spdlog::warn` 日志并安全跳过，而非静默崩溃。

### 10. FlatChunkGenerator 特性放置

`FlatChunkGenerator::placeFeatures()` 根据 `FlatLevelGeneratorSettings` 的 `hasDecoration()` 和 `hasLakes()` 标志控制特性放置：

| hasDecoration | hasLakes | 行为 |
|:---:|:---:|:---|
| false | false | 不放置任何特性，仅填充层 |
| false | true | 仅放置平坦世界专用熔岩湖（LAKE_LAVA_UNDERGROUND + LAKE_LAVA_SURFACE），不放置任何生物群系原生特性 |
| true | false | 放置生物群系装饰特性（排除 UndergroundStructures、SurfaceStructures、Lakes 阶段） |
| true | true | 放置生物群系装饰特性（排除 UndergroundStructures、SurfaceStructures 阶段）+ 专用熔岩湖（跳过生物群系原生 Lakes 阶段以避免重复） |

参考 MC 1.21.11: `FlatLevelGeneratorSettings.adjustGenerationSettings()` — `decoration=false` 时完全跳过生物群系特性复制循环，`addLakes` 的熔岩湖由 `createLakesList()` 单独提供（仅含两个熔岩湖特性，不含水湖）。

非运动阻挡层（如水层）在 `updateLayers()` 中被标记为 nullptr，`placeFeatures()` 末尾的 `_placeFillLayers()` 在特性放置后将空气方块替换为原始方块状态，确保与湖泊等特性不冲突。
