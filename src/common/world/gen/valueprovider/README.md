# 值提供器 (Value Providers)

实现 MC 1.21 的值提供器系统，用于世界生成中的可控随机整数和高度采样。

## 目录结构

```
valueprovider/
├── IntProvider.hpp         # 整数值提供器基类与具体实现（Constant/Uniform/Biased/Clamped/Normal/WeightedList）
├── IntProviderParser.hpp   # IntProvider JSON 反序列化接口
├── IntProviderParser.cpp   # IntProvider JSON 反序列化实现（支持 MC 数据包格式）
├── HeightProvider.hpp      # 高度提供器（Uniform/Biased/Trapezoid）+ WorldGenerationContext
├── FloatProvider.hpp       # 浮点数值提供器（Constant/Uniform/Trapezoid/Clamped）
└── README.md               # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────┐
│                       WorldGenerationContext                        │
│               （世界高度上下文，提供 minY/depth 信息）                 │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          HeightProvider                             │
│    ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐    │
│    │ConstantHeight│  │ UniformHeight│  │BiasedToBottomHeight  │    │
│    └──────────────┘  └──────────────┘  └──────────────────────┘    │
│    ┌──────────────────────┐  ┌────────────────────────────────┐    │
│    │VeryBiasedToBottomHeight│  │      TrapezoidHeight          │    │
│    └──────────────────────┘  └────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ (使用 VerticalAnchor)
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                           IntProvider                               │
│  ┌───────────┐  ┌────────────┐  ┌─────────────┐  ┌──────────────┐  │
│  │ConstantInt│  │ UniformInt │  │BiasedToBottom│  │  ClampedInt  │  │
│  └───────────┘  └────────────┘  └─────────────┘  └──────────────┘  │
│  ┌───────────────────┐  ┌────────────────────────────────────────┐ │
│  │ ClampedNormalInt  │  │          WeightedListInt               │ │
│  └───────────────────┘  └────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     IntProviderParser                               │
│          从 JSON 解析 IntProvider，支持 MC 数据包格式                  │
│    裸整数简写: 5 → ConstantInt(5)                                    │
│    类型分派: { "type": "minecraft:uniform", "value": {...} }        │
│    支持 constant/uniform/biased_to_bottom/clamped/                 │
│           clamped_normal/weighted_list 六种类型                      │
│    可选范围校验: parse(json, minInclusive, maxInclusive)              │
└─────────────────────────────────────────────────────────────────────┘
```

HeightProvider 依赖 VerticalAnchor（来自 `surface/SurfaceRules.hpp`）解析相对高度。

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（i32, f64 等） |
| `common/core/Result.hpp` | 错误处理（IntProviderParser 使用 Result<T>） |
| `common/util/math/random/IRandom.hpp` | 随机数接口 |
| `common/world/gen/surface/SurfaceRules.hpp` | VerticalAnchor |
| `nlohmann/json.hpp` | JSON 解析（IntProviderParser 使用） |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `feature/cave/VegetationPatchFeature` | depth 和 xzRadius 使用 IntProvider |
| `feature/BlockColumnFeature` | 层高度使用 IntProvider |
| `placement/Placement` | countProvider 使用 IntProvider |
| `feature/template/CappedStructureProcessor` | limit 使用 IntProvider（通过 IntProviderParser 从 JSON 解析） |
| `jigsaw/ProcessorListLoader` | 解析 capped 处理器的 limit 字段 |

## 容易踩的坑

### 1. IntProvider 的 min >= max 边界情况

`sample()` 方法在 `min >= max` 时直接返回 min，不会崩溃但可能产生非预期结果。构造时务必确保 `min < max`。

### 2. HeightProvider 必须通过 WorldGenerationContext 解析

`HeightProvider::sample()` 需要 `WorldGenerationContext` 参数来解析 `VerticalAnchor`。主世界使用 `WorldGenerationContext(-64, 384)`，下界使用 `WorldGenerationContext(0, 128)`。

### 3. VerticalAnchor 的三种类型

- `absolute(y)`：绝对 Y 坐标
- `aboveBottom(offset)`：从世界底部向上偏移
- `belowTop(offset)`：从世界顶部向下偏移

使用 `belowTop` 时要特别注意世界高度变化的影响。

### 4. WeightedListInt 的空列表处理

`WeightedListInt::sample()` 在空列表时返回 0，`getMinValue()/getMaxValue()` 也返回 0。构造时确保至少有一个 entry。

### 5. IntProvider JSON 解析的 value 嵌套

MC 1.21 的 IntProvider JSON 使用 `"value"` 子对象包裹参数（如 `uniform` 的 `min_inclusive`/`max_inclusive`），而 `constant` 的 `value` 字段是直接的整数。IntProviderParser 同时支持这两种格式：有 `value` 子对象时从中读取参数，否则从顶层读取。

### 6. CappedStructureProcessor 使用 POSITIVE_CODEC

MC 原版中 CappedProcessor 的 limit 字段使用 `IntProvider.POSITIVE_CODEC`（minValue >= 1）。IntProviderParser 支持范围校验参数，调用 `parse(json, 1)` 即可启用。
