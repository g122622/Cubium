# Physics Module

Minecraft 兼容的物理系统，实现碰撞检测、实体移动和物理常量。

## 目录结构

```
physics/
├── collision/
│   └── CollisionShape.hpp    # 碰撞形状定义
├── CollisionCache.hpp        # 碰撞箱缓存（头文件）
├── CollisionCache.cpp        # 碰撞箱缓存（实现）
├── PhysicsConstants.hpp      # 物理常量定义
├── PhysicsEngine.hpp         # 物理引擎接口
└── PhysicsEngine.cpp         # 物理引擎实现
```

## 文件详解

### PhysicsConstants.hpp

**职责**：定义 Minecraft 1.16.5 兼容的物理常量。

**内容**：
- **重力和阻力**：`GRAVITY`（0.08 blocks/tick²，MC 1.16.5 标准）、`DRAG_AIR`（0.98）、`DRAG_GROUND`（0.546，滑度*0.91）、`DRAG_WATER`、`DRAG_LAVA`
- **运动参数**：`JUMP_VELOCITY`（0.42，MC 1.16.5 标准）、`STEP_HEIGHT`（0.6）、`MOTION_THRESHOLD`（0.003）
- **滑度系数**：`SLIPPERINESS_DEFAULT`（0.6）、`SLIPPERINESS_ICE`（0.98）、`SLIPPERINESS_SLIME`（0.8）、`SLIPPERINESS_BLUE_ICE`（0.989）
- **物品物理**：`ITEM_GRAVITY`（0.04）、`ITEM_DRAG`（0.98）、`ITEM_WATER_BOUNCE_FACTOR`（0.5）
- **粒子物理**：`RAIN_GRAVITY`、`SNOW_GRAVITY`
- **实体限制**：`MAX_MOVEMENT_SPEED`、`MAX_FALL_SPEED`
- **游泳潜水**：`SWIM_JUMP_VELOCITY`、`WATER_GRAVITY`、`SWIM_SPEED_BASE`、`WATER_DRAG`、`DOLPHINS_GRACE_WATER_DRAG`
- **飞行**：`FLY_SPEED`（0.05）、`WALK_SPEED`（0.1）、`SPRINT_FLY_MULTIPLIER`（2.0）
- **梯子**：`LADDER_SPEED_MAX`、`LADDER_CLIMB_SPEED`、`LADDER_SLIDE_SPEED`
- **鞘翅**：`ELYTRA_DRAG_HORIZONTAL`、`ELYTRA_DRAG_VERTICAL`、`ELYTRA_MIN_SPEED`、`ELYTRA_LIFT_COEFFICIENT`
- **缓降**：`SLOW_FALLING_GRAVITY`（0.01）
- **特殊方块**：
  - 蜘蛛网：`COBWEB_SLOWDOWN_XZ`（0.25）、`COBWEB_SLOWDOWN_Y`（0.05）
  - 蜂蜜块：`HONEY_BLOCK_MAX_SLIDE_VELOCITY`、`HONEY_BLOCK_SLIDE_THRESHOLD`、`HONEY_BLOCK_JUMP_FACTOR`（0.5）
  - 史莱姆块：`SLIME_BLOCK_BOUNCE_FACTOR_LIVING`（1.0）、`SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING`（0.8）
  - 甜浆果丛：`SWEET_BERRY_BUSH_SLOWDOWN_XZ`（0.8）、`SWEET_BERRY_BUSH_SLOWDOWN_Y`（0.75）

**重要对齐说明**：
- 重力值 `GRAVITY = 0.08` 已与 MC 1.16.5 对齐（原错误值 0.01）
- 地面阻力计算：`slipperiness * 0.91`（默认滑度 0.6 → 0.546）
- 动态滑度：通过 `Block::getSlipperiness()` 获取，冰块 0.98，蜂蜜块 0.5

**使用方法**：
```cpp
#include "physics/PhysicsConstants.hpp"

// 应用重力
velocity.y -= mc::physics::GRAVITY;

// 应用空气阻力
velocity *= mc::physics::DRAG_AIR;

// 计算地面移动因子
f32 moveFactor = mc::physics::getGroundMoveFactor(speed, slipperiness);
```

---

### CollisionShape.hpp

**职责**：定义方块的碰撞形状，是 VoxelShape 的简化实现。

