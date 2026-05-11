# 泥土方块模块 (Dirt Blocks)

泥土方块模块提供各种泥土类方块的实现。

## 目录结构

```
dirt/
├── README.md                   # 本文档
├── SpreadableSnowyDirtBlock.hpp/cpp  # 可蔓延泥土基类
```

## 类层次结构

```
Block
└── SpreadableSnowyDirtBlock   # 可蔓延的雪覆盖泥土基类（带 SNOWY 属性）
    ├── GrassBlock             # 草方块（蔓延和退化机制）
    └── MyceliumBlock           # 菌丝方块（蔓延和退化机制）
```

## 核心机制

### SNOWY 属性

SpreadableSnowyDirtBlock 实现了 MC 1.16.5 的 SNOWY 属性：

- **属性定义**: `BlockStateProperties::SNOWY()` - 布尔属性，表示顶部是否覆盖雪
- **默认值**: `false` - 默认无雪

#### 属性更新时机

1. **放置时** (`getStateForPlacement`):
   - 检查放置位置上方是否有雪块 (`SNOW_BLOCK`) 或雪层 (`SNOW`)
   - 如果有则设置 `SNOWY = true`

2. **邻居更新时** (`updatePostPlacement`):
   - 只有上方 (`Direction::Up`) 方块变化时才更新
   - 检查上方是否为雪块或雪层（任意层数）
   - 更新 SNOWY 属性以反映当前状态

3. **蔓延时** (`randomTick`):
   - 当草方块蔓延到目标泥土位置时
   - 检查目标位置上方是否有雪层 (`SNOW`)
   - 注意：蔓延时只检查雪层，不检查雪块（与 MC 1.16.5 一致）

### 蔓延和退化

SpreadableSnowyDirtBlock 实现了 MC 1.16.5 的蔓延和退化机制：

1. **退化条件**：当上方光照不足或被非雪方块遮挡时，退化成泥土
2. **蔓延条件**：当光照 >= 9 且上方无水源时，向周围泥土蔓延

### 光照检测

- 使用 `getSkyLight()` 和 `getBlockLight()` 计算综合光照
- 检查上方是否有雪层且层数为 1（`LAYERS == 1`）
- 检查上方是否有完整水源（level == 8）

### isSnowyConditions 详细逻辑

参考 MC 1.16.5 `SpreadableSnowyDirtBlock.isSnowyConditions()`:

1. **单层雪**: 如果上方是雪层且只有 1 层 (`LAYERS == 1`)，直接返回 `true`
2. **满水流**: 如果上方是满水流 (`fluidState.getLevel() == 8`)，返回 `false`
3. **光照检查**: 计算上方光照，如果光照 < 15 则满足条件

## 使用方法

### 创建草方块

```cpp
auto grassBlock = std::make_unique<GrassBlock>(
    BlockProperties(Material::EARTH())
        .hardness(0.6f)
        .ticksRandomly()
);
```

### 创建菌丝

```cpp
auto mycelium = std::make_unique<MyceliumBlock>(
    BlockProperties(Materials::EARTH())
        .hardness(0.6f)
        .ticksRandomly()
);
```

### 检查 SNOWY 属性

```cpp
#include "world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"

// 检查方块是否积雪
bool isSnowy = state.get(SpreadableSnowyDirtBlock::SNOWY());

// 设置 SNOWY 属性
const BlockState& newState = state.with(SpreadableSnowyDirtBlock::SNOWY(), true);
```

## 方块 ID 映射

| 方块 | MC ID |
|------|-------|
| GrassBlock | minecraft:grass_block |
| MyceliumBlock | minecraft:mycelium |

## 状态数量

每个子类有 2 个状态（`SNOWY` 为布尔属性）：
- `snowy=false` - 无雪状态
- `snowy=true` - 积雪状态

## 依赖项

- `world/block/Block` - 方块基类
- `world/IWorld` - 世界接口
- `world/block/BlockRegistry` - 方块注册表
- `world/block/blocks/ice/SnowBlock` - 雪层方块（用于 LAYERS 属性检查）
- `util/property/Properties.hpp` - SNOWY 属性定义
- `item/context/BlockItemUseContext.hpp` - 放置上下文

## 参考文档

- MC 1.16.5 Source - SnowyDirtBlock（SNOWY 属性定义和更新逻辑）
- MC 1.16.5 Source - SpreadableSnowyDirtBlock（蔓延和退化机制）
- MC 1.16.5 Source - GrassBlock（草方块实现）
- MC 1.16.5 Source - MyceliumBlock（菌丝实现）
