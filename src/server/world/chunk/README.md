# 区块调度系统 (Chunk Scheduling System)

Moonrise 风格的区块调度系统，提供高性能、并行的区块生成和加载能力。

## 目录结构

```
src/server/world/chunk/
├── ChunkTaskScheduler.hpp/cpp      # 区块任务调度器
├── ChunkHolderManager.hpp/cpp      # 区块持有者管理器
└── task/
    └── ChunkProgressionTask.hpp/cpp # 区块进度任务基类和实现
```

## 核心组件

### ChunkTaskScheduler

区块任务调度器，管理多个执行器：

- **parallelGenExecutor**: 并行生成执行器，用于可并行执行的生成阶段
- **radiusAwareScheduler**: 半径感知调度器，用于需要邻居区块访问权限的任务
- **mainThreadExecutor**: 主线程任务队列，用于必须在主线程执行的任务

```cpp
ChunkTaskScheduler scheduler(world, 4);  // 4 个工作线程
scheduler.start();

// 调度任务
scheduler.scheduleChunkTask(x, z, []() {
    // 执行区块生成
}, Priority::NORMAL);

// 主线程执行
scheduler.executeMainThreadTask();
```

### ChunkHolderManager

区块持有者管理器，负责：

- 集中管理所有区块持有者 (SingleChunkLifecycleManager)
- 卸载队列管理
- 票据级别变化调度
- 与 ThreadedTicketLevelPropagator 集成

```cpp
ChunkHolderManager manager(world, propagator);

// 添加票据
manager.addTicket(x, z, 31, "player");

// 处理票据更新
manager.processTicketUpdates();

// 处理卸载队列
manager.processUnloadQueue(100);
```

### ChunkProgressionTask

区块进度任务基类：

- **ChunkUpgradeStatusTask**: 升级区块状态
- **ChunkLightTask**: 异步光照计算
- **ChunkFullTask**: 完成区块（主线程执行）

```cpp
ChunkUpgradeStatusTask task(scheduler, world, x, z, ChunkStatuses::FEATURES, ChunkStatuses::EMPTY);
task.setNeighbors(neighbors);
task.schedule();
task.addCompleteCallback([](ChunkPrimer* primer, const std::string& error) {
    // 处理完成
});
```

## ChunkStatus 并行配置

| 状态 | 写入半径 | 可并行 |
|------|---------|--------|
| EMPTY | 0 | ✓ |
| STRUCTURE_STARTS | 0 | ✓ |
| STRUCTURE_REFERENCES | 0 | ✓ |
| BIOMES | 0 | ✓ |
| NOISE | 0 | ✓ |
| SURFACE | 0 | ✓ |
| CARVERS | 0 | ✓ |
| LIQUID_CARVERS | 0 | ✓ |
| FEATURES | 1 | ✗ |
| LIGHT | 2 | ✗ |
| SPAWN | 0 | ✓ |
| HEIGHTMAPS | 0 | ✓ |
| FULL | 0 | ✗ |

## 优先级系统

```cpp
enum class Priority : i32 {
    BLOCKING = 0,   // 阻塞优先级（最高）
    HIGHEST = 1,
    HIGH = 2,
    NORMAL = 3,
    LOW = 4,
    LOWER = 5,
    LOWEST = 6,
    COUNT = 7
};
```

## 与 ThreadedTicketLevelPropagator 集成

票据级别传播使用 Section 分组（64x64 区块），支持高效的大规模区块加载：

```cpp
ThreadedTicketLevelPropagator propagator;

// 设置票据源
propagator.setSource(x, z, 31);  // 级别 31 = 完全加载

// 执行传播更新
propagator.performUpdate(sectionX, sectionZ, lock, updatedPositions);

// 获取级别
i32 level = propagator.getLevel(x, z);
```

## 区域锁 (ReentrantAreaLock)

用于并发控制，支持可重入锁定：

```cpp
ReentrantAreaLock lock(6);  // 64x64 区块为一个区域

// 锁定半径 2 的范围
auto* node = lock.lock(centerX, centerZ, 2);
// ... 执行操作 ...
lock.unlock(node);
```

## 使用示例

### 完整的区块生成流程

```cpp
// 1. 创建调度器
ChunkTaskScheduler scheduler(world);
scheduler.start();

// 2. 创建票据传播器和持有者管理器
ThreadedTicketLevelPropagator propagator;
ChunkHolderManager holderManager(world, propagator);

// 3. 添加票据触发加载
holderManager.addTicket(x, z, 31, "player");

// 4. 处理票据更新
holderManager.processTicketUpdates();

// 5. 获取或创建持有者
auto* holder = holderManager.getOrCreateChunkHolder(x, z);

// 6. 创建并调度任务
auto task = std::make_unique<ChunkUpgradeStatusTask>(
    scheduler, world, x, z, ChunkStatuses::FEATURES, ChunkStatuses::EMPTY);
task->schedule();

// 7. 主线程执行任务
scheduler.executeAllRecentlyQueuedMainThreadTasks();
```