**内容**：
- **Type 枚举**：`Empty`（无碰撞）、`FullBlock`（完整方块）、`SimpleBox`（自定义盒）
- **静态工厂方法**：`empty()`、`fullBlock()`、`box(minX, minY, minZ, maxX, maxY, maxZ)`
- **碰撞检测**：`intersects()`、`getWorldBoxes()`
- **查询方法**：`isEmpty()`、`isFullBlock()`、`boxCount()`

**关键特性**：
- 碰撞箱使用方块本地坐标（0-1 范围）
- 支持多碰撞箱（如楼梯、栅栏等复杂形状）
- `getWorldBoxes()` 转换为世界坐标碰撞箱

**使用方法**：
```cpp
// 完整方块
auto stone = CollisionShape::fullBlock();

// 空形状（空气、水、岩浆）
auto air = CollisionShape::empty();

// 半砖（下半砖）
auto slab = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

// 楼梯（多个碰撞箱）
auto stairs = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f)
                 .addBox(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.5f);

// 检测碰撞
if (!shape.isEmpty() && shape.intersects(entityBox, blockX, blockY, blockZ)) {
    // 发生碰撞
}
```

---

### PhysicsEngine.hpp / PhysicsEngine.cpp

**职责**：核心物理引擎，实现 Minecraft 兼容的实体移动和碰撞检测。

**核心算法**（参考 MC `Entity.move()` 和 `Entity.getAllowedMovement()`）：

1. **逐轴碰撞检测**：
   - Y 轴优先处理（重力最重要）
   - X/Z 按移动幅度排序处理
   - 每次移动后更新实体碰撞箱位置

2. **自动步进（Auto-Step）**：
   - 当水平方向移动受阻时尝试抬起
   - 抬起高度为 `stepHeight`（玩家 0.6）
   - 使用双策略竞争：策略A（整体抬起）和策略B（先抬起后移动）
   - 选择水平移动距离最远的策略
   - 向上移动 → 水平移动 → 向下落回

3. **初始重叠解决**：
   - 处理浮点误差导致的轻微嵌入
   - 仅向上推出来避免地面穿透

**主要方法**：

| 方法 | 描述 |
|------|------|
| `moveEntity(entityBox, movement, stepHeight)` | 带碰撞检测的实体移动 |
| `isOnGround(entityBox)` | 检测是否在地面 |
| `collectCollisionBoxes(searchBox, boxes)` | 收集范围内的碰撞箱 |
| `collidedVertically()` | 上次移动是否有垂直碰撞 |
| `collidedHorizontally()` | 上次移动是否有水平碰撞 |

**ICollisionWorld 接口**：
```cpp
class ICollisionWorld {
    virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;
    virtual bool isWithinWorldBounds(i32 x, i32 y, i32 z) const = 0;
    virtual const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const = 0;
    virtual i32 getMinBuildHeight() const;
    virtual i32 getMaxBuildHeight() const;
};
```

**使用方法**：
```cpp
// 创建物理引擎（ServerWorld/ClientWorld 实现 ICollisionWorld）
PhysicsEngine physics(world);

// 移动实体
AxisAlignedBB entityBox = AxisAlignedBB::fromPosition(pos, 0.6f, 1.8f);
Vector3 actualMovement = physics.moveEntity(entityBox, desiredMovement, 0.6f);

// 更新实体位置
position += actualMovement;

// 检测地面状态
bool onGround = physics.isOnGround(entityBox);
```

---

### CollisionCache.hpp / CollisionCache.cpp

**职责**：缓存区块内的方块碰撞箱，避免每帧重新计算。

**特性**：
- **线程安全**：使用读写锁（`std::shared_mutex`）
- **版本控制**：支持区块版本号用于增量更新
- **统计信息**：命中率追踪用于性能分析

**主要方法**：

| 方法 | 描述 |
|------|------|
| `getChunkCollisionBoxes(x, z)` | 获取碰撞箱列表（未命中返回 nullptr） |
| `cacheChunkCollisionBoxes(x, z, boxes, version)` | 缓存碰撞箱 |
| `invalidateChunk(x, z)` | 使单个区块缓存失效 |
| `invalidateChunkAndNeighbors(x, z, radius)` | 使区块及邻居缓存失效 |
| `clear()` | 清除所有缓存 |
| `hitCount()` / `missCount()` | 命中/未命中统计 |

