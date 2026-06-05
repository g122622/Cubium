# Biome 模块

本目录包含 Cubium 项目的生物群系（Biome）系统，负责生物群系定义、气候参数、生成和分布。该系统完整复刻了 Minecraft Java 1.21.11 的多噪声（MultiNoise）3D 生物群系生成算法。

## 目录结构

```
biome/
├── Biome.hpp                    # 生物群系定义类
├── Biome.cpp
├── BiomeEffects.hpp             # 生物群系视觉效果（水色、雾色、天色等）
├── BiomeGenerationSettings.hpp  # 生物群系生成设置（特征配置）
├── BiomeGenerationSettings.cpp
├── BiomeRegistry.hpp            # 生物群系注册表 + 工厂函数
├── BiomeRegistry.cpp
├── BiomeSource.hpp              # 生物群系源基类
├── BiomeSource.cpp
├── BiomeContainer.hpp           # 4x4x4 采样生物群系存储
├── BiomeContainer.cpp
├── Biomes.hpp                   # 生物群系 ID 常量 + 聚合头文件
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

## 核心概念：MC 1.18+ 多噪声系统

MC 1.18+ 使用 6 个气候参数（Climate Parameters）来确定每个位置应生成的生物群系：

| 参数 | 范围 | 说明 |
|------|------|------|
| Temperature | [-1, 1] | 温度 |
| Humidity | [-1, 1] | 湿度 |
| Continentalness | [-1.2, 1.0] | 大陆度（海洋→内陆） |
| Erosion | [-1, 1] | 侵蚀度 |
| Depth | [-1, 1] | 深度（表面/地下） |
| Weirdness | [-1, 1] | 奇异度（山谷/山峰） |

每个生物群系注册时定义一组 Climate Parameter 范围，系统通过最近邻匹配（fitness 计算）确定最匹配的生物群系。

## 文件详细说明

### Biome.hpp / Biome.cpp

定义单个生物群系的所有属性：
- 基本信息：ID、名称、类别（Category）
- 地形参数：深度（depth）、比例（scale）
- 气候参数：温度、湿度
- 方块设置：表面/次表面/水下方块
- 生成设置：特征列表
- 生物生成设置

### BiomeEffects.hpp

生物群系视觉效果配置，包括水体颜色、雾颜色、天空颜色、树叶颜色、草颜色、草颜色修改器、干燥树叶颜色等。

### BiomeSource.hpp / BiomeSource.cpp

生物群系源基类，提供：
- `getNoiseBiome(quartX, quartY, quartZ)` — 核心 3D 采样接口
- `possibleBiomes()` — 返回可能生成的生物群系列表
- `fillBiomeContainer()` — 批量填充区块生物群系
- `findBiome()` — 搜索指定生物群系
- `getBiomesWithin()` — 获取范围内所有生物群系

### climate/Climate.hpp / Climate.cpp

Climate 参数系统核心：
- `Parameter` — 量化参数范围，支持 `point()`、`span()`、`fullRange()`、`distance()`
- `TargetPoint` — 6 维气候采样结果
- `ParameterPoint` — 参数定义点，支持 `fitness()` 距离计算
- `ParameterList<T>` — 参数列表 + 最近邻搜索
- `Sampler` — 持有 6 个密度函数引用，在任意 3D 位置采样

### source/MultiNoiseBiomeSource

基于 Climate 参数的多噪声生物群系源，用于主世界和下界。持有 NoiseRouter 和 ParameterList，通过 Sampler 采样后匹配最近生物群系。

### source/OverworldBiomeBuilder

主世界生物群系参数构建器，实现 MC 1.21.11 完整的生物群系映射逻辑：

- **5 温度档** × **5 湿度档** × **7 侵蚀档** × **大陆度范围** × **13 奇异度切片**
- 每个切片（MidSlice/HighSlice/Peaks/LowSlice/Valleys）内按温度×湿度×侵蚀×大陆度组合注册生物群系
- 支持：河流/冻河、沼泽/红树林沼泽、石岸、风袭热带草原、樱桃树林、苍白花园等

### source/NetherBiomeSource

下界生物群系源，使用简化的多噪声参数（温度×湿度）映射 5 个下界生物群系。

### source/EndBiomeSource

末地生物群系源，使用专用算法：
- 中央岛屿（距原点 64 区块半径内）→ THE_END
- 外围岛屿通过 EndIslands 噪声函数判断：
  - noise > 0.25 → EndHighlands
  - noise >= -0.0625 → EndMidlands
  - noise < -0.21875 → SmallEndIslands
  - 其他 → EndBarrens

### BiomeRegistry.hpp / BiomeRegistry.cpp

生物群系注册表单例 + BiomeFactory 工厂函数。支持 186 个生物群系（含 MC 1.18+ 新增的山峰、洞穴、樱花树林、苍白花园等）。

## 模块关系

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

## 依赖关系

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

### 2. OverworldBiomeBuilder 的 pick 方法使用 Parameter 而非 f64

MC 1.21.11 中 `pickMiddleBiome` 等方法接收 `Climate.Parameter`（奇异度范围），通过 `weirdness.max >= 0` 判断正/负奇异度来选择变体生物群系。

### 3. 13 个奇异度切片

`addInlandBiomes()` 注册 13 个奇异度切片，覆盖完整的 [-1, 1] 范围：
- 4 个 MidSlice、4 个 HighSlice、2 个 Peaks、2 个 LowSlice、1 个 Valleys
- Valleys 切片负责河流/冻河/沼泽的生成

### 4. 中央岛屿判定使用区块坐标

`EndBiomeSource::isInCentralIsland()` 使用区块坐标（blockX >> 4），不是方块坐标。4096 = 64²（64 个区块半径）。

### 5. Parameter::span 支持两种重载

- `Parameter::span(f32 min, f32 max)` — 从浮点值创建范围
- `Parameter::span(const Parameter& first, const Parameter& second)` — 从两个参数的 min/max 创建范围

## 参考资料

- Minecraft Wiki - Biome: https://minecraft.wiki/w/Biome
- MC 1.21.11 源码 - `net.minecraft.world.level.biome` 包
- MC 1.21.11 源码 - `net.minecraft.world.level.biome.OverworldBiomeBuilder`
