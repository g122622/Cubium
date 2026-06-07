# 值提供器 (Value Providers)

实现 MC 1.21 的值提供器系统，用于世界生成中的可控随机整数和高度采样。

## 目录结构

```
valueprovider/
├── IntProvider.hpp      # 整数值提供器（Uniform/Biased/Clamped/Normal/WeightedList）
├── HeightProvider.hpp   # 高度提供器（Uniform/Biased/Trapezoid）+ WorldGenerationContext
└── README.md            # 本文档
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
```

HeightProvider 依赖 VerticalAnchor（来自 `surface/SurfaceRules.hpp`）解析相对高度。

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型（i32, f64 等） |
| `common/util/math/random/IRandom.hpp` | 随机数接口 |
| `common/world/gen/surface/SurfaceRules.hpp` | VerticalAnchor |

### 下游依赖（依赖本模块）

目前暂无下游依赖。未来可用于：
- 特征放置器（Placement）的高度范围
- 密度函数的噪声参数
- 矿石生成的大小/数量随机化

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
