# 末地黑曜石柱特征模块

## 目录结构树

```text
spike/
├── EndSpikeFeature.hpp      # 黑曜石柱特征定义（配置、特征类、注册接口）
├── EndSpikeFeature.cpp      # 黑曜石柱特征实现
└── README.md
```

## 内部模块关系

- `EndSpike` - 黑曜石柱状态数据结构，存储单根柱子的位置、尺寸、是否受保护
- `EndSpikeFeatureConfig` - 特征配置，包含柱子列表和销毁模式标志
- `EndSpikeFeature` - 核心放置算法，负责生成黑曜石柱和铁栏杆笼子
- `ConfiguredEndSpikeFeature` - 配置化包装，实现 `ConfiguredFeatureBase` 接口
- `EndSpikeFeatures` - 静态特征集合工厂，提供 `createStandard()` 和 `getAllFeaturesAndClear()` 接口

## 上下游外部依赖关系

**被依赖（上游）**：
- `FeatureRegistry` → 注册 `EndSpikeFeatures` 到 `SurfaceStructures` 阶段
- `BiomeGenerationSettings::createTheEnd()` → 通过 `FeatureIds` 引用该特征

**依赖（下游）**：
- `ConfiguredFeature`、`Feature.hpp` → 特征框架接口
- `VanillaBlocks` → 黑曜石、铁栏杆、基岩等方块
- `WorldGenRegion`、`ChunkPrimer` → 世界生成区域访问
- `math::Random` → 随机数生成
- `BlockStateProperties` → 方块状态属性（铁栏杆连接方向）

## 容易踩的坑

- `getAllFeaturesAndClear()` 是所有权转移语义，调用后静态缓存会被清空，不要重复调用。
- 柱子布局使用 `generator.seed()` 计算，而非固定种子。若调用方传入固定种子，不同世界会共享同一柱环排列。
- 黑曜石柱使用绝对中心坐标分布（围绕 0,0），若调用方 `pos` 参数不是末地原点，会导致柱子偏离主岛中心。
- `destroying` 模式会清除柱子区域方块，用于末影龙战斗重建场景，普通生成时必须为 `false`。
- `intersectsWorldGenRegion()` 按区块范围裁剪柱子，减少跨区块无效遍历，但柱子可能跨越多区块，需确保所有相关区块都已加载。
