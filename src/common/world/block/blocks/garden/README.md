# 花园觉醒方块 (Garden Awakening Blocks)

1.21.4+ 花园觉醒更新特有的方块实现。

## 目录结构

```
garden/
├── CactusFlowerBlock.hpp    # 仙人掌花方块（可放置在仙人掌/耕地/实心面上）
├── CactusFlowerBlock.cpp
└── README.md                # 本文件
```

## 内部模块关系

```
Block
└── BushBlock
    └── FlowerBlock
        └── CactusFlowerBlock   (自定义 canSustain：仙人掌/耕地/实心顶面)
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
