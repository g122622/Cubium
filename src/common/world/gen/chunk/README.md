# 区块生成模块 (Chunk Generation)

本模块实现 Minecraft 1.16.5 风格的区块生成器，包括主世界、下界、末地三种维度的地形生成，以及调试世界生成器。

## 目录结构

```
chunk/
├── IChunkGenerator.hpp         # 区块生成器接口和 WorldGenRegion 定义
├── IChunkGenerator.cpp         # 接口实现和 BaseChunkGenerator 基类
├── NoiseChunkGenerator.hpp     # 噪声地形生成器（主世界）
├── NoiseChunkGenerator.cpp     # 主世界地形生成实现
├── NetherChunkGenerator.hpp    # 下界区块生成器
├── NetherChunkGenerator.cpp    # 下界地形生成实现
├── EndChunkGenerator.hpp       # 末地区块生成器
├── EndChunkGenerator.cpp       # 末地地形生成实现
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
   ┌───┼───────────────────┬───────────────────┐
   │   │                   │                   │
NoiseChunkGenerator  NetherChunkGenerator  EndChunkGenerator  DebugChunkGenerator
   │   │                   │                   │                   │
   └───┴───────────────────┴───────────────────┴───────────────────┘
                           │
                    WorldGenRegion (生成区域视图)
```

- **IChunkGenerator**：定义区块生成流水线接口（结构起点→结构引用→生物群系→噪声→地表→雕刻→特性→生物）
- **BaseChunkGenerator**：提供默认实现，子类只需重写核心方法
- **WorldGenRegion**：提供有限的世界视图，按 `ChunkStatus::taskRange()` 构建动态方阵区域，越界访问会触发断言

## 上下游依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `chunk/ChunkPrimer` | 区块生成中间状态存储 |
| `chunk/ChunkStatus` | 区块生成阶段状态机 |
| `biome/BiomeSource` | 生物群系源（MC 1.18+ 多噪声采样） |
| `gen/noise/` | 噪声生成器（Octaves、Perlin、Simplex） |
| `gen/carver/` | 雕刻器（洞穴、峡谷、水下雕刻） |
| `gen/structure/StructureManager` | 结构生成管理 |
| `gen/feature/FeatureRegistry` | 特性注册表 |
| `gen/surface/SurfaceSystem` | MC 1.21 地表规则系统 |
| `gen/density/NoiseRouter` | MC 1.21 密度函数管线 |
| `gen/spawn/WorldGenSpawner` | 初始生物生成 |
| `gen/settings/DimensionSettings` | 维度配置 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `server/world/ServerWorld` | 服务端世界管理，创建和调用区块生成器 |
| `world/chunk/ChunkLoadTicketManager` | 区块加载管理，触发区块生成任务 |

## 容易踩的坑

### 1. 噪声缓存大小

噪声缓存需要 +1 用于插值，大小不匹配会导致数组越界：
```cpp
// 错误
noiseCache[0].resize(m_noiseSizeZ, std::vector<f32>(m_noiseSizeY));
// 正确
noiseCache[0].resize(m_noiseSizeZ + 1, std::vector<f32>(m_noiseSizeY + 1));
```

### 2. 世界坐标 vs 本地坐标

- **WorldGenRegion** 使用世界坐标
- **ChunkPrimer** 使用本地坐标 (0-15)
- 转换：`localX = worldX & 15`, `localZ = worldZ & 15`

### 3. 区块索引计算

WorldGenRegion 的区块索引布局：
```cpp
// 索引 = (relZ + radius) * (radius * 2 + 1) + (relX + radius)
// relX, relZ 范围: [-radius, radius]
// 中心区块索引 = radius * (radius * 2 + 1) + radius
```

### 4. 生成阶段顺序

必须按 ChunkStatus 顺序调用：`STRUCTURE_STARTS → STRUCTURE_REFERENCES → BIOMES → NOISE → SURFACE → CARVERS → LIQUID_CARVERS → FEATURES → LIGHT → SPAWN → HEIGHTMAPS → FULL`

### 5. 随机种子确定性

种子计算必须与 MC 一致：
```cpp
math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                 static_cast<u64>(chunkZ) * 132897987541ULL +
                 m_seed);
```

### 6. 噪声八度缩放方向

MC 1.16.5 的倍频缩放方向与常见 FBM 相反：
```cpp
// 错误（常见 FBM）
frequency *= 2.0f;  amplitude *= 0.5f;
// 正确（MC 风格）
octaveScale *= 0.5f;  // 从 1.0 开始，每层乘 0.5
```
如果方向错误，主世界会明显变平，峰值高度长期卡在 80~90 左右。

### 7. 线程安全

`fillNoiseColumn` 的 5x5 生物群系滑窗缓存改为调用栈局部对象，不再存到生成器成员上。如果后续给生成器新增可变成员，需重新评估并发安全性。

### 8. WorldGenRegion 窗口越界

`WorldGenRegion` 按阶段特定的 `ChunkStatus::taskRange()` 构建窗口，缺失区块会在 `getTopBlockY()` 等热路径触发断言。不要将窗口外的高度查询视为"高度 0"，应修复区域半径或调用点。

### 9. 生物群系权重计算

权重因子受中心生物群系深度影响：
```cpp
const f32 depthFactor = (depth > centerDepth) ? 0.5f : 1.0f;
// 这确保深谷边缘更陡峭，山峰边缘更平缓
```

### 10. isDebugGenerator() 标识

`DebugChunkGenerator::isDebugGenerator()` 返回 `true`，用于 `ServerWorld::isDebugWorld()` 检测调试世界，此时方块放置/破坏、计划刻、天气更新、红石更新等均被禁用。

### 11. MC 1.21 密度函数管线

`NoiseChunkGenerator` 现在支持 MC 1.21 密度函数管线（`m_useDensityFunctionPipeline`），通过 `NoiseRouter` 和 `NoiseChunk` 实现 cell-based 三线性插值。新旧管线共存，由设置决定使用哪条路径。

### 12. SurfaceRules 系统

地表生成使用 MC 1.21 的 `SurfaceSystem` 和 `SurfaceRules`，替代旧版 `SurfaceBuilder` 按生物群系路由的方式。基岩生成统一通过 `_applyBedrock()` 实现，由 `DimensionSettings::bedrockFloor` 和 `bedrockRoof` 驱动。
