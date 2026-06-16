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
└── README.md                   # 本文档
```

## 内部模块关系

```
IChunkGenerator (接口)
       ↑
BaseChunkGenerator (基类，提供默认生物群系填充、结构生成空实现、生物生成)
       ↑
   ┌───┴───────────────────┐
NoiseChunkGenerator    DebugChunkGenerator
   │                       │
   └───────────────────────┘
               │
        WorldGenRegion (生成区域视图)
```

- **IChunkGenerator**：定义区块生成流水线接口（结构起点→结构引用→生物群系→噪声→地表→雕刻→特性→生物）
- **BaseChunkGenerator**：提供默认实现，子类只需重写核心方法
- **NoiseChunkGenerator**：统一的地形生成器，通过不同的 NoiseRouter 配置支持所有维度
- **DebugChunkGenerator**：调试模式生成器，在 Y=60 层放置屏障基座，Y=70 层展示所有方块状态网格
- **WorldGenRegion**：提供有限的世界视图，按当前 `ChunkStep` 的累积依赖构建动态方阵区域，并用 `directDependencies()` 校验区块访问阶段

### 维度配置

| 维度 | NoiseRouter | 生物群系源 | 参数 |
|------|------------|-----------|------|
| 主世界 | `NoiseRouterData::overworld(seed)` | `MultiNoiseBiomeSource::createOverworld()` | BlendedNoise(0.25, 0.125, 80, 160, 8) |
| 下界 | `NoiseRouterData::nether(seed)` | `MultiNoiseBiomeSource::createNether()` | BlendedNoise(0.25, 0.375, 80, 60, 8) |
| 末地 | `NoiseRouterData::end(seed)` | `EndBiomeSource(seed)` | BlendedNoise(0.25, 0.25, 80, 160, 4) |

## 关键算法对齐要点

### 1. applyCarvers 生物群系采样 Y 坐标

```cpp
// MC 1.21.11: 在 Y=0（quart 0）处采样生物群系，而非 Y=64
BiomeId biome = chunk.getBiomeAtBlock(8, 0, 8);
```

### 2. spawnInitialMobs 生物群系采样 Y 坐标

```cpp
// MC 1.21.11: 在最大建造高度处采样，而非 Y=64
BiomeId biome = chunk.getBiomeAtBlock(8, region.getMaxBuildHeight(), 8);
```

### 3. DebugChunkGenerator ChunkStatus 设置

`generateNoise()` 和 `buildSurface()` 必须在方法末尾设置对应的 ChunkStatus（`NOISE` 和 `SURFACE`），否则会破坏生成管线状态机。

### 4. Beardifier 结构地形适配

当前 `_buildBeardifier` 仅处理当前区块自身的 structureStarts。MC 原版通过 `StructureManager.startsForStructure()` 遍历所有引用区块的 StructureStart，包括跨区块结构的贡献。此功能待 StructureManager 完善后实现。

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
math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                 static_cast<u64>(chunkZ) * 132897987541ULL +
                 m_seed);
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
