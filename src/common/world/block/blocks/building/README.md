# Building Blocks 模块

建筑相关方块实现，包括楼梯、台阶、墙、栅栏、活板门等。

## 目录结构

```
building/
├── StairsBlock.hpp/cpp      # 楼梯方块
├── SlabBlock.hpp/cpp        # 台阶方块
├── WallBlock.hpp/cpp        # 墙方块
├── FenceBlock.hpp/cpp       # 栅栏方块
├── TrapDoorBlock.hpp/cpp    # 活板门方块
├── PaneBlock.hpp/cpp        # 玻璃板/铁栏杆
└── README.md
```

## 文件介绍

### StairsBlock.hpp/cpp
楼梯方块，支持内角/外角自动检测和连接。

**状态属性**：
- `FACING`: 水平朝向 (NORTH, SOUTH, EAST, WEST)
- `HALF`: 上半/下半 (TOP, BOTTOM)
- `SHAPE`: 楼梯形状 (STRAIGHT, INNER_LEFT, INNER_RIGHT, OUTER_LEFT, OUTER_RIGHT)

### SlabBlock.hpp/cpp
台阶方块，支持单层和双层状态。

**状态属性**：
- `TYPE`: 台阶类型 (BOTTOM, TOP, DOUBLE)
- `WATERLOGGED`: 是否含水

### WallBlock.hpp/cpp
墙方块，支持与相邻墙/栅栏连接。

**状态属性**：
- `UP`: 是否有顶部突起
- `NORTH/WEST/EAST/SOUTH`: 各方向连接高度 (NONE, LOW, TALL)
- `WATERLOGGED`: 是否含水

### FenceBlock.hpp/cpp
栅栏方块，支持与相邻栅栏/墙连接。

**状态属性**：
- `NORTH/WEST/EAST/SOUTH`: 各方向是否连接
- `WATERLOGGED`: 是否含水

### TrapDoorBlock.hpp/cpp
活板门方块，支持红石控制，并在开合时播放对应的木/铁活板门音效。

**状态属性**：
- `FACING`: 水平朝向
- `OPEN`: 是否打开
- `HALF`: 上半/下半
- `POWERED`: 是否被充能
- `WATERLOGGED`: 是否含水

## 模块关系

```
Block (基类)
├── StairsBlock (楼梯)
├── SlabBlock (台阶)
├── WallBlock (墙)
├── FenceBlock (栅栏)
└── TrapDoorBlock (活板门)
```

## 输入/输出

### 输入
- 方块属性 (BlockProperties)
- 世界状态 (IWorld)
- 邻居方块状态

### 输出
- 方块状态 (BlockState)
- 碰撞形状 (CollisionShape)
- 连接检测结果

## 依赖项

- `Block.hpp` - 方块基类
- `BlockRegistry.hpp` - 方块注册表
- `Properties.hpp` - 状态属性
- `Direction.hpp` - 方向定义
- `CollisionShape.hpp` - 碰撞形状
