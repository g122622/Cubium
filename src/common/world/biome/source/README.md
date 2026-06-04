# source/ — MC 1.18+ 生物群系源实现

## 概述

MC 1.18+ 引入的 3D 多噪声生物群系源实现。
BiomeSource 是新的接口，替代旧版 BiomeProvider。

## 目录结构

```
source/
├── MultiNoiseBiomeSource.hpp  — 多噪声生物群系源（主世界、下界）
├── MultiNoiseBiomeSource.cpp  — 实现
├── OverworldBiomeBuilder.hpp  — 主世界生物群系参数映射
├── OverworldBiomeBuilder.cpp  — 实现
├── NetherBiomeSource.hpp      — 下界生物群系参数构建器
├── NetherBiomeSource.cpp      — 实现
├── EndBiomeSource.hpp         — 末地生物群系源
├── EndBiomeSource.cpp         — 实现
└── README.md                  — 本文件
```

## 文件介绍

### MultiNoiseBiomeSource

基于 Climate 参数的多噪声生物群系源，支持主世界和下界：

1. 接收 quart 坐标 (x, y, z)
2. 使用 Climate.Sampler 采样 6 个气候参数
3. 在 ParameterList 中查找最近邻生物群系

工厂方法：
- `createOverworld(seed, largeBiomes)` — 创建主世界生物群系源
- `createNether(seed)` — 创建下界生物群系源

### OverworldBiomeBuilder

主世界生物群系到气候参数的映射：

- **温度** 5 档：冰冻/冷/温和/暖/热
- **湿度** 5 档：干旱/干燥/中性/湿润/潮湿
- **大陆度** 7 档：蘑菇岛/深海/海洋/海岸/近内陆/中内陆/远内陆
- **侵蚀** 7 档
- **深度**：表面/地下
- **奇异度**：山谷/低坡/中坡/高坡/山峰

生物群系选择逻辑：
- 海洋区域：`OCEANS[温度][深浅]`
- 海岸区域：`pickBeachBiome(温度)`
- 近内陆/中内陆/远内陆：根据奇异度选择中部/高原/山坡/山峰
- 地下：滴水石洞、繁茂洞穴、深暗之域

### NetherBiomeSource

下界使用简化的气候参数映射，仅 temperature 和 humidity 有效：

| 生物群系 | Temperature | Humidity | Offset |
|----------|-------------|----------|--------|
| 下界荒地 | 0.0 | 0.0 | 0.0 |
| 灵魂沙峡谷 | 0.0 | -0.5 | 0.0 |
| 绯红森林 | 0.4 | 0.0 | 0.0 |
| 诡异森林 | 0.0 | 0.5 | 0.375 |
| 玄武岩三角洲 | -0.5 | 0.0 | 0.175 |

### EndBiomeSource

末地使用专用算法（非 MultiNoise）：

| 条件 | 生物群系 |
|------|---------|
| x²+z² ≤ 4096（距原点64格内） | TheEnd |
| erosion > 0.25 | EndHighlands |
| erosion ≥ -0.0625 | EndMidlands |
| erosion < -0.21875 | SmallEndIslands |
| -0.21875 ≤ erosion < -0.0625 | EndBarrens |

## 内部模块关系

```
MultiNoiseBiomeSource
  ├── Climate.ParameterList<BiomeId>  ← OverworldBiomeBuilder/NetherBiomeSource 构建
  ├── Climate.Sampler                  ← NoiseRouter.createClimateSampler()
  └── BiomeSource (继承)
      ├── getNoiseBiome() → Sampler.sample() → ParameterList.findValue()
      ├── fillBiomeContainer() → 遍历 24 section × 4×4×4 采样
      └── findBiome() → 螺旋搜索

EndBiomeSource
  └── BiomeSource (继承)
      └── getNoiseBiome() → 中央岛屿判断 + erosion 阈值
```

## 外部依赖关系

### 依赖

- `common/world/biome/climate/Climate.hpp` — 参数类型和采样器
- `common/world/biome/BiomeSource.hpp` — 基类接口
- `common/world/biome/Biomes.hpp` — 生物群系 ID 常量
- `common/world/gen/density/NoiseRouter.hpp` — 气候采样器创建
- `common/world/chunk/IChunk.hpp` — BiomeContainer

### 被依赖

- `common/world/gen/chunk/NoiseChunkGenerator` — 使用 BiomeSource 替代 BiomeProvider
- `common/world/dimension/Dimension` — 创建各维度的 BiomeSource

## 容易踩的坑

1. **fillBiomeContainer 的 section 索引**：section 0 对应 MIN_BUILD_HEIGHT=-64，
   quartY 偏移量需要加 `MIN_BUILD_HEIGHT >> 2`
2. **OverworldBiomeBuilder 生命周期**：`createOverworld()` 中 NoiseRouter 和 Sampler
   的生命周期需要一致管理，当前简化实现有待完善
3. **EndBiomeSource 中央岛屿判断**：使用区块坐标（blockX/16）而非方块坐标，
   范围是 chunkX² + chunkZ² ≤ 4096
4. **下界参数**：continentalness、erosion、depth、weirdness 均为 0.0（point），
   不要设为 fullRange，否则最近邻匹配不准确
