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
- `RotatedPillarBlock`: 轴向旋转方块基类
- `FlowerBlock`: 花朵基类
- `BlockStateProperties`: 方块状态属性定义
- `CollisionShape`: 碰撞形状
- `StateContainer`: 状态容器

## 容易踩的坑

1. **状态容器初始化**: 使用 Builder 模式在构造函数中初始化，`fillStateContainer` 保持空实现（因为状态容器已在构造函数中创建）。

2. **PaleHangingMossBlock 的支撑逻辑**: `isValidPosition` 检查上方方块是否有向下的实心面（`isSolidSide`），或上方方块本身是苍白垂苔（允许链式悬挂）。`updatePostPlacement` 在支撑变化时调度tick销毁方块，并更新TIP属性。

3. **EyeblossomBlock 的光照**: 只有开放状态才发光（等级1），关闭状态不发光。光照等级通过成员变量 `m_isOpen` 判断，而非 BlockState。
