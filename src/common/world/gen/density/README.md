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
└── README.md              — 本文件
```

## 文件介绍

### DensityFunction.hpp

密度函数核心接口：

```cpp
class DensityFunction {
    virtual f64 compute(i32 blockX, i32 blockY, i32 blockZ) const = 0;
    virtual f64 minValue() const = 0;
    virtual f64 maxValue() const = 0;
};
```

### DensityFunctions.hpp

所有密度函数实现：

| 类名 | 用途 | compute 公式 |
|------|------|-------------|
| `Constant` | 常量值 | `value` |
| `YClampedGradient` | Y轴梯度 | `clampedMap(y, fromY, toY, fromVal, toVal)` |
| `Clamp` | 钳制 | `clamp(input, min, max)` |
| `Mapped(Abs)` | 绝对值 | `abs(input)` |
| `Mapped(Square)` | 平方 | `input * input` |
| `Mapped(Cube)` | 立方 | `input^3` |
| `Mapped(HalfNegative)` | 负值减半 | `input > 0 ? input : input * 0.5` |
| `Mapped(QuarterNegative)` | 负值四分之一 | `input > 0 ? input : input * 0.25` |
| `Mapped(Squeeze)` | 压缩 | `clamp(x,-1,1)/2 - clamp(x,-1,1)^3/24` |
| `TwoArgument(Add)` | 加法 | `arg1 + arg2` |
| `TwoArgument(Mul)` | 乘法 | `arg1 * arg2` |
| `TwoArgument(Min)` | 最小值 | `min(arg1, arg2)` |
| `TwoArgument(Max)` | 最大值 | `max(arg1, arg2)` |
| `NoiseDensity` | 基础噪声 | `noise(x*xzScale, y*yScale, z*xzScale)` |
| `ShiftedNoise` | 带偏移噪声 | `noise(x*xzScale+shiftX, y*yScale+shiftY, z*xzScale+shiftZ)` |
| `ShiftNoise(ShiftA)` | XZ偏移 | `noise(x*0.25, 0, z*0.25) * 4` |
| `ShiftNoise(ShiftB)` | ZX偏移 | `noise(z*0.25, x*0.25, 0) * 4` |
| `ShiftNoise(Shift)` | XYZ偏移 | `noise(x*0.25, y*0.25, z*0.25) * 4` |
| `RangeChoice` | 条件选择 | `input ∈ [min,max) ? whenInRange : whenOutOfRange` |
| `Spline` | 样条插值 | 三次 Hermite 插值 |
| `Cache2D` | 2D缓存 | XZ不变时复用 |
| `FlatCache` | 区块级缓存 | quart 坐标缓存 |
| `CacheAllInCell` | 全缓存 | 区块内所有位置缓存 |
| `WeirdScaledSampler` | 奇异缩放 | 根据rarity映射缩放噪声 |
| `EndIslands` | 末地岛屿 | 末地岛屿噪声 |

### NoiseRouter.hpp

噪声路由器，持有 15 个密度函数：

- **气候参数**（6个）：temperature, vegetation, continents, erosion, depth, ridges
- **地形**（2个）：finalDensity, preliminarySurfaceLevel
- **洞穴**（4个）：barrierNoise, fluidLevelFloodednessNoise, fluidLevelSpreadNoise, lavaNoise
- **矿脉**（3个）：veinToggle, veinRidged, veinGap

### NoiseRouterData.hpp

预定义噪声配置：

| 维度 | 特点 |
|------|------|
| 主世界 | 6 个气候参数 + 完整噪声路由 |
| 大型生物群系 | 主世界参数偏移 -2 个倍频 |
| 下界 | 仅 temperature + vegetation，大陆度/侵蚀为零 |
| 末地 | 仅 endIslands，气候参数全为零 |

## 内部模块关系

```
NoiseRouter ──持有──→ 15 个 DensityFunction
     │
     └──createClimateSampler()──→ Climate.Sampler
                                     │
Climate.Sampler ──持有──→ 6 个 DensityFunction 引用
                           (temperature, vegetation, continents, erosion, depth, ridges)

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
5. **peaksAndValleys**：公式 `-abs(abs(ridges) - 2/3) - 1/3) * 3` 需要精确实现
6. **缓存线程安全**：Cache2D/FlatCache/CacheAllInCell 使用 mutable 缓存，线程不安全，
   每个区块生成任务应有独立实例
7. **NoiseRouterData 当前简化**：finalDensity 和洞穴/矿脉密度函数暂时使用常量零，
   后续需要完善 spline 计算和完整地形密度
