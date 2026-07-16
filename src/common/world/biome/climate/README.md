# climate/ — 气候参数系统

## 概述

3D 多噪声生物群系生成系统核心。通过 6 个气候参数（temperature, humidity, continentalness, erosion, depth, weirdness）
在三维空间中采样，然后通过最近邻匹配确定生物群系。

## 目录结构

```
climate/
├── ParameterTypes.hpp  — 参数类型：Parameter/TargetPoint/ParameterPoint + 常量 + 辅助函数（quantizeCoord/parameters/pointParameters）
├── RTree.hpp           — RTree 空间索引模板族（7 维参数空间加速最近邻搜索）
├── ParameterList.hpp   — ParameterList<T>：参数列表 + 最近邻查找（RTree 加速）
├── Sampler.hpp         — 气候采样器声明（持有 6 个密度函数引用）
├── Sampler.cpp         — 采样器实现（sample + findSpawnPosition）
├── SpawnFinder.hpp     — 出生点查找器声明（气候空间径向搜索）
├── SpawnFinder.cpp     — 出生点查找器实现
└── README.md           — 本文件
```

## 内部模块关系

```
Sampler ──持有──→ DensityFunction（6个引用）
  │
  └──sample()──→ TargetPoint
                    │
ParameterList<BiomeId> ──findValue()──→ BiomeId
  │                                     ↑
  ├─持有──→ RTree<BiomeId> ──search()──┘
  │           │
  │           └─分支限界最近邻搜索（带缓存）
  │
  └─持有──→ ParameterPoint ──fitness()──┘  （暴力搜索备用）

Sampler::findSpawnPosition() ──→ SpawnFinder（径向搜索出生点）
```

头文件依赖：`RTree.hpp` → `ParameterTypes.hpp`；`ParameterList.hpp` → 两者；
`Sampler.hpp` ↔ `SpawnFinder.hpp` 互相前向声明（避免循环，实现在各自 .cpp）。

## 外部依赖关系

### 依赖

- `common/world/gen/density/DensityFunction.hpp` — Sampler 持有 DensityFunction 引用
- `common/util/math/MathUtils.hpp` — SpawnFinder 径向搜索用 TWO_PI
- `common/core/Types.hpp` — 基础类型 i32/i64/f32/f64
- `common/util/assert/AssertAll.hpp` — MC_ASSERT_RELEASE

### 被依赖

- `common/world/biome/source/MultiNoiseBiomeSource` — 使用 Sampler + ParameterList + TargetPoint
- `common/world/biome/source/OverworldBiomeBuilder` — 构建主世界 ParameterList（用 Parameter/ParameterPoint/ParameterList）
- `common/world/biome/source/NetherBiomeBuilder` — 构建下界 ParameterList
- `common/world/gen/density/NoiseRouter` — `createClimateSampler()` 构造 Sampler
- `common/world/gen/RandomState` / `NoiseChunk` — 持有 Sampler、设置 spawnTarget
- `common/world/gen/settings/DimensionSettings` — spawnTarget 字段（vector<ParameterPoint>）
- `server/world/ServerWorld` — `sampler.findSpawnPosition()` 查找出生区块

## 容易踩的坑

1. **quart 坐标 vs 方块坐标**：quart 坐标 = 方块坐标 / 4，`Sampler::sample()` 接收 quart 坐标，
   内部 `<< 2` 转方块坐标调用 DensityFunction。
2. **量化精度**：所有气候参数比较都基于量化后的整数值（×10000），不要直接比较浮点值。
3. **offset 字段**：ParameterPoint 的 offset 用于微调优先级，TargetPoint 中 offset 固定为 0。
4. **ParameterList 用 RTree 加速搜索**：构造时自动构建 RTree 索引，`add()` 会重建索引；
   `findValue()` 用 RTree 分支限界搜索，`findValueBruteForce()` 线性扫描（仅测试验证用）。
5. **Sampler 无默认构造**：6 个 DensityFunction 引用必填，不存在「空采样器」概念；
   `findSpawnPosition()` 在 spawnTarget 为空时直接返回 (0,0,0)。
6. **Sampler ↔ SpawnFinder 循环依赖**：两个头文件互相前向声明，实现放在各自 .cpp；
   `Sampler.cpp` include `SpawnFinder.hpp`，`SpawnFinder.cpp` include `Sampler.hpp`，头文件不互相 include。
