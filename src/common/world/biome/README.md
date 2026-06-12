# Biome 模块

本目录包含 Cubium 项目的生物群系（Biome）系统，负责生物群系定义、气候参数、生成和分布。该系统完整复刻了 Minecraft Java 1.21.11 的多噪声（MultiNoise）3D 生物群系生成算法。

## 目录结构

```
biome/
├── Biome.hpp                    # 生物群系定义类
├── Biome.cpp
├── BiomeAmbientSounds.hpp       # 生物群系环境音效配置
├── BiomeEffects.hpp             # 生物群系视觉效果（水色、雾色、天色等）
├── BiomeEffects.cpp
├── BiomeGenerationSettings.hpp  # 生物群系生成设置（特征配置）
├── BiomeGenerationSettings.cpp
├── BiomeRegistry.hpp            # 生物群系注册表 + 工厂函数
├── BiomeRegistry.cpp
├── BiomeSource.hpp              # 生物群系源基类
├── BiomeSource.cpp
├── Biomes.hpp                   # 生物群系 ID 常量 + isOceanOrRiverBiome() 工具函数
├── climate/                     # Climate 参数系统
│   ├── Climate.hpp              # Parameter, ParameterPoint, TargetPoint, ParameterList, Sampler
│   ├── Climate.cpp
│   └── README.md
├── source/                      # 生物群系源实现
│   ├── MultiNoiseBiomeSource.hpp  # 多噪声生物群系源（主世界、下界）
│   ├── MultiNoiseBiomeSource.cpp
│   ├── OverworldBiomeBuilder.hpp  # 主世界生物群系参数构建器
│   ├── OverworldBiomeBuilder.cpp
│   ├── NetherBiomeSource.hpp      # 下界生物群系源
│   ├── NetherBiomeSource.cpp
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
│                BiomeSource                              │
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

### 2. MC 1.18+ 多噪声系统的 6 个气候参数
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
MC 1.21.11 中 `pickMiddleBiome` 等方法接收 `Climate.Parameter`（奇异度范围），通过 `weirdness.max >= 0` 判断正/负奇异度来选择变体生物群系。

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
