# Collision Module

Minecraft 兼容的碰撞形状系统，提供方块碰撞箱的定义和操作。

## 目录结构

```
collision/
└── CollisionShape.hpp    # 碰撞形状定义（VoxelShape 简化实现，支持空形状/完整方块/多盒组合）
```

## 内部模块关系

本目录仅包含一个头文件 `CollisionShape.hpp`，定义了方块的碰撞形状抽象：
- `Type` 枚举：`Empty`（无碰撞）、`FullBlock`（完整方块）、`SimpleBox`（多碰撞箱）
- 提供静态工厂方法：`empty()`、`fullBlock()`、`box(...)`
- 支持链式添加碰撞箱：`addBox(...)`
- 提供 `intersects()` 检测与实体碰撞箱是否相交
- 提供 `getWorldBoxes()` 转换为世界坐标碰撞箱
- 提供 `getFaceShape()` 获取面投影（用于光照遮挡检测）

## 上下游外部依赖关系

### 依赖项

```cpp
#include "../../util/AxisAlignedBB.hpp"  // AABB 碰撞箱
#include "../../util/Direction.hpp"       // 方向枚举（Axis 枚举）
#include "../../core/Types.hpp"           // 基础类型（u8, f32 等）
```

### 被依赖项

- `physics/PhysicsEngine.hpp` - 使用 `CollisionShape` 进行碰撞检测
- `world/block/Block.hpp` - 方块通过 `BlockState` 提供 `CollisionShape`
- `physics/CollisionCache.hpp` - 缓存方块碰撞箱

## 容易踩的坑

### 1. 坐标系混淆

`CollisionShape` 使用方块本地坐标（0-1），`AxisAlignedBB` 使用世界坐标。使用 `getWorldBoxes(blockX, blockY, blockZ)` 转换为世界坐标。

```cpp
// 错误：直接使用世界坐标
CollisionShape shape = CollisionShape::box(10.0f, 64.0f, 20.0f, 11.0f, 65.0f, 21.0f);

// 正确：使用本地坐标，通过 getWorldBoxes 转换
CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
auto worldBoxes = shape.getWorldBoxes(10, 64, 20);
```

### 2. 空形状 vs 完整方块

- `empty()` - 无碰撞（空气、水、岩浆、花等）
- `fullBlock()` - 完整方块碰撞（石头、泥土等）

检查顺序很重要：先检查 `isEmpty()` 再调用 `intersects()` 避免不必要的相交检测。

### 3. 多碰撞箱的累积会改变类型

使用 `addBox()` 会改变形状类型，`fullBlock()` 调用 `addBox()` 后变为 `SimpleBox`。

### 4. 性能考虑

避免频繁创建 `CollisionShape`，建议使用静态缓存：

```cpp
static const CollisionShape& getBottomShape() {
    static CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);
    return shape;
}
```

### 5. 面投影的面边界判断

`getFaceShape(Direction)` 用于光照系统判断光线是否穿过相邻方块边界。边界检查使用 `EPSILON = 1.0e-4f` 容差（单精度浮点）。如果形状未延伸到指定面的边界，返回空形状。