**使用方法**：
```cpp
CollisionCache cache;

// 尝试获取缓存
const auto* boxes = cache.getChunkCollisionBoxes(chunkX, chunkZ);
if (!boxes) {
    // 缓存未命中，重新计算
    std::vector<AxisAlignedBB> newBoxes = computeCollisionBoxes(chunkX, chunkZ);
    cache.cacheChunkCollisionBoxes(chunkX, chunkZ, std::move(newBoxes), version);
}

// 方块更新时使缓存失效
world->setBlockState(x, y, z, newState);
cache.invalidateChunkAndNeighbors(chunkX, chunkZ, 1);
```

---

## 模块关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                        Physics Module                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────┐     ┌──────────────────┐                 │
│  │ PhysicsConstants │     │  CollisionShape  │                 │
│  │     (常量)        │     │    (形状定义)     │                 │
│  └──────────────────┘     └────────┬─────────┘                 │
│                                    │                            │
│                                    ▼                            │
│  ┌──────────────────┐     ┌──────────────────┐                 │
│  │  CollisionCache  │◄───►│  PhysicsEngine   │                 │
│  │   (缓存层)        │     │   (核心引擎)      │                 │
│  └──────────────────┘     └────────┬─────────┘                 │
│                                    │                            │
└────────────────────────────────────┼────────────────────────────┘
                                     │
                                     ▼
           ┌─────────────────────────────────────────────┐
           │              ICollisionWorld                │
           │   (ServerWorld / ClientWorld 实现)          │
           └─────────────────────────────────────────────┘
                                     │
                                     ▼
           ┌─────────────────────────────────────────────┐
           │   Block / BlockState / ChunkData            │
           │   (提供方块碰撞形状和世界数据)               │
           └─────────────────────────────────────────────┘
```

---

## 整体职责

1. **碰撞检测**：提供高效的方块碰撞箱检测和收集
2. **实体移动**：实现 Minecraft 兼容的逐轴碰撞解决
3. **自动步进**：支持上楼梯/台阶的自动抬高
4. **性能优化**：通过碰撞缓存避免重复计算

## 输入和输出

### 输入

| 来源 | 数据 | 描述 |
|------|------|------|
| ICollisionWorld | BlockState | 方块状态（包含碰撞形状） |
| ICollisionWorld | ChunkData | 区块数据 |
| PhysicsEngine | AxisAlignedBB | 实体碰撞箱 |
| PhysicsEngine | Vector3 | 期望移动向量 |
| PhysicsEngine | float | 步进高度 |

### 输出

| 输出 | 描述 |
|------|------|
| Vector3 | 实际移动向量（碰撞处理后） |
| bool | 地面检测状态 |
| bool | 垂直/水平碰撞状态 |
| std::vector<AxisAlignedBB> | 碰撞箱列表（从缓存获取） |

## 依赖项

### 内部依赖

```
physics/
├── common/core/Types.hpp          # 基础类型定义
├── common/util/AxisAlignedBB.hpp  # AABB 碰撞箱
├── common/util/Direction.hpp      # 方向枚举
├── common/util/math/Vector3.hpp   # 向量类型
├── common/world/block/Block.hpp   # 方块定义
├── common/world/chunk/ChunkData.hpp # 区块数据
└── common/world/chunk/ChunkPos.hpp # 区块坐标
```

### 外部依赖

- `spdlog` - 日志输出
- `shared_mutex` - 读写锁（C++20）

## 使用方法

### 基本使用

```cpp
#include "physics/PhysicsEngine.hpp"
#include "physics/PhysicsConstants.hpp"

// 1. 实现或获取 ICollisionWorld 接口
ServerWorld world;  // ServerWorld 实现了 ICollisionWorld

// 2. 创建物理引擎
PhysicsEngine physics(world);

// 3. 移动实体
AxisAlignedBB box = player->getBoundingBox();
Vector3 movement(0.1f, -0.08f, 0.0f);  // 水平移动 + 重力
Vector3 actual = physics.moveEntity(box, movement, 0.6f);
player->setPosition(player->position() + actual);
player->setOnGround(physics.isOnGround(box));
```

### 使用碰撞缓存

```cpp
#include "physics/CollisionCache.hpp"

// 在世界类中集成
class ServerWorld : public ICollisionWorld {
    CollisionCache m_collisionCache;

