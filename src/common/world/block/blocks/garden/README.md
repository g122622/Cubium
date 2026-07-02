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
    └── FlowerBlock
    │   └── CactusFlowerBlock   (自定义 canSustain：仙人掌/耕地/实心顶面)
    └── FlowerBedBlock          (花瓣床：FACING + FLOWER_AMOUNT 状态，可堆叠放置/骨粉催熟)
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

| 模块 | 用途 |
|------|------|
| `world/block/blocks/vegetation/FlowerBlock` | 花朵基类 |
| `world/block/registry/VanillaBlocks` | 仙人掌方块引用 |
| `world/block/BlockTags` | 方块标签查询 |
| `world/block/IWorld` | 世界接口 |

### 下游依赖（谁依赖本模块）

| 模块 | 用途 |
|------|------|
| `world/block/registry/GardenBlocks` | 注册仙人掌花方块 |

## 容易踩的坑

1. **CactusFlowerBlock 的 canSustain 逻辑**: 仙人掌花比普通花朵放置条件更广，不仅限于泥土/耕地，还可以放置在仙人掌上方和任何具有实心顶面的方块上。不要使用 FlowerBlock 默认的 `material.isSolid()` 检查。

2. **FlowerBedBlock 的堆叠放置**: 右键已有花瓣床时 AMOUNT+1（最多4），潜行右键则正常放置新方块。`isReplaceable` 需要同时检查玩家未潜行、手持同类型物品、AMOUNT < 4 三个条件。

3. **FlowerBedBlock 的骨粉行为**: AMOUNT < 4 时增加1；AMOUNT = 4 时弹出一个自身物品而非继续增加。掉落数量由 loot table 中的 `flower_amount` block_state_property 条件控制。

4. **FlowerBedBlock 的形状计算**: 形状由 FACING 和 AMOUNT 共同决定（4x4=16种），每个花瓣段为 8x3x8 像素盒子，逆时针旋转叠加。
