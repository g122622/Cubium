# 世界边界模块 (World Border)

提供 Minecraft 1.16.5 风格的世界边界系统，支持边界大小设置、中心点设置、伤害计算、警告效果等功能。

## 目录结构

```
border/
├── WorldBorder.hpp       # 世界边界类定义，包含 IBorderListener/IBorderState 接口
├── WorldBorder.cpp       # 世界边界实现，含 StationaryBorderState/MovingBorderState
└── README.md             # 本文件
```

## 内部模块关系

```
WorldBorder
    │
    ├── IBorderState（状态模式）
    │       ├── StationaryBorderState  // 静止边界，固定大小
    │       └── MovingBorderState      // 移动边界，线性插值过渡
    │
    └── IBorderListener（监听器）
            └── 用于网络同步，通知边界变化事件
```

**状态转换：**
- `setSize()` 创建静止状态
- `setSizeLerp()` 创建移动状态
- `tick()` 更新移动状态，过渡完成后转为静止状态

## 上下游外部依赖关系

**上游依赖（本模块依赖）：**
- `common/core/Types.hpp` - 基础类型定义
- `common/util/AxisAlignedBB.hpp` - AABB 碰撞检测
- `common/world/block/BlockPos.hpp` - 方块位置
- `common/world/chunk/ChunkPos.hpp` - 区块位置
- `common/world/WorldConstants.hpp` - 世界常量（CHUNK_WIDTH）

**下游依赖（被谁使用）：**
- `ServerWorld` - 持有 WorldBorder 实例，每 tick 调用 `border.tick()`
- `WorldBorderPacket` - 网络同步包，实现 `IBorderListener` 接口
- `WorldBorderCommand` - `/worldborder` 命令处理

## 边界参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| 初始大小 | 60,000,000 | 约 6000 万格 |
| 最大大小 | 29,999,872 | 约 3000 万格半径 |
| 伤害每格 | 0.2 | 越界每格伤害量 |
| 伤害缓冲 | 5.0 | 越界缓冲距离（不受伤） |
| 警告时间 | 15 秒 | 边界收缩前警告时间 |
| 警告距离 | 5 格 | 接近边界时警告距离 |

## 容易踩的坑

### 1. contains() 与 intersects() 语义不同

`contains(x, z)` 使用严格不等式（`>` 和 `<`），点在边界上返回 false。`intersects()` 使用非严格不等式，边界接触即返回 true。在玩家位置检测时，应使用 `contains()`；在区块加载判断时，应使用 `intersectsChunk()`。

### 2. 伤害计算需加 damageBuffer

伤害公式：`伤害 = max(1, floor(-(distance + damageBuffer) * damagePerBlock))`

其中 `distance` 是 `getClosestDistance()` 的返回值（边界内为正，边界外为负）。damageBuffer 是缓冲距离，玩家越界在 buffer 距离内不受伤。参考 MC 1.16.5 `LivingEntity.baseTick()` 第306-318行。

### 3. 监听器使用 weak_ptr

`addListener()` 接受 `shared_ptr` 但内部存储 `weak_ptr`，监听器生命周期由调用方管理。调用方必须确保监听器对象在 WorldBorder 生命周期内有效，并在适当时机调用 `removeListener()`。

### 4. MovingBorderState 使用缓存延迟计算

`MovingBorderState` 使用 `m_dirty` 标志和缓存变量延迟计算边界值，仅在需要时重新计算。调用 `tick()` 后会标记 dirty，下次查询时才更新缓存。不要在 tick 后立即假设缓存已更新。

### 5. 序列化需处理过渡状态

`serialize()` 和 `deserialize()` 支持过渡状态的保存和恢复。如果 `timeUntilTarget > 0` 且 `size != targetSize`，`deserialize()` 会恢复为移动状态。否则恢复为静止状态。

### 6. BlockPos contains 检测使用方块边界

`contains(const BlockPos&)` 重载检测方块是否完全在边界内，使用 `pos.x + 1.0` 和 `pos.z + 1.0` 作为方块最大边界。这与点检测 `contains(x, z)` 不同。
