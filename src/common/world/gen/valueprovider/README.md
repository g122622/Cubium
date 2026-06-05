# 值提供器 (Value Providers)

实现 MC 1.21 的值提供器系统，用于世界生成中的可控随机整数和高度采样。

## 目录结构

```
valueprovider/
├── IntProvider.hpp      # 整数值提供器（IntProvider 系列）
├── HeightProvider.hpp   # 高度提供器（HeightProvider 系列）
└── README.md            # 本文档
```

## IntProvider 系列

参考 MC 1.21.11: `net.minecraft.util.valueproviders`

| 类名 | 说明 | 关键参数 |
|------|------|----------|
| `IntProvider` | 抽象基类 | `sample(rng)`, `getMinValue()`, `getMaxValue()` |
| `ConstantInt` | 固定值 | `value` |
| `UniformInt` | 均匀分布 | `minInclusive`, `maxInclusive` |
| `BiasedToBottomInt` | 偏向底部 | `minInclusive`, `maxInclusive` |
| `ClampedInt` | 钳位 | `source`, `minInclusive`, `maxInclusive` |
| `ClampedNormalInt` | 正态分布钳位 | `mean`, `deviation`, `minInclusive`, `maxInclusive` |
| `WeightedListInt` | 加权列表 | `entries[]` (provider + weight) |

## HeightProvider 系列

参考 MC 1.21.11: `net.minecraft.world.level.levelgen.heightproviders`

| 类名 | 说明 | 关键参数 |
|------|------|----------|
| `HeightProvider` | 抽象基类 | `sample(rng, context)` |
| `ConstantHeight` | 固定高度 | `value` (VerticalAnchor) |
| `UniformHeight` | 均匀分布 | `minInclusive`, `maxInclusive` (VerticalAnchor) |
| `BiasedToBottomHeight` | 偏向底部 | `minInclusive`, `maxInclusive`, `inner` |
| `VeryBiasedToBottomHeight` | 强烈偏向底部 | `minInclusive`, `maxInclusive`, `inner` |
| `TrapezoidHeight` | 梯形/三角形分布 | `minInclusive`, `maxInclusive`, `plateau` |

## WorldGenerationContext

提供高度解析所需的世界信息：
- `getMinGenY()`: 最低生成 Y 坐标（主世界 -64，下界 0）
- `getGenDepth()`: 生成深度（主世界 384，下界 128）

## 使用方法

```cpp
using namespace mc::world::gen::valueprovider;

// IntProvider 示例
auto uniform = UniformInt::create(0, 16);
i32 value = uniform->sample(rng);  // [0, 16] 均匀随机

auto biased = BiasedToBottomInt::create(5, 30);
i32 biasedValue = biased->sample(rng);  // 偏向 5

auto normal = ClampedNormalInt::create(64.0, 10.0, 0, 128);
i32 normalValue = normal->sample(rng);  // 正态分布，钳位到 [0, 128]

// HeightProvider 示例
WorldGenerationContext ctx(-64, 384);  // 主世界

auto height = UniformHeight::create(
    VerticalAnchor::absolute(0),
    VerticalAnchor::absolute(128)
);
i32 y = height->sample(rng, ctx);  // [0, 128] 均匀随机

auto seaLevel = ConstantHeight::create(VerticalAnchor::absolute(63));
i32 seaY = seaLevel->sample(rng, ctx);  // 始终 63
```

## 依赖

- `common/core/Types.hpp` — 基础类型
- `common/util/math/random/IRandom.hpp` — 随机数接口
- `common/world/gen/surface/SurfaceRules.hpp` — VerticalAnchor
