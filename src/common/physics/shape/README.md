# VoxelShape 系统

本目录实现了完整的体素形状系统，用于方块碰撞检测、实体碰撞、光照遮挡检测等功能。

## 目录结构

```
shape/
├── BooleanOp.hpp             # 布尔运算函数对象（并集、交集、差集等）
├── DiscreteVoxelShape.hpp/cpp # 离散体素形状，使用位图存储体素占用状态
├── VoxelShape.hpp/cpp        # 体素形状主类，包含离散网格和浮点坐标点列表
└── Shapes.hpp/cpp            # 形状工厂类，提供创建和操作 VoxelShape 的静态方法
```

## 内部模块关系

```
BooleanOp ──┐
            ▼
DiscreteVoxelShape ◄──────────────────────────────┐
            │                                      │
            ▼                                      │
       VoxelShape ◄────────────────────────────────┤
            │                                      │
            ▼                                      │
          Shapes ──(join操作)──► BooleanOp ────────┘
              │                   (布尔运算)
              └──(创建/操作)──► VoxelShape
```

- **BooleanOp**：布尔运算函数对象，无依赖
- **DiscreteVoxelShape**：底层数据结构，使用位图存储，被 VoxelShape 组合使用
- **VoxelShape**：主类，包含 `shared_ptr<DiscreteVoxelShape>` 和浮点坐标点列表
- **Shapes**：工厂类，创建/操作 VoxelShape，执行布尔运算

## 上下游外部依赖关系

**上游依赖（本目录依赖）：**
- `common/core/Types.hpp` - 基础类型（i32, f64 等）
- `common/util/Direction.hpp` - Axis, AxisCycle, Direction 枚举
- `common/util/AxisAlignedBB.hpp` - 碰撞箱类
- `common/util/math/Vector3.hpp` - 3D 向量
- `common/world/block/BlockPos.hpp` - 方块位置

**下游依赖（依赖本目录）：**
- `common/physics/collision/CollisionShape.hpp` - 简化版碰撞形状
- `common/physics/PhysicsEngine.hpp` - 物理引擎，使用 VoxelShape 进行碰撞检测
- `common/physics/CollisionCache.hpp` - 碰撞缓存
- `common/world/block/Block.hpp` - 方块类，使用 VoxelShape 定义形状
- `common/world/block/blocks/*.cpp` - 各方块实现，定义碰撞/遮挡形状
- `common/world/lighting/engine/*.cpp` - 光照引擎，使用面遮挡检测
- `common/entity/entities/*.cpp` - 实体，使用碰撞检测
- `client/renderer/trident/chunk/ChunkMesher.cpp` - 区块网格生成

## 容易踩的坑

1. **坐标范围**：VoxelShape 使用方块本地坐标（0-1 范围），不是世界坐标。`Shapes::box(0, 0, 0, 0.5, 1, 0.5)` 表示方块的左下半部分。

2. **面形状缓存**：`VoxelShape::getFaceShape()` 有内部缓存，首次调用会计算并缓存。如果形状变化后需要重新获取面形状，应使用新对象。

3. **布尔运算性能**：`Shapes::join()` 执行布尔运算时会产生新的离散网格和坐标点列表，复杂形状运算开销较大，应缓存结果。

4. **空形状 vs 非空判断**：`Shapes::empty()` 返回单例空形状，不要用 `VoxelShape()` 默认构造（行为可能不同）。检查空形状用 `isEmpty()`，不要用 `shape.shape().isEmpty()`。

5. **contains() 半开区间**：`VoxelShape::contains(x, y, z)` 使用半开区间 `[min, max)`，边界判断需注意。

6. **DiscreteVoxelShape 边界缓存**：内部有边界缓存（m_xMin/m_xMax 等），修改体素后会标记 dirty。频繁修改后查询边界会触发重算。