    void onBlockChanged(BlockPos pos) {
        // 方块变化时使缓存失效
        ChunkCoord cx = pos.x >> 4;
        ChunkCoord cz = pos.z >> 4;
        m_collisionCache.invalidateChunkAndNeighbors(cx, cz);
    }
};
```

### 物理常量应用

```cpp
#include "physics/PhysicsConstants.hpp"

void Entity::applyGravity() {
    if (isInWater()) {
        velocity.y -= physics::WATER_GRAVITY;
        velocity *= physics::DRAG_WATER;
    } else if (isInLava()) {
        velocity.y -= physics::GRAVITY;
        velocity *= physics::DRAG_LAVA;
    } else {
        velocity.y -= physics::GRAVITY;
        velocity *= physics::DRAG_AIR;
    }
}
```

## 容易踩的坑

### 1. 初始重叠问题

**问题**：浮点误差可能导致实体轻微嵌入地面，逐轴碰撞算法不会将其推出。

**解决方案**：`PhysicsEngine::resolveInitialOverlaps()` 在移动前检测并推出重叠，限制向上推的最大距离（0.45 格）避免影响合法穿插场景。

```cpp
// PhysicsEngine 内部处理，用户无需关心
f32 overlapPushUp = resolveInitialOverlaps(entityBox, boxes);
```

### 2. 步进高度检查时机

**问题**：步进仅在水平碰撞且满足条件时触发。

**条件**：`stepHeight > 0 && horizontalCollision && (wasOnGround || (verticalCollision && movement.y < 0))`

**注意**：空中时通常不会触发步进，除非正在下落且碰到垂直障碍。

### 3. 碰撞缓存失效

**问题**：方块更新后必须使缓存失效，否则会使用过时的碰撞数据。

**解决方案**：方块变化时调用 `invalidateChunkAndNeighbors()`，半径通常为 1（邻居区块可能有边界方块）。

```cpp
// 错误：忘记使缓存失效
world->setBlockState(pos, newState);
// 物理引擎仍使用旧缓存！

// 正确：
world->setBlockState(pos, newState);
cache.invalidateChunkAndNeighbors(pos.x >> 4, pos.z >> 4, 1);
```

### 4. 坐标系问题

**问题**：`CollisionShape` 使用方块本地坐标（0-1），`AxisAlignedBB` 使用世界坐标。

**解决方案**：使用 `getWorldBoxes()` 转换。

```cpp
// 方块本地碰撞箱
CollisionShape shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f);

// 转换为世界坐标
auto worldBoxes = shape.getWorldBoxes(blockX, blockY, blockZ);
```

### 5. 空形状 vs 无碰撞

**问题**：空气方块返回 `empty()` 形状，但某些特殊方块（如水、岩浆）也无碰撞但有其他属性。

**解决方案**：检查 `BlockState::isAir()` 和 `CollisionShape::isEmpty()` 区分情况。

```cpp
if (state->isAir()) {
    // 空气，完全跳过
} else if (shape.isEmpty()) {
    // 非固体方块（水、岩浆、花等）
}
```

### 6. 线程安全

**问题**：`CollisionCache` 使用读写锁，但返回的指针在修改后可能失效。

**解决方案**：不要长期持有 `getChunkCollisionBoxes()` 返回的指针，使用后立即释放。

```cpp
// 危险：指针在后续操作后可能失效
const auto* boxes = cache.getChunkCollisionBoxes(x, z);
// ... 其他操作可能导致缓存更新 ...
for (const auto& box : *boxes) { }  // 可能崩溃！

