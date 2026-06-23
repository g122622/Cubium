# 末地特征模块 (End Features)

末地特征模块提供末地维度相关的世界生成特征，包括黑曜石柱、末地折跃门、冰刺、紫颂树和末地小岛。

## 目录结构

```
end/
├── EndSpikeFeature.hpp/cpp       # 黑曜石柱（SurfaceStructures 阶段）
├── EndGatewayFeature.hpp/cpp     # 末地折跃门（SurfaceStructures 阶段）
├── IceSpikeFeature.hpp/cpp       # 冰刺（VegetalDecoration 阶段）
├── ChorusPlantFeature.hpp/cpp    # 紫颂树特征（VegetalDecoration 阶段）
├── EndIslandFeature.hpp/cpp      # 末地小岛特征（RawGeneration 阶段）
└── EndFeatures.hpp/cpp           # 末地特征注册（EndFeatureRegistry）
```

## 特征列表

| 类名 | 装饰阶段 | 说明 |
|------|----------|------|
| `EndSpikeFeature` | SurfaceStructures | 生成黑曜石柱和末影水晶 |
| `EndGatewayFeature` | SurfaceStructures | 生成末地折跃门方块和结构 |
| `IceSpikeFeature` | VegetalDecoration | 在冰原生物群系生成冰刺 |
| `ChorusPlantFeature` | VegetalDecoration | 在末地高地生成紫颂树 |
| `EndIslandFeature` | RawGeneration | 在小型末地岛屿生成末地石锥形岛屿 |

## 内部模块关系

```
ConfiguredFeatureBase (基类)
├── ConfiguredEndSpikeFeature      ─ 黑曜石柱
├── ConfiguredEndGatewayFeature    ─ 末地折跃门
├── ConfiguredIceSpikeFeature      ─ 冰刺
├── ConfiguredChorusPlantFeature   ─ 紫颂树
└── ConfiguredEndIslandFeature     ─ 末地小岛

EndFeatureRegistry
├── EndSpikeFeatures     → SurfaceStructures
├── EndGatewayFeatures   → SurfaceStructures
├── IceSpikeFeatures     → VegetalDecoration
├── ChorusPlantFeatures  → VegetalDecoration
└── EndIslandFeatures    → RawGeneration

ChorusPlantFeature.place()
└── ChorusFlowerBlock::generatePlant()    （递归生成紫颂树）
    └── ChorusFlowerBlock::growTreeRecursive()
        ├── ChorusPlantBlock::getStateWithConnections()  （计算连接状态）
        └── ChorusFlowerBlock::allNeighborsEmpty()       （检查邻居是否为空）
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 依赖 | 用途 |
|------|------|
| `world/gen/feature/ConfiguredFeature` | 配置化特征基类和特征注册表 |
| `world/gen/feature/FeatureIds` | 特征 ID 常量（EndIslandFeatureIds、ChorusPlantFeatureIds） |
| `world/gen/placement/PlacementUtils` | 放置链工具 |
| `world/gen/chunk/IChunkGenerator` | WorldGenRegion |
| `world/block/blocks/end/ChorusFlowerBlock` | 紫颂树递归生成 |
| `world/block/blocks/end/ChorusPlantBlock` | 紫颂植物连接状态计算 |
| `world/block/registry/VanillaBlocks` | 原版方块引用（末地石等） |
| `util/math/random/Random` | 随机数生成 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/gen/feature/ConfiguredFeature.cpp` | 特征注册 |
| `world/biome/BiomeGenerationSettings` | 末地生物群系添加特征 |

## 容易踩的坑

### 1. 紫颂树生成需要末地石基底

`ChorusPlantFeature::place()` 要求起始位置为空气且下方为末地石，否则返回 false。在测试时需确保末地石地面存在。

### 2. EndIslandFeature 使用 RawGeneration 阶段

末地小岛在 RawGeneration 阶段生成（地形基础阶段），而紫颂树在 VegetalDecoration 阶段生成（植被装饰阶段）。注册时注意使用正确的装饰阶段。

### 3. ChorusPlantFeature 调用 ChorusFlowerBlock 静态方法

紫颂树的实际生成逻辑在 `ChorusFlowerBlock::generatePlant()` 中，不是在 Feature 类内部。`ChorusPlantFeature` 只是检查放置条件后委托给 `ChorusFlowerBlock`。

### 4. BlockState 临时值

`ChorusPlantBlock::getStateWithConnections()` 返回 `BlockState` 值类型，传给 `WorldGenRegion::setBlockState()` 时需先存入局部变量再取地址，不能对临时值取地址。
