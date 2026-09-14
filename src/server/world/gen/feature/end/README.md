# 末地特征模块 (End Features)

末地特征模块提供末地维度相关的世界生成特征，包括黑曜石柱、末地折跃门、冰刺、紫颂树和末地小岛。

## 目录结构

```
end/
├── EndSpikeFeature.hpp/cpp       # 黑曜石柱（SurfaceStructures 阶段）
├── EndGatewayFeature.hpp/cpp     # 末地折跃门（SurfaceStructures 阶段）
├── IceSpikeFeature.hpp/cpp       # 冰刺（VegetalDecoration 阶段）
├── ChorusPlantFeature.hpp/cpp    # 紫颂树特征（VegetalDecoration 阶段）
└── EndIslandFeature.hpp/cpp      # 末地小岛特征（RawGeneration 阶段）
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

各 ConfiguredXxxFeature 由数据包 configured_feature JSON 定义 →
FeatureTypeRegistry 工厂构造 → 注册到 ConfiguredFeatureRegistry →
经 placed_feature JSON 包装为 PlacedFeature → 供 BiomeLoader 引用

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
| `world/gen/feature/ConfiguredFeature` | 配置化特征基类与 `ConfiguredFeatureRegistry` |
| `world/gen/placement/PlacementUtils` | 放置链工具 |
| `world/gen/chunk/IChunkGenerator` | WorldGenRegion |
| `world/block/blocks/end/ChorusFlowerBlock` | 紫颂树递归生成 |
| `world/block/blocks/end/ChorusPlantBlock` | 紫颂植物连接状态计算 |
| `world/block/registry/VanillaBlocks` | 原版方块引用（末地石等） |
| `util/math/random/Random` | 随机数生成 |

### 下游依赖（依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/gen/feature/ConfiguredFeatureRegistry` | 数据驱动注册末地特征 |
| `world/biome/BiomeLoader` | 末地生物群系从 JSON `features` 数组按 `ResourceLocation` 引用特征 |

## 容易踩的坑

### 1. 紫颂树生成需要末地石基底

`ChorusPlantFeature::place()` 要求起始位置为空气且下方为末地石，否则返回 false。在测试时需确保末地石地面存在。

### 2. EndIslandFeature 使用 RawGeneration 阶段

末地小岛在 RawGeneration 阶段生成（地形基础阶段），而紫颂树在 VegetalDecoration 阶段生成（植被装饰阶段）。特征的装饰阶段由 `ConfiguredXxxFeature::stage()` 返回，数据包通过 biome 的 `features` 二维数组（11 层对应 11 个 DecorationStage）将 placed_feature 放入正确阶段。

### 3. ChorusPlantFeature 调用 ChorusFlowerBlock 静态方法

紫颂树的实际生成逻辑在 `ChorusFlowerBlock::generatePlant()` 中，不是在 Feature 类内部。`ChorusPlantFeature` 只是检查放置条件后委托给 `ChorusFlowerBlock`。

### 4. BlockState 临时值

`ChorusPlantBlock::getStateWithConnections()` 返回 `BlockState` 值类型，传给 `WorldGenRegion::setBlockState()` 时需先存入局部变量再取地址，不能对临时值取地址。

### 5. EndSpikeFeature 的两类放置接口

`EndSpikeFeature` 提供两个放置接口，分别用于世界生成阶段和运行时：

- `place(WorldGenRegion&, ...)`：世界生成阶段使用，按 `chunkX`/`chunkZ` 划分仅生成中心位于该区块的柱子（避免跨区块写入）
- `placeSpike(IWorld&, ...)`：运行时使用（如龙重生阶段），立即放置单根柱子，包括柱体、笼子、基岩底座、末影水晶和底部火焰

`placeSpike` 由 `EndDragonFight` 在 `SUMMONING_PILLARS` 阶段调用，配置中 `crystalBeamTarget` 和 `crystalInvulnerable` 控制生成水晶的光束目标和无敌标志。

### 6. EndSpike::getTopBoundingBox 用于柱顶水晶扫描

`EndSpike::getTopBoundingBox()` 返回覆盖柱子整个 Y 轴的 AABB（X/Z 为柱子外接方形）。`EndDragonFight::_updateCrystalCount()` 和 `resetSpikeCrystals()` 使用此碰撞箱通过 `IWorld::getEntitiesInAABB()` 查找柱顶末影水晶。
