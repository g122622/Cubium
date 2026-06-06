# 苍白花园方块 (Pale Garden Blocks)

本目录包含苍白花园生物群系特有的方块实现。

## 目录结构

```
pale_garden/
├── CreakingHeartBlock.hpp    # 嘎枝之心方块
├── CreakingHeartBlock.cpp
├── PaleHangingMossBlock.hpp  # 苍白苔藓方块
├── PaleHangingMossBlock.cpp
├── EyeblossomBlock.hpp       # 眼 blossom 花朵方块
├── EyeblossomBlock.cpp
└── README.md                 # 本文件
```

## 文件介绍

### CreakingHeartBlock

嘎枝之心方块，苍白橡木中的核心方块。

- **属性**:
  - `AXIS`: 坐标轴（X/Y/Z）
  - `CREAKING_HEART_STATE`: 状态（Uprooted/Dormant/Awake）
  - `NATURAL`: 是否自然生成
- **特性**:
  - 支持红石比较器输出（Uprooted=0, Dormant=1, Awake=2）
  - 继承自 RotatedPillarBlock

### PaleHangingMossBlock

苍白苔藓方块，悬挂在苍白橡树上的苔藓。

- **属性**:
  - `TIP`: 是否为末端
- **特性**:
  - 无碰撞（noCollision）
  - 非固体（notSolid）
  - 使用形状进行光照遮挡
  - 形状:
    - 非末端: box(2, 0, 2, 14, 16, 14)
    - 末端: box(2, 0, 2, 14, 10, 14)
  - 支持骨粉催生向下生长（待实现）

### EyeblossomBlock

眼 blossom 花朵，苍白花园中的特殊花朵。

- **变体**:
  - `open_eyeblossom`: 开放状态，发光等级为1
  - `closed_eyeblossom`: 关闭状态，不发光
- **特性**:
  - 继承自 FlowerBlock
  - 开放状态会发光
  - 可疑炖汤效果（待实现）:
    - 开放: 失明效果 (BLINDNESS)
    - 关闭: 恶心效果 (NAUSEA)

## 内部模块关系

```
Block
├── RotatedPillarBlock
│   └── CreakingHeartBlock
└── BushBlock
    └── FlowerBlock
        └── EyeblossomBlock

PaleHangingMossBlock 直接继承 Block
```

## 外部依赖关系

- `Block`: 基础方块类
- `FlowerBlock`: 花朵基类
- `BlockStateProperties`: 方块状态属性定义
- `CollisionShape`: 碰撞形状
- `StateContainer`: 状态容器

## 容易踩的坑

1. **PaleHangingMossBlock 的 isValidPosition**: 目前只返回 true，需要后续检查上方是否为苍白橡木原木、树叶或苍白苔藓。

2. **EyeblossomBlock 的光照**: 只有开放状态才发光，关闭状态不发光。

3. **状态容器初始化**: 使用 Builder 模式在构造函数中初始化，而不是 fillStateContainer（保持空实现）。
