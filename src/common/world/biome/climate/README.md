# climate/ — MC 1.18+ 气候参数系统

## 概述

MC 1.18+ 引入的 3D 多噪声生物群系生成系统核心。取代旧版 2D 层叠生成，
通过 6 个气候参数（temperature, humidity, continentalness, erosion, depth, weirdness）
在三维空间中采样，然后通过最近邻匹配确定生物群系。

## 目录结构

```
climate/
├── Climate.hpp    — 参数类型、采样器、参数列表定义
└── Climate.cpp    — 采样器实现
```

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
