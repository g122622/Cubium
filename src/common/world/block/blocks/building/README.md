# Building Blocks 模块

建筑相关方块实现，包括楼梯、台阶、墙、栅栏、活板门等。

## 目录结构

```
building/
├── StairsBlock.hpp/cpp      # 楼梯方块（内角/外角自动检测、含水支持）
├── SlabBlock.hpp/cpp        # 台阶方块（单层/双层、含水仅限单层）
├── WallBlock.hpp/cpp        # 墙方块（高度连接检测、含水支持）
├── FenceBlock.hpp/cpp       # 栅栏方块（连接检测、含水支持）
├── TrapDoorBlock.hpp/cpp    # 活板门方块（红石控制、攀爬支持）
└── README.md
```

## 模块关系

```
Block (基类)
├── StairsBlock (楼梯) ──── IWaterLoggable
├── SlabBlock (台阶) ────── IWaterLoggable
├── WallBlock (墙) ──────── IWaterLoggable
├── FenceBlock (栅栏) ───── IWaterLoggable
└── TrapDoorBlock (活板门) ─ IWaterLoggable
```

## 上下游依赖

### 上游依赖
- `Block.hpp` - 方块基类
- `BlockRegistry.hpp` - 方块注册表
- `Properties.hpp` - 状态属性（HORIZONTAL_FACING, HALF, STAIRS_SHAPE, SLAB_TYPE, WALL_HEIGHT_*, OPEN, POWERED 等）
- `Direction.hpp` - 方向定义
- `CollisionShape.hpp` - 碰撞形状
- `WaterLoggableHelpers.hpp` - 含水方块工具函数
- `FenceGateHelpers.hpp` - 栅栏门连接检测工具函数（FenceBlock、WallBlock 共享）
- `IWorld.hpp` - 世界接口

### 下游依赖
- `VanillaBlocks.hpp` - 注册原版建筑方块

## 容易踩的坑

### 1. SlabBlock 双层台阶不能含水

`isWaterlogged()` 和 `canContainFluid()` 必须检查 `SLAB_TYPE` 是否为 `Double`，双层台阶永远返回 `false`。否则会导致含水状态与视觉不一致。

### 2. StairsBlock 形状索引映射

形状索引使用硬编码常量 `SHAPE_INDEX_MAP[20]`，将 `(shape.ordinal() * 4 + facing.getHorizontalIndex())` 映射到形状数组索引。修改形状枚举顺序时必须同步更新此数组。

### 3. StairsBlock 内角/外角检测

`_calculateShape()` 检测邻居楼梯时需要三个辅助方法配合：
- `_neighborIsStairs()`: 检查邻居是否为同层、朝向垂直的楼梯，返回朝向信息
- `_isDifferentStairs()`: 对应 MC 的 `canTakeShape`，检查第三位置是否阻止角形状形成
- `isStairs()`: 静态方法，检查方块状态是否包含 `STAIRS_SHAPE` 属性

忽略任一条件或辅助方法的逻辑错误会导致错误的形状计算。
`getStateForPlacement()` 会在放置时立即调用 `_calculateShape()`，确保楼梯一放置就有正确的角形状。

### 4. WallBlock 形状数量巨大

墙有 162 种形状组合（`2 * 3^4 = 162`：UP × NORTH/LOW/TALL × EAST/LOW/TALL × SOUTH/LOW/TALL × WEST/LOW/TALL）。形状缓存数组必须预计算，运行时组合会导致性能问题。

### 5. FenceBlock/WallBlock 连接检测

`_canConnect()` 和 `_getWallHeight()` 必须正确处理三种情况：
- 栅栏门：使用 `FenceGateHelpers::isFenceGateParallel()` 检测是否平行连接
- 同类方块：栅栏连栅栏，墙连墙
- 固体方块：`isSolid()` 为 true 时连接

漏掉栅栏门平行检测会导致视觉错误。

### 6. TrapDoorBlock 铁活板门不能手动操作

`onBlockActivated()` 中必须检查 `m_isIron`，铁活板门只能通过红石控制，玩家右键应返回 `ActionResultType::Pass`。

### 7. TrapDoorBlock 攀爬检测

`isLadder()` 只对打开状态的活板门返回 `true`。MC 1.16.5 中，打开的活板门可以作为梯子攀爬，但需要检查实体是否在正确侧（上半/下半）。

### 8. 含水方块必须调度流体 Tick

所有含水方块在 `updatePostPlacement()` 中，如果 `WATERLOGGED` 为 true，必须调用 `waterloggable::scheduleWaterTick(world, pos)`。否则水流动后方块内的水不会更新。

### 9. 形状缓存初始化时机

`m_shapes` 数组在构造函数中预计算。如果在构造函数之外延迟初始化，需要保证线程安全。推荐在构造函数中一次性完成。

### 10. FenceBlock 遮挡形状

栅栏的 `getOcclusionShape()` 返回空形状（不阻挡光线），但 `getShape()` 和 `getCollisionShape()` 返回实际形状。不要混淆这两个方法。
