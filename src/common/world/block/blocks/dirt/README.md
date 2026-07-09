# 泥土方块模块 (Dirt Blocks)

泥土方块模块提供各种泥土类方块的实现，主要是可蔓延的雪覆盖泥土基类、泥巴方块及其子类。

## 目录结构

```
dirt/
├── README.md                        # 本文档
├── MudBlock.hpp/cpp                 # 泥巴方块（碰撞箱略矮，14/16格高，不可路径寻找）
└── SpreadableSnowyDirtBlock.hpp/cpp # 可蔓延泥土基类定义（草方块、菌丝的基类）
```

## 内部模块关系

```
Block
├── MudBlock                          # 泥巴方块（碰撞箱14/16格高，不可路径寻找）
└── SpreadableSnowyDirtBlock         # 可蔓延的雪覆盖泥土基类（带 SNOWY 属性）
    ├── GrassBlock                   # 草方块（蔓延和退化机制）
    └── MyceliumBlock                # 菌丝方块（蔓延和退化机制）
```

**SpreadableSnowyDirtBlock 核心职责**：
- 管理 SNOWY 属性（表示顶部是否覆盖雪）
- 实现蔓延机制（光照充足时向周围泥土蔓延）
- 实现退化机制（光照不足时退化成泥土）

**GrassBlock 额外职责**：
- 实现 IGrowable 接口（骨粉催熟）
- getBoneMealType() 返回 NEIGHBOR_SPREADER（粒子在方块上方水平扩散，3 倍数量）
- canGrow() 检查上方是否有空气
- grow() 在上方散布短草和花朵（128 次循环随机偏移）
- grow() 花朵放置基于生物群系花卉特征列表：通过 ChunkData 获取散布位置对应的生物群系，从 `BiomeGenerationSettings::getFlowerFeatureIds()` 获取花卉 `ResourceLocation` 列表，随机选取一个 id 后经 `ConfiguredFeatureRegistry::get` 解析为 `ConfiguredFlowerFeature*`（`dynamic_cast`），从其 `FlowerFeatureConfig` 随机选择花朵方块状态；无花卉特征或解析失败的生物群系回退到蒲公英

**子类差异**：GrassBlock 实现了 IGrowable 接口用于骨粉交互；MyceliumBlock 不实现 IGrowable（MC 原版行为）。

## 上下游外部依赖关系

### 上游依赖

| 依赖 | 用途 |
|------|------|
| `world/block/Block` | 方块基类 |
| `world/IWorld` | 世界接口（获取方块状态、光照、设置方块） |
| `world/biome/Biome` | 生物群系定义（获取 generationSettings） |
| `world/biome/BiomeGenerationSettings` | 生物群系生成设置（获取花卉特征列表） |
| `world/biome/BiomeRegistry` | 生物群系注册表（通过 BiomeId 查找生物群系） |
| `world/chunk/data/ChunkData` | 区块数据（获取某位置的生物群系ID） |
| `world/gen/feature/ConfiguredFeature` | 特征注册表（通过 ID 查找特征） |
| `world/gen/feature/vegetation/FlowerFeature` | 花卉特征（ConfiguredFlowerFeature、FlowerFeatureConfig） |
| `world/block/BlockRegistry` | 方块注册表（获取 DIRT 方块） |
| `world/block/blocks/ice/SnowBlock` | 雪层方块（用于 LAYERS 属性检查） |
| `world/fluid/Fluid` | 流体系统（检测水源） |
| `world/lighting/engine/LightEngineUtils` | 光照引擎工具 |
| `util/property/Properties.hpp` | SNOWY 属性定义 |
| `util/Direction.hpp` | 方向定义 |
| `item/context/BlockItemUseContext.hpp` | 放置上下文 |
| `common/world/block/registry/VanillaBlocks.hpp` | 原版方块引用（DIRT, SNOW, SNOW_BLOCK） |

### 下游依赖

| 模块 | 用途 |
|------|------|
| `VanillaBlocks` | 注册 GRASS_BLOCK、MYCELIUM 方块 |
| 世界生成 | 草方块和菌丝的生物群系生成 |
| 渲染系统 | 草方块和菌丝的模型渲染（SNOWY 属性影响纹理） |

## 容易踩的坑

### 1. MudBlock 碰撞箱略矮

泥巴的碰撞箱只有 14/16 格高（而非完整方块的 16/16），实体走在上面会略微下沉。但方块支持形状和视觉遮挡形状仍为完整方块，这意味着其他方块可以放在泥巴上方而不悬空。

### 2. SNOWY 属性更新时机

SNOWY 属性有三个更新时机，逻辑各有不同：
- **放置时**：检查 SNOW_BLOCK 或 SNOW（任意层数）
- **邻居更新时**：只响应上方方块变化，检查 SNOW_BLOCK 或 SNOW
- **蔓延时**：只检查 SNOW（雪层），不检查 SNOW_BLOCK

这个差异是 MC 1.16.5 的原版行为，蔓延时雪块下方不会设置 SNOWY=true。

### 2. 蔓延目标位置的水源检测

`isSnowyAndNotUnderwater()` 检查目标位置上方是否有流体，有流体则不蔓延。这是防止草方块在水下蔓延的机制。

### 3. 单层雪的光照条件判断

`isSnowyConditions()` 对单层雪（LAYERS == 1）直接返回 true，跳过光照检测。只有多层雪才会进入光照检查逻辑。

### 4. 蔓延随机范围

蔓延时偏移范围为：dx/dz ∈ [-1, 1]，dy ∈ [-3, 1]。这意味着草方块可以向下蔓延最多3格，向上蔓延最多1格。

### 5. 蔓延光照阈值和流体源等级

蔓延光照阈值使用 `game::GRASS_SPREAD_LIGHT_THRESHOLD` 常量（值为 9），流体源等级使用 `fluid::SOURCE_LEVEL` 常量（值为 8）。定义分别在 `Constants.hpp` 和 `Fluid.hpp` 中。

## 参考文档

- MC 1.16.5 Source - SnowyDirtBlock（SNOWY 属性定义和更新逻辑）
- MC 1.16.5 Source - SpreadableSnowyDirtBlock（蔓延和退化机制）
- MC 1.16.5 Source - GrassBlock（草方块实现）
- MC 1.16.5 Source - MyceliumBlock（菌丝实现）
