# VoxelShape 系统

本目录实现了完整的体素形状系统，用于方块碰撞检测、光照遮挡检测等功能。

## 架构概览

```
shape/
├── BooleanOp.hpp       # 布尔运算接口
├── DiscreteVoxelShape.hpp/cpp  # 离散体素形状
├── VoxelShape.hpp/cpp  # 体素形状主类
└── Shapes.hpp/cpp      # 形状工厂类
```

## 核心类

### BooleanOp

布尔运算函数对象，用于形状之间的布尔运算：

- `And()` - 交集 (a && b)
- `Or()` - 并集 (a || b)
- `OnlyFirst()` - 仅第一个 (a && !b)
- `OnlySecond()` - 仅第二个 (!a && b)
- `NotSame()` - 不等 (a != b)

### DiscreteVoxelShape

离散体素形状，使用位图存储体素占用状态：

- 支持 3D 体素网格
- 高效的位存储
- 支持填充、清除、遍历等操作
- 边界缓存优化
- **盒子合并算法**：`forAllBoxes(consumer, simplify=true)` 可将相邻体素合并为更大的盒子

关键方法：

- `isFull(x, y, z)` / `fill(x, y, z)` / `clear(x, y, z)` - 体素操作
- `firstFull(axis)` / `lastFull(axis)` - 边界查询
- `forAllBoxes(consumer, simplify)` - 遍历所有盒子（支持合并）
- `forAllFaces(consumer)` - 遍历所有面
- `forAllEdges(consumer, simplify)` - 遍历所有边
- `isZAxisLineFull(fromZ, toZ, x, y)` - 检查 Z 轴线段是否完全填充
- `setZAxisLine(fromZ, toZ, x, y, filled)` - 设置 Z 轴线段填充状态
- `isXZRectangleFull(fromX, toX, fromZ, toZ, y)` - 检查 XZ 矩形是否完全填充

### VoxelShape

体素形状主类，包含：

- 离散体素网格
- 浮点坐标点列表
- 面形状缓存

关键方法：
- `min(axis)` / `max(axis)` - 边界查询
- `isEmpty()` - 空检查
- `getFaceShape(direction)` - 获取面形状（用于光照遮挡）
- `collide(axis, entityBox, movement)` - 碰撞检测
- `move(dx, dy, dz)` - 移动形状
- `optimize()` - 优化形状

### Shapes

形状工厂类，提供：

- `empty()` - 空形状
- `block()` - 完整方块
- `box(minX, minY, minZ, maxX, maxY, maxZ)` - 创建盒子
- `join(a, b, op)` - 布尔运算
- `or_(a, b)` - 并集
- `faceShapeOccludes(shape1, shape2)` - 面遮挡检测
- `blockOccludes(sourceShape, targetShape, direction)` - 方块面遮挡检测
- `slice(shape, axis, index)` - 切片操作

## 光照系统集成

面遮挡检测是光照系统的核心：

```cpp
// 检查两个面形状是否互相遮挡
bool occluded = Shapes::faceShapeOccludes(faceShape1, faceShape2);

// 检查两个方块之间的面是否遮挡
bool blocked = Shapes::blockOccludes(sourceShape, targetShape, Direction::North);

// 检查合并面是否遮挡
bool mergedBlocked = Shapes::mergedFaceOccludes(sourceShape, targetShape, direction);
```

## 使用示例

### 创建形状

```cpp
// 完整方块
auto fullBlock = Shapes::block();

// 空形状
auto empty = Shapes::empty();

// 自定义盒子
auto box = Shapes::box(0.0, 0.0, 0.0, 0.5, 1.0, 0.5);  // 半砖
```

### 面形状获取

```cpp
// 获取北面的面形状（用于光照遮挡检测）
VoxelShape northFace = shape.getFaceShape(Direction::North);
```

### 碰撞检测

```cpp
// 计算实体在Y轴方向的碰撞偏移
f64 actualMovement = shape.collide(Axis::Y, entityBox, desiredMovement);
```

## 参考

本实现参考 Minecraft 1.21.11 的 VoxelShape 系统：

- `net.minecraft.world.phys.shapes.VoxelShape`
- `net.minecraft.world.phys.shapes.Shapes`
- `net.minecraft.world.phys.shapes.DiscreteVoxelShape`
- `net.minecraft.world.phys.shapes.BooleanOp`

## 与光照系统的关系

VoxelShape 的面遮挡检测用于确定光线是否可以通过两个相邻方块之间的边界：

1. 每个方块有一个遮挡形状 (`getOcclusionShape()`)
2. 通过 `getFaceShape(direction)` 获取特定方向的投影形状
3. 使用 `faceShapeOccludes()` 检查两个投影形状是否完全遮挡单位正方形
4. 如果遮挡，光线无法通过该边界

这支持条件透明方块的正确光照传播（如台阶、楼梯、栅栏等）。
