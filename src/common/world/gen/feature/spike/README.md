# 末地黑曜石柱特征模块

## 1. 目录结构树

```text
spike/
├── EndSpikeFeature.hpp
├── EndSpikeFeature.cpp
└── README.md
```

## 2. 文件介绍

| 文件 | 职责 |
|---|---|
| EndSpikeFeature.hpp/.cpp | 定义并实现末地主岛黑曜石柱（End Spike）特征、配置对象、配置化包装与特征集合导出 |

## 3. 模块关系

- `EndSpikeFeature` 负责具体结构放置算法。
- `ConfiguredEndSpikeFeature` 将算法封装为统一 `ConfiguredFeatureBase` 接口。
- `EndSpikeFeatures` 提供静态特征集合，供 `FeatureRegistry` 在 `SurfaceStructures` 阶段注册。
- `BiomeGenerationSettings::createTheEnd()` 通过 `FeatureIds` 引用该阶段槽位。

## 4. 整体职责

- 生成末地主岛核心视觉结构：围绕中心分布的黑曜石柱。
- 提供可复用、可注册的特征对象，接入统一世界生成管线。
- 在生成阶段按 WorldGenRegion 相交关系裁剪无关柱体，减少跨区块无效遍历。
- 为后续末影龙战斗场景（如柱子重建/摧毁）保留配置扩展点。

## 5. 输入/输出

- 输入：`WorldGenRegion`、`Random`、起始坐标、`EndSpikeFeatureConfig`。
- 输出：
  - 世界中写入黑曜石柱（及受保护柱顶部笼体占位方块）。
  - 返回 `bool` 指示本次放置流程是否执行成功。

## 6. 依赖项

- 内部依赖：`ConfiguredFeature`、`DecorationStage`、`FeatureIds`。
- 外部依赖：`VanillaBlocks`、`ChunkPrimer`、`IChunkGenerator`、`math::Random`。

## 7. 使用方法

```cpp
#include "common/world/gen/feature/spike/EndSpikeFeature.hpp"

// 一般由 FeatureRegistry 统一注册，不建议业务层手动调用
mc::EndSpikeFeatures::initialize();
auto features = mc::EndSpikeFeatures::getAllFeaturesAndClear();

// 这些特征会被注册到 SurfaceStructures 阶段
```

## 8. 容易踩的坑

- `getAllFeaturesAndClear()` 是所有权转移语义，调用后静态缓存会被清空。
- 柱子布局需要使用当前生成器种子计算，固定种子会导致不同世界共享同一柱环排列。
- 黑曜石柱在实现中使用绝对中心坐标分布，若调用方坐标策略不一致，容易出现“柱子偏离主岛中心”的错位。
- 柱顶笼体当前使用基岩占位，不是最终铁栏杆表现，需要后续材质对齐。

## 9. 测试用例

- tests/common/world/gen/test_vegetation_features.cpp：
  - `EndSurfaceFeatureIdsAreOffsetAfterIceSpikes`
  - `IceSpikeFeatureNames`（包含 `end_spike` 名称槽位验证）
  - `TheEndBiomeSettings`（主岛配置已引用黑曜石柱）

## 10. Mermaid 图表

```mermaid
flowchart LR
    A[EndSpikeFeature] --> B[ConfiguredEndSpikeFeature]
    B --> C[EndSpikeFeatures]
    C --> D[FeatureRegistry SurfaceStructures]
    D --> E[BiomeGenerationSettings TheEnd]
    E --> F[ChunkGenerator placeFeatures]

    style A fill:#fce5cd,stroke:#b45f06,stroke-width:2px
    style D fill:#d9ead3,stroke:#38761d,stroke-width:2px
    style F fill:#cfe2f3,stroke:#0b5394,stroke-width:2px
```
