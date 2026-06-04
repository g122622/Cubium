# climate/ — MC 1.18+ 气候参数系统

## 概述

MC 1.18+ 引入的 3D 多噪声生物群系生成系统核心。取代旧版 2D 层叠生成，
通过 6 个气候参数（temperature, humidity, continentalness, erosion, depth, weirdness）
在三维空间中采样，然后通过最近邻匹配确定生物群系。

## 目录结构

```
climate/
├── Climate.hpp    — 参数类型、采样器、参数列表
└── Climate.cpp    — 采样器实现
```

## 文件介绍

### Climate.hpp

定义了气候系统的核心类型：

- **`QUANTIZATION_FACTOR`** (10000.0f) — 浮点参数量化为整数的因子，优化比较性能
- **`Parameter`** — 气候参数范围 [min, max]，支持 `point()`、`span()`、`fullRange()` 创建方式
  - `distance(i64 value)` — 计算量化值到此范围的距离
- **`TargetPoint`** — 6 个量化气候参数值（由 Sampler 采样得到）
  - `fromFloats()` — 从浮点值创建并量化
  - `toParameterArray()` — 转换为 7 元素数组（含 offset=0）
- **`ParameterPoint`** — 6 个 Parameter + offset（定义生物群系的气候条件范围）
  - `fitness(const TargetPoint&)` — 计算与目标点的适配度（距离平方和）
- **`ParameterList<T>`** — ParameterPoint → T 映射列表，支持最近邻查找
  - `findValue(const TargetPoint&)` — 查找最匹配的值
- **`Sampler`** — 气候采样器，持有 6 个 DensityFunction 引用
  - `sample(quartX, quartY, quartZ)` — 在指定 quart 坐标处采样气候值

### Climate.cpp

- **`Sampler::sample()`** — 将 quart 坐标转换为方块坐标，调用 6 个密度函数计算，
  结果量化后封装为 `TargetPoint`

## 内部模块关系

```
Sampler ──持有──→ DensityFunction（6个引用）
  │
  └──sample()──→ TargetPoint
                    │
ParameterList<BiomeId> ──findValue()──→ BiomeId
  │                                     ↑
  └──持有──→ ParameterPoint ──fitness()──┘
```

## 外部依赖关系

### 依赖

- `common/world/gen/density/DensityFunction.hpp` — Sampler 持有 DensityFunction 引用
- `common/core/Types.hpp` — 基础类型 i32, i64, f32, f64
- `common/util/assert/AssertAll.hpp` — MC_ASSERT_RELEASE

### 被依赖

- `common/world/biome/source/MultiNoiseBiomeSource` — 使用 Sampler + ParameterList
- `common/world/biome/source/OverworldBiomeBuilder` — 构建主世界 ParameterList
- `common/world/biome/source/NetherBiomeSource` — 构建下界 ParameterList

## 容易踩的坑

1. **quart 坐标 vs 方块坐标**：quart 坐标 = 方块坐标 / 4，Sampler.sample() 接收 quart 坐标，
   内部转换为方块坐标调用 DensityFunction
2. **量化精度**：所有气候参数比较都基于量化后的整数值（×10000），
   不要直接比较浮点值
3. **offset 字段**：ParameterPoint 的 offset 用于微调优先级，TargetPoint 中 offset 固定为 0
4. **ParameterList 线性搜索**：当前实现使用线性搜索，MC 1.21 原版使用 RTree 加速，
   后续可根据性能需求替换