// 安全：立即使用
if (const auto* boxes = cache.getChunkCollisionBoxes(x, z)) {
    for (const auto& box : *boxes) { }  // OK
}
// 指针离开作用域，不再持有
```

## 涉及的测试用例

### PhysicsEngineTests.cpp

| 测试名称 | 描述 |
|---------|------|
| `PhysicsConstantsTest.GravityValue_Correct` | 重力值验证 |
| `PhysicsConstantsTest.JumpVelocity_Correct` | 跳跃速度验证 |
| `PhysicsConstantsTest.StepHeight_Correct` | 步进高度验证 |
| `PhysicsConstantsTest.DragValues_Correct` | 阻力值验证 |
| `PhysicsConstantsTest.SlipperinessValues_Correct` | 滑度值验证 |
| `PhysicsConstantsTest.GroundMoveFactor_Correct` | 地面移动因子验证 |
| `PhysicsConstantsTest.SpecialBlockConstants_Correct` | 特殊方块常量验证 |
| `PhysicsConstantsTest.SwimConstants_Correct` | 游泳常量验证 |
| `PhysicsConstantsTest.FlyConstants_Correct` | 飞行常量验证 |
| `PhysicsConstantsTest.ItemConstants_Correct` | 物品常量验证 |
| `PhysicsConstantsTest.LadderConstants_Correct` | 梯子常量验证 |
| `PhysicsConstantsTest.ElytraConstants_Correct` | 鞘翅常量验证 |
| `PhysicsConstantsTest.SlowFallingGravity_Correct` | 缓降重力验证 |
| `CollisionCacheThreadSafeTest.CacheAndRetrieve` | 线程安全缓存和检索 |
| `CollisionCacheThreadSafeTest.CacheMiss` | 线程安全缓存未命中 |
| `CollisionCacheThreadSafeTest.InvalidateChunk` | 线程安全区块失效 |
| `CollisionCacheThreadSafeTest.InvalidateChunkAndNeighbors` | 线程安全邻居失效 |
| `CollisionCacheThreadSafeTest.HitMissStats` | 线程安全命中统计 |
| `CollisionCacheThreadSafeTest.ThreadSafeHitMissStats` | 多线程统计原子性 |

### CollisionCacheTests.cpp

| 测试名称 | 描述 |
|---------|------|
| `CacheAndRetrieve` | 缓存存储和检索 |
| `CacheCopy` | 拷贝版本缓存 |
| `RetrieveNonExistent` | 检索不存在的缓存 |
| `MultipleChunks` | 缓存多个区块 |
| `InvalidateSingle` | 使单个区块缓存失效 |
| `InvalidateNonExistent` | 使不存在的缓存失效 |
| `InvalidateAndNeighbors` | 使区块及邻居缓存失效 |
| `ClearAll` | 清除所有缓存 |
| `HitMissStats` | 命中/未命中统计 |
| `VersionNumber` | 版本号测试 |
| `NegativeCoordinates` | 负坐标测试 |
| `EmptyBoxList` | 空碰撞箱列表 |
| `LargeNumberOfBoxes` | 大量碰撞箱测试 |

### ServerWorldCollisionTests.cpp

| 测试名称 | 描述 |
|---------|------|
| `PhysicsEngineInitialized` | 物理引擎初始化验证 |
| `CollisionCacheInitialized` | 碰撞缓存初始化验证 |
| `HasBlockCollisionEmptyWorld` | 空世界碰撞检测 |
| `HasBlockCollisionWithAir` | 空气区域碰撞检测 |
| `HasBlockCollisionWithGround` | 地面碰撞检测 |
| `GetBlockCollisionsEmptyArea` | 空区域碰撞箱获取 |
| `HasEntityCollisionNoEntities` | 无实体时碰撞检测 |
| `HasEntityCollisionWithEntity` | 有实体时碰撞检测 |
| `HasEntityCollisionExceptSelf` | 排除自身的碰撞检测 |
| `GetEntityCollisions` | 获取碰撞实体列表 |
| `PhysicsEngineMoveEntity` | 实体移动测试 |
| `PhysicsEngineIsOnGround` | 地面检测测试 |
| `InvalidateCollisionCache` | 缓存失效测试 |
| `ICollisionWorldGetBlockState` | ICollisionWorld 接口测试 |
| `ICollisionWorldIsWithinWorldBounds` | 世界边界测试 |
| `ICollisionWorldGetChunkAt` | 区块获取测试 |

### PlayerMovementTest.cpp

| 测试名称 | 描述 |
|---------|------|
| `FlySpeedDefaultValue_IsCorrect` | 飞行速度默认值 |
| `WalkSpeedDefaultValue_IsCorrect` | 行走速度默认值 |
| `CreativeMode_HasFlyAbility` | 创造模式飞行能力 |
| `SurvivalMode_NoFlyAbility` | 生存模式无飞行 |
| `Flying_HorizontalMovement_AddsToVelocity` | 飞行水平移动 |
| `Flying_Sprint_DoublesSpeed` | 冲刺加速 |

## 参考资料

- Minecraft 1.16.5 源码
  - `Entity.move()` - 核心移动逻辑
  - `Entity.getAllowedMovement()` - 碰撞解决和步进
  - `Entity.collideBoundingBox()` - 逐轴碰撞计算
  - `VoxelShape` - 复杂碰撞形状
