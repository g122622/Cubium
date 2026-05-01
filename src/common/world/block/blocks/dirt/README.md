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
└── SpreadableSnowyDirtBlock   # 可蔓延的雪覆盖泥土基类
    ├── GrassBlock             # 草方块（蔓延和退化机制）
    └── MyceliumBlock           # 菌丝方块（蔓延和退化机制）
```

## 核心机制

### 蔓延和退化

SpreadableSnowyDirtBlock 实现了 MC 1.16.5 的蔓延和退化机制：

1. **退化条件**：当上方光照不足或被非雪方块遮挡时，退化成泥土
2. **蔓延条件**：当光照 >= 9 且上方无水源时，向周围泥土蔓延

### 光照检测

- 使用 `getSkyLight()` 和 `getBlockLight()` 计算综合光照
- 检查上方是否有雪层（1层）
- 检查上方是否有完整水源（level == 8）

## 使用方法

### 创建草方块

```cpp
auto grassBlock = std::make_unique<GrassBlock>(
    BlockProperties(Materials::EARTH())
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

## 方块 ID 映射

| 方块 | MC ID |
|------|-------|
| GrassBlock | minecraft:grass_block |
| MyceliumBlock | minecraft:mycelium |

## 依赖项

- `world/block/Block` - 方块基类
- `world/IWorld` - 世界接口
- `world/block/BlockRegistry` - 方块注册表

## 参考文档

- MC 1.16.5 Source - SpreadableSnowyDirtBlock
- MC 1.16.5 Source - GrassBlock
- MC 1.16.5 Source - MyceliumBlock
