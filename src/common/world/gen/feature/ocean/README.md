# 海洋特征模块 (Ocean Features)

该目录实现海洋植被与珊瑚相关的世界生成特征，负责在海底与暖水海域补充海带、海草、海泡菜与珊瑚结构。

## 目录结构树

```text
ocean/
├── README.md              # 本文档
├── KelpFeature.hpp/cpp    # 海带特征（kelp + kelp_plant）
├── SeagrassFeature.hpp/cpp# 海草特征（普通海草 + 高海草）
├── SeaPickleFeature.hpp/cpp # 海泡菜特征（依赖活珊瑚基底）
└── CoralFeature.hpp/cpp   # 珊瑚特征（树形/蘑菇形/爪形）
```

## 文件介绍

- `KelpFeature.hpp/cpp`
  - 负责海带柱状生长放置。
  - 配置项包含主体状态、顶部状态与最大高度。
- `SeagrassFeature.hpp/cpp`
  - 负责普通海草与高海草混合放置。
  - 高海草通过 `HALF` 属性区分上下半。
- `SeaPickleFeature.hpp/cpp`
  - 负责海泡菜簇放置与数量控制（`PICKLES_1_4`）。
  - 仅允许在活珊瑚块上方生成。
- `CoralFeature.hpp/cpp`
  - 负责珊瑚结构生成。
  - 包含树形、蘑菇形、爪形三类结构，并带有珊瑚扇/墙珊瑚扇装饰。

## 模块关系

- 该目录依赖方块注册层：`VanillaBlocks`、`BlockRegistry`、`BlockStateProperties`。
- 该目录被 `ConfiguredFeature`/`FeatureRegistry` 注册并在 `VegetalDecoration` 阶段触发。
- 生物群系配置通过 `BiomeGenerationSettings` 引用对应 `FeatureIds`。

## 整体职责

- 将海洋生态相关地物从“空配置/占位逻辑”替换为可执行生成逻辑。
- 从区块原点输入 (`y=0`) 通过 `OceanFloor` 高度图回推真实海底放置高度。
- 保证特征只在合理环境中放置：
  - 海带/海草：水体 + 海底支撑。
  - 海泡菜：水体 + 活珊瑚基底。
  - 珊瑚：水体中生成结构并附带装饰。

## 输入 / 输出

- 输入：
  - `WorldGenRegion`（读写方块）
  - `math::Random`（随机形态）
  - `BlockPos`（起始位置）
  - 各 FeatureConfig（状态指针、概率、高度等）
- 输出：
  - 在世界生成区域内写入海洋方块状态
  - 返回布尔值指示是否成功放置至少一个方块

## 依赖项

- 内部依赖：
  - `world/gen/chunk/IChunkGenerator.hpp`
  - `world/chunk/ChunkPrimer.hpp`
  - `world/block/VanillaBlocks.hpp`
  - `util/Direction.hpp`
- 关键状态属性：
  - `PICKLES_1_4`
  - `HALF`
  - `FACING`

## 使用方法

```cpp
VanillaBlocks::initialize();
FeatureRegistry::instance().initialize();

// 以海带特征为例
auto kelp = KelpFeatures::createNormalKelp();
const KelpFeatureConfig& config = kelp->getConfig();
KelpFeature feature;
math::Random rng(12345);

feature.place(region, rng, BlockPos(8, 40, 8), config);
```

## 容易踩的坑

- 传入的起始坐标通常是区块原点，不能直接用其 `y` 做向下扫描起点。
- 海泡菜必须在活珊瑚块上方，否则会全部尝试失败。
- 高海草需要同时设置上下半状态，缺失任一状态会退化为普通海草或直接失败。
- 珊瑚墙扇使用 `FACING` 指向支撑面方向，方向传反会导致后续掉落。

## 测试用例

- `tests/common/world/gen/OceanFeatureTest.cpp`
  - `KelpFeaturePlacesKelpInWater`
  - `SeagrassMixedFeaturePlacesSeaPlant`
  - `CoralFeaturePlacesConfiguredCoralBlock`
  - `SeaPickleFeatureFailsOnNonCoralGround`
  - `SeaPickleFeaturePlacesOnLivingCoral`

## Mermaid 图表

```mermaid
flowchart TD
    A[生物群系装饰阶段 VegetalDecoration] --> B[FeatureRegistry]
    B --> C[KelpFeature]
    B --> D[SeagrassFeature]
    B --> E[SeaPickleFeature]
    B --> F[CoralFeature]

    C --> G[VanillaBlocks::KELP_PLANT / KELP]
    D --> H[VanillaBlocks::SEAGRASS / TALL_SEAGRASS]
    E --> I[VanillaBlocks::SEA_PICKLE]
    E --> J[活珊瑚基底检测]
    F --> K[珊瑚块 + 珊瑚扇 + 墙珊瑚扇]

    style A fill:#e6f4ea,stroke:#2e7d32,color:#1b5e20
    style B fill:#e3f2fd,stroke:#1565c0,color:#0d47a1
    style C fill:#fff8e1,stroke:#f9a825,color:#5d4037
    style D fill:#fff8e1,stroke:#f9a825,color:#5d4037
    style E fill:#fff8e1,stroke:#f9a825,color:#5d4037
    style F fill:#fff8e1,stroke:#f9a825,color:#5d4037
```
