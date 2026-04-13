# Chunk 模块

本目录包含 Minecraft 区块系统的核心实现，参考 Minecraft Java Edition 1.16.5 和 Moonrise 的架构设计。

## 目录结构

```
src/common/world/chunk/
├── ChunkData.hpp/cpp              # 区块数据存储
├── ChunkTrackingManager.hpp/cpp   # 玩家区块追踪管理器
├── PlayerChunkTracker.hpp/cpp     # 单玩家区块追踪器
├── ChunkLoadTicket.hpp            # 区块加载票据类型定义
├── ChunkPos.hpp                   # 区块/段位置类型
├── ChunkPrimer.hpp/cpp            # 区块生成中间状态
├── ChunkStatus.hpp/cpp            # 区块生成阶段定义
├── IChunk.hpp/cpp                 # 区块接口和基础类型
├── SingleChunkLifecycleManager.hpp/cpp  # 单区块生命周期管理
├── ThreadedTicketLevelPropagator.hpp/cpp  # Section 分组票据传播器
└── ReentrantAreaLock.hpp/cpp      # 可重入区域锁
```

## 核心组件

### 票据系统架构

票据系统分为两层：

1. **ThreadedTicketLevelPropagator** - 高性能票据级别传播
   - Section 分组（64x64 区块为一个 Section）
   - 增加/减少传播分离队列
   - 与 ReentrantAreaLock 集成

2. **ChunkTrackingManager** - 玩家区块追踪
   - 管理玩家视距内的区块集合
   - 提供追踪变化回调
   - 用于区块发送系统确定目标玩家

### PlayerChunkTracker

**职责**：追踪单个玩家的视距范围区块。

**主要内容**：
- 维护玩家当前位置和视距
- 计算视距范围内的区块集合
- 提供进入/离开回调
- 切比雪夫距离计算

**使用示例**：
```cpp
PlayerChunkTracker tracker(10);  // 视距 10

// 设置玩家位置，触发回调
tracker.setPlayerPosition(5, 10,
    [](ChunkCoord x, ChunkCoord z, bool isTracking) {
        // 区块进入视距
    },
    [](ChunkCoord x, ChunkCoord z, bool isTracking) {
        // 区块离开视距
    });

// 检查区块是否在视距内
bool inRange = tracker.isChunkInRange(8, 12);

// 获取距离
i32 dist = tracker.getDistanceToPlayer(8, 12);
```

---

### ChunkTrackingManager

**职责**：管理所有玩家的区块追踪关系。

**主要内容**：
- 玩家追踪器映射
- 区块到玩家的反向映射
- 追踪变化回调
- 线程安全的查询接口

**使用示例**：
```cpp
ChunkTrackingManager manager;

// 设置追踪变化回调
manager.setTrackingChangeCallback(
    [](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
        // 玩家进入/离开区块视距
    });

// 更新玩家位置
manager.updatePlayerPosition(playerId, chunkX, chunkZ);

// 查询追踪某区块的玩家
auto players = manager.getTrackingPlayers(x, z);

// 移除玩家
manager.removePlayer(playerId);
```

---

### ThreadedTicketLevelPropagator

**职责**：高效的票据级别传播，参考 Moonrise 架构。

**核心特性**：
- Section 分组（64x64 区块）减少内存使用
- 增加/减少传播分离队列
- 区域锁集成
- 级别变化回调

**票据级别**：
- 级别越小优先级越高
- 级别 1-31 用于玩家加载
- 级别 31 = FULL（完整区块）
- 级别 32 = ENTITY_TICKING
- 级别 33 = BORDER
- 级别 34+ 表示未加载

---

### SingleChunkLifecycleManager

**职责**：管理单个区块的加载状态和生成进度。

**主要内容**：
- 状态消费者回调
- 完整区块状态回调
- 优先级管理
- Future 管理
- 票据管理

---

## 与服务端集成

服务端使用以下架构：

```
ServerChunkManager
├── ThreadedTicketLevelPropagator  // 票据级别传播
├── ChunkHolderManager             // 区块持有者管理
│   └── SingleChunkLifecycleManager[]  // 单区块生命周期
├── ChunkTrackingManager           // 玩家追踪管理
│   └── PlayerChunkTracker[]       // 单玩家追踪
└── ChunkTaskScheduler             // 区块任务调度
```

**票据流**：
1. `ServerChunkManager::updatePlayerPosition()` → `ChunkHolderManager::addTicket()`
2. `ChunkHolderManager::addTicket()` → `ThreadedTicketLevelPropagator::setSource()`
3. `ThreadedTicketLevelPropagator::performUpdate()` → 级别传播
4. 回调 `onTicketLevelChanged()` → 加载/卸载决策

**追踪流**：
1. `ServerChunkManager::updatePlayerPosition()` → `ChunkTrackingManager::updatePlayerPosition()`
2. `ChunkTrackingManager` 触发追踪变化回调
3. `ChunkSendManager::onPlayerTrackingChange()` → 发送/卸载区块

---

## 测试文件

- `tests/common/test_chunk_tracking.cpp` - PlayerChunkTracker 和 ChunkTrackingManager 测试
- `tests/common/world/chunk/test_ticket_level_propagator.cpp` - ThreadedTicketLevelPropagator 测试
- `tests/server/world/chunk/test_chunk_task_scheduler.cpp` - ChunkTaskScheduler 测试
