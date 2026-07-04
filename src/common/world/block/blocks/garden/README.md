# 花园觉醒方块 (Garden Awakening Blocks)

1.21.4+ 花园觉醒更新特有的方块实现。

## 目录结构

```
garden/
├── CactusFlowerBlock.hpp    # 仙人掌花方块（可放置在仙人掌/耕地/实心面上）
├── CactusFlowerBlock.cpp
├── FlowerBedBlock.hpp       # 花瓣床方块（粉红色花瓣/野花，可堆叠放置）
├── FlowerBedBlock.cpp
└── README.md                # 本文件
```

## 内部模块关系

```
Block
└── BushBlock
    ├── FlowerBlock
    │   └── CactusFlowerBlock   (自定义 canSustain：仙人掌/耕地/实心顶面)
    └── FlowerBedBlock          (花瓣床：FACING + FLOWER_AMOUNT 状态，可堆叠放置/骨粉催熟)
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/blocks/agricultural/BushBlock` | 花瓣床/花朵的植物基类（提供 canSurvive 等） |
| `world/block/IGrowable` | 骨粉催熟接口（FlowerBedBlock 实现） |
| `world/block/BlockItemRegistry` | 堆叠放置时检查手持物品是否为同类型 BlockItem |
| `entity/utils/ItemDropHelper` | 骨粉催熟满4时弹出物品实体 |
| `entity/entities/player/Player` | isReplaceable 中检查玩家是否潜行 |
| `world/block/IWorld` | 世界接口 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/GardenBlocks` | 注册 WILDFLOWERS（野花） |
| `world/block/registry/TrailsBlocks` | 注册 PINK_PETALS（粉红色花瓣） |

## 容易踩的坑

1. **CactusFlowerBlock 的 canSustain 逻辑**: 仙人掌花比普通花朵放置条件更广，不仅限于泥土/耕地，还可以放置在仙人掌上方和任何具有实心顶面的方块上。不要使用 FlowerBlock 默认的 `material.isSolid()` 检查。

2. **FlowerBedBlock 的堆叠放置**: 右键已有花瓣床时 AMOUNT+1（最多4）。堆叠逻辑由 `isReplaceable()` 虚方法控制，需同时满足三个条件：玩家未潜行、手持同类型物品、AMOUNT < 4。`getStateForPlacement()` 负责处理具体的堆叠状态计算（保持原朝向、AMOUNT+1）。

3. **FlowerBedBlock 的骨粉行为**: AMOUNT < 4 时增加1；AMOUNT = 4 时弹出一个自身物品而非继续增加。掉落数量由 loot table 中的 `flower_amount` block_state_property 条件控制。

4. **FlowerBedBlock 的形状计算**: 形状由 FACING 和 AMOUNT 共同决定（4x4=16种），每个花瓣段为 8x3x8 像素盒子，逆时针旋转叠加。

5. **野花地物生成**: MC Java 中野花通过 `WildflowerFeature` 在世界生成时放置（初始 AMOUNT 为1-4随机，4 朝向 × 4 数量共 16 种状态等权重）。本项目复用 `FlowerFeature` 实现，通过 `FlowerFeatures::createWildflowersBirchForest()`（tries=64）和 `FlowerFeatures::createWildflowersMeadow()`（tries=8，稀疏分布）两个预设配置，分别在白桦森林和草甸生物群系中生成。
