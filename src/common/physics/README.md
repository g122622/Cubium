# Physics Module

Minecraft 兼容的物理系统，实现碰撞检测、实体移动和物理常量。

## 目录结构

```
physics/
├── collision/
│   └── CollisionShape.hpp      # 碰撞形状定义（空形状/完整方块/自定义盒）
├── shape/
│   ├── BooleanOp.hpp           # 布尔运算接口（并集/交集/差集）
│   ├── DiscreteVoxelShape.hpp/cpp  # 离散体素形状（位图存储占用状态）
│   ├── VoxelShape.hpp/cpp      # 体素形状主类（支持形状运算和碰撞检测）
│   └── Shapes.hpp/cpp          # 形状工厂类（创建基础形状和布尔运算）
├── CollisionCache.hpp/cpp      # 碰撞箱缓存（区块级别，线程安全）
├── PhysicsConstants.hpp        # 物理常量定义（重力/阻力/步进高度等）
└── PhysicsEngine.hpp/cpp       # 物理引擎核心（逐轴碰撞/自动步进/地面检测）
```

## 内部模块关系

```
PhysicsConstants ──┐
                   │
CollisionShape ────┼──► PhysicsEngine ◄──► CollisionCache
                   │         │
VoxelShape ────────┘         │
    │                        ▼
    └──► DiscreteVoxelShape   ICollisionWorld（接口）
              │                    │
              └──► BooleanOp       └──► ServerWorld / ClientWorld（实现）
```

- `PhysicsConstants` 提供所有物理常量，被 `PhysicsEngine` 和实体系统使用
- `CollisionShape` 是简化的碰撞形状，用于方块碰撞检测
- `VoxelShape` 是完整的体素形状系统，支持复杂形状（楼梯、栅栏等）和布尔运算
- `PhysicsEngine` 依赖 `ICollisionWorld` 接口获取世界数据，使用 `CollisionCache` 优化性能

## 上下游外部依赖关系

### 上游依赖（本模块依赖的模块）

- `common/core/Types.hpp` - 基础类型定义
- `common/world/WorldConstants.hpp` - 世界常量（区块尺寸、高度限制等）
- `common/util/AxisAlignedBB.hpp` - AABB 碰撞箱
- `common/util/Direction.hpp` - 方向枚举
- `common/util/math/Vector3.hpp` - 向量类型
- `common/world/block/Block.hpp` - 方块定义
- `common/world/chunk/ChunkData.hpp` - 区块数据
- `common/world/chunk/ChunkPos.hpp` - 区块坐标

### 下游依赖（依赖本模块的模块）

- `server/world/ServerWorld.hpp` - 服务端世界（实现 `ICollisionWorld`）
- `client/world/ClientWorld.hpp` - 客户端世界（实现 `ICollisionWorld`）
- `server/entity/` / `client/entity/` - 实体系统（使用物理引擎移动实体）
- `common/world/block/` - 方块系统（使用 `CollisionShape` 定义碰撞形状）
- `common/world/level/` - 光照系统（使用 `VoxelShape` 面遮挡检测）

## 容易踩的坑

### 1. 初始重叠问题

浮点误差可能导致实体轻微嵌入地面，逐轴碰撞算法不会将其推出。`PhysicsEngine::resolveInitialOverlaps()` 在移动前检测并推出重叠，限制向上推的最大距离（0.45 格）避免影响合法穿插场景。

### 2. 步进触发条件

步进仅在满足以下条件时触发：`stepHeight > 0 && horizontalCollision && (wasOnGround || (verticalCollision && movement.y < 0))`。空中时通常不会触发步进，除非正在下落且碰到垂直障碍。

### 3. 碰撞缓存失效

方块更新后必须使缓存失效，否则会使用过时的碰撞数据。方块变化时调用 `invalidateChunkAndNeighbors()`，半径通常为 1（邻居区块可能有边界方块）。

### 4. 坐标系问题

`CollisionShape` 使用方块本地坐标（0-1），`AxisAlignedBB` 使用世界坐标。使用 `getWorldBoxes()` 转换。

### 5. 空形状 vs 无碰撞

空气方块返回 `empty()` 形状，但某些特殊方块（如水、岩浆）也无碰撞但有其他属性。检查 `BlockState::isAir()` 和 `CollisionShape::isEmpty()` 区分情况。

### 6. 碰撞缓存线程安全

`CollisionCache` 返回的指针在修改后可能失效。不要长期持有 `getChunkCollisionBoxes()` 返回的指针，使用后立即释放。

### 7. 步进策略竞争

MC 1.16.5 使用三种策略竞争：策略A（整体抬起后水平移动）、策略B（先向上抬起再水平移动）、策略C（部分抬起高度水平移动）。最终选择水平移动距离最远的策略。

### 8. 精度常量

- 碰撞计算：`1.0e-7f`（与 MC 1.16.5 一致）
- 面形状检测：`1.0e-7f`
- 地面探测：`0.01f`

### 9. VoxelShape 面形状缓存

`VoxelShape` 会缓存每个方向的面形状用于光照遮挡检测。如果形状发生变化，缓存需要失效。使用 `clearFaceShapeCache()` 手动清除。
