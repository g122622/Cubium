# density/ — MC 1.18+ 密度函数系统

## 概述

MC 1.18+ 引入的密度函数系统，用于地形生成和气候参数计算。
密度函数是可组合的表达式树，每个节点接收方块坐标 (x, y, z) 返回一个密度值。

NoiseRouter 持有 15 个密度函数引用，其中 6 个用于 Climate.Sampler。

## 目录结构

```
density/
├── DensityFunction.hpp    — 密度函数核心接口
├── DensityFunctions.hpp   — 所有密度函数实现类 + 工厂函数
├── DensityFunctions.cpp   — 实现
├── NoiseRouter.hpp        — 噪声路由器（持有 15 个密度函数）
├── NoiseRouter.cpp        — 实现
├── NoiseRouterData.hpp    — 预定义噪声配置（主世界/下界/末地）
├── NoiseRouterData.cpp    — 实现
├── NoiseChunk.hpp         — 区块噪声采样单元（三线性插值 + 缓存管理）
├── NoiseChunk.cpp         — 实现
└── README.md              — 本文件
```

## 内部模块关系

```
NoiseRouter ──持有──→ 15 个 DensityFunction
     │
     └──createClimateSampler()──→ Climate.Sampler
                                     │
Climate.Sampler ──持有──→ 6 个 DensityFunction 引用
                           (temperature, vegetation, continents, erosion, depth, ridges)

NoiseChunk ──包装──→ DensityFunction (Interpolated/CacheAllInCell/CacheOnce)
     │
     └──持有──→ NoiseInterpolator[] (三线性插值器)
     └──持有──→ CellCache[] (cell 内缓存)
     └──持有──→ Aquifer (含水层采样器)

DensityFunction 实现链:
NoiseDensity ──持有──→ NormalNoise ──持有──→ 2 × PerlinNoise ──持有──→ PerlinLayer[]
ShiftedNoise ──持有──→ NormalNoise + 3 × DensityFunction (shiftX/Y/Z)
```

## 外部依赖关系

### 依赖

- `common/world/gen/noise/NormalNoise.hpp` — 噪声采样
- `common/world/gen/noise/PerlinNoise.hpp` — Perlin 噪声
- `common/world/biome/climate/Climate.hpp` — Climate.Sampler 创建
- `common/core/Constants.hpp` — MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT
- `common/util/math/MathUtils.hpp` — 数学工具

### 被依赖

- `common/world/biome/source/MultiNoiseBiomeSource` — 使用 NoiseRouter.createClimateSampler()
- `common/world/gen/chunk/NoiseChunkGenerator` — 使用 finalDensity 地形生成

## 容易踩的坑

1. **NormalNoise 种子**：两个 PerlinNoise 实例必须使用不同种子（第二个偏移 0xDEADBEEF）
2. **INPUT_FACTOR = 1.018...**：第二个 Perlin 噪声的坐标乘以此因子，不能遗漏
3. **YClampedGradient 范围**：主世界使用 [-64, 320]，下界/末地使用 [0, 128]
4. **ShiftA vs ShiftB**：ShiftA 使用 (x, 0, z)，ShiftB 使用 (z, x, 0)，坐标顺序不同
5. **peaksAndValleys 公式**：`mul(-3, add(abs(add(abs(ridges), -2/3)), -1/3))` 需要精确实现
6. **缓存线程安全**：Cache2D/FlatCache/CacheAllInCell 使用 mutable 缓存，线程不安全，
   每个区块生成任务应有独立实例
7. **NoiseChunk 插值顺序**：三线性插值必须按 Y → X → Z 顺序调用 updateForY/updateForX/updateForZ，
   否则结果错误
8. **NoiseChunk slice 交换**：advanceCellX 后必须调用 swapSlices() 切换缓冲区
9. **NoiseInterpolator 双缓冲**：slice0/slice1 分别存储当前列和下一列的角点数据，
   初始化时需要先填充 slice0
