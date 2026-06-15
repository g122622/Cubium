# source/ — 生物群系源实现

## 概述

引入的 3D 多噪声生物群系源实现。
IBiomeSource 是新的接口基类，替代旧版 BiomeProvider。
文件名保持 BiomeSource.hpp/cpp，类名为 IBiomeSource（通过 `using BiomeSource = IBiomeSource;` 兼容别名）。

## 目录结构

```
source/
├── MultiNoiseBiomeSource.hpp  — 多噪声生物群系源（主世界、下界）
├── MultiNoiseBiomeSource.cpp  — 实现
├── OverworldBiomeBuilder.hpp  — 主世界生物群系参数映射
├── OverworldBiomeBuilder.cpp  — 实现
├── NetherBiomeBuilder.hpp     — 下界生物群系参数构建器
├── NetherBiomeBuilder.cpp     — 实现
├── EndBiomeSource.hpp         — 末地生物群系源
├── EndBiomeSource.cpp         — 实现
└── README.md                  — 本文件
```

## 内部模块关系

```
MultiNoiseBiomeSource
  ├── Climate.ParameterList<BiomeId>  ← OverworldBiomeBuilder/NetherBiomeBuilder 构建
  ├── Climate.Sampler                  ← NoiseRouter.createClimateSampler()
  └── IBiomeSource (继承)
      ├── getNoiseBiome() → Sampler.sample() → ParameterList.findValue()
      ├── fillBiomeContainer() [final] → 遍历 24 section × 4×4×4 采样
      └── findBiome() → 螺旋搜索

EndBiomeSource
  └── IBiomeSource (继承)
      └── getNoiseBiome() → 中央岛屿判断 + erosion 阈值
```

## 外部依赖关系

### 依赖

- `common/world/biome/climate/Climate.hpp` — 参数类型和采样器
- `common/world/biome/BiomeSource.hpp` — 基类接口（IBiomeSource）
- `common/world/biome/Biomes.hpp` — 生物群系 ID 常量
- `common/world/gen/density/NoiseRouter.hpp` — 气候采样器创建
- `common/world/chunk/IChunk.hpp` — BiomeContainer

### 被依赖

- `common/world/gen/chunk/NoiseChunkGenerator` — 使用 IBiomeSource 替代 BiomeProvider
- `common/world/dimension/Dimension` — 创建各维度的 IBiomeSource

## 容易踩的坑

1. **fillBiomeContainer 的 section 索引**：section 0 对应 MIN_BUILD_HEIGHT=-64，
   quartY 偏移量需要加 `MIN_BUILD_HEIGHT >> 2`
2. **OverworldBiomeBuilder 生命周期**：`createOverworld()` 中 NoiseRouter 和 Sampler
   的生命周期需要一致管理，当前简化实现有待完善
3. **EndBiomeSource 中央岛屿判断**：使用区块坐标（blockX/16）而非方块坐标，
   范围是 chunkX² + chunkZ² ≤ 4096
4. **下界参数**：continentalness、erosion、depth、weirdness 均为 0.0（point），
   不要设为 fullRange，否则最近邻匹配不准确
5. **fillBiomeContainer 已提升为基类 final 方法**：IBiomeSource 基类中
   `fillBiomeContainer()` 标记为 `final`，子类不可覆写，统一由基类实现
   遍历 4×4×4 采样逻辑
6. **类名 vs 文件名**：基类名为 `IBiomeSource`，但文件名仍为
   `BiomeSource.hpp/cpp`。代码中可通过 `using BiomeSource = IBiomeSource;`
   兼容别名使用旧名称
