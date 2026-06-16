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
├── BlendedNoise.hpp       — MC 1.18+ 混合噪声密度函数（旧式三层 Perlin）
├── BlendedNoise.cpp       — 实现
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
BlendedNoise ──持有──→ 3 × PerlinNoise (minLimit/maxLimit/main) + PerlinLayer[]
EndIslands ──持有──→ SimplexNoise
```

## 外部依赖关系

### 依赖

- `common/world/gen/noise/NormalNoise.hpp` — 噪声采样
- `common/world/gen/noise/PerlinNoise.hpp` — Perlin 噪声
- `common/world/gen/noise/SimplexNoise.hpp` — Simplex 噪声（EndIslands）
- `common/world/biome/climate/Climate.hpp` — Climate.Sampler 创建
- `common/world/WorldConstants.hpp` — MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT
- `common/util/math/MathUtils.hpp` — 数学工具

### 被依赖

- `common/world/biome/source/MultiNoiseBiomeSource` — 使用 NoiseRouter.createClimateSampler()
- `common/world/biome/source/EndBiomeSource` — 使用 Climate.Sampler.erosion 区分末地生物群系
- `common/world/gen/chunk/NoiseChunkGenerator` — 使用 finalDensity 地形生成

## 维度密度函数配置

| 维度 | finalDensity 管线 |
|------|-------------------|
| 主世界 | `slideOverworld(postProcess(depth + continents + 0.5*erosion + 0.5*ridgesPV))` |
| 下界 | `noNewCaves(slideNetherLike(blendedNoise + yClampedGradient(0,128,1.5,-1.5)))` |
| 末地 | `postProcess(slideEndLike(cache2d(endIslands) + blendedNoise_end))` |

其中 BlendedNoise 参数：
- 主世界: xzScale=0.25, yScale=0.125, xzFactor=80, yFactor=160, smear=8
- 下界: xzScale=0.25, yScale=0.375, xzFactor=80, yFactor=60, smear=8
- 末地: xzScale=0.25, yScale=0.25, xzFactor=80, yFactor=160, smear=4

## 关键算法对齐要点（MC 1.21.11）

### 1. YClampedGradient toY 值

主世界 `depthPlusOffset` 中的 `YClampedGradient` 范围为 `[MIN_BUILD_HEIGHT, MAX_BUILD_HEIGHT]`（即 [-64, 320]），toY 使用 `MAX_BUILD_HEIGHT`（320），而非 `MAX_BUILD_HEIGHT - 1`（319）。

### 2. CaveDensityFunctions::entrances() 无 min(5.0) 包装

MC 原版 `entrances()` 直接返回 `add(slopedCheese, mul(5.0, entrancesFunc))` 的结果，不使用 `min(5.0, ...)` 裁剪。C++ 曾错误地添加了 `min(5.0, ...)` 包装。

### 3. EndIslands float 精度

Java 的 `EndIslandDensityFunction.getHeightValue()` 使用 `float` 算术：
- `Mth.sqrt(float)` 返回 float，不是 double
- `Mth.abs((float)k1)` 先转 float 再取绝对值（大数精度截断）
- 阈值使用 `-0.9f`（约 -0.89999998）而非 `-0.9`（double）
- 整数除法和取模使用 `/` 和 `%`（负数向零取整），而非 `>>` 和 `&`（负数行为不同）

### 4. DensityFunctions::Invert 边界

`Invert`（1/x）的边界值在输入范围跨越零点时应使用 `±infinity`，而非 `±1e6`。

### 5. BlendAlpha / BlendOffset / BeardifierMarker

- `BlendAlpha`：始终返回 1.0（minValue=1.0, maxValue=1.0）
- `BlendOffset`：始终返回 0.0（minValue=0.0, maxValue=0.0）
- `BeardifierMarker`：Marker 类型，在 NoiseChunk::apply() 中替换为实际 Beardifier（当前暂返回 0.0）

### 6. preliminarySurfaceLevel

当前使用 `constant(0.0)` 占位。MC 原版使用 `findTopSurface` 密度函数向下搜索密度 > 0 的位置，影响含水层水位和结构放置。

### 7. BlendDensity

当前为恒等函数（直接返回输入）。MC 原版通过 Blender.blendDensity() 实现旧区块混合，与 BlendAlpha/BlendOffset 一起用于区块边界平滑过渡。

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
10. **BlendedNoise 涂抹效果**：使用 `PerlinNoise::getValueWithSmear()` 对 Y 轴应用涂抹，
    涂抹参数影响地形条纹结构
11. **EndIslands 种子**：使用 `LegacyRandomSource(seed).consumeCount(17292)` 初始化 SimplexNoise，
    确保与 Java 版生成相同的世界
