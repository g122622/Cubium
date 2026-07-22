# Server Sync 模块

服务端同步模块，负责将服务端的世界数据同步给客户端。每个 `ServerDimension` 实例各自持有一套独立的同步管理器，确保多维度之间数据隔离。

## 目录结构

```
src/server/sync/
├── ChunkSendManager.hpp/cpp       # 区块发送管理器（区块数据发送、卸载通知）
├── EntitySyncManager.hpp/cpp      # 实体同步管理器（位置追踪、生成/移动/销毁包）
└── BlockUpdateSyncManager.hpp/cpp # 方块更新同步管理器（方块变化批量发送）
```

> 光照数据同步（`markLightChanged`/`_syncLightDataToChunk`）已统一由 `ServerWorld` 承担，
> 不再有独立的 `LightSyncManager`。区块加载光照由 `server/world/ChunkLoadLightTask` 在
> worker 线程完成后，经 `ServerWorld` 续延队列回主线程 flush + send（见
> `server/world/README.md` 光照章节）。

## 内部模块关系

```
                    ┌─────────────────────────────────────┐
                    │         ServerDimension             │
                    │   （每个维度独立持有以下管理器）        │
                    └──────────────┬──────────────────────┘
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
        ▼                          ▼                          ▼
┌───────────────┐       ┌───────────────────┐       ┌─────────────────┐
│ChunkSendManager│◄──────│BlockUpdateSync    │       │EntitySyncManager│
│               │ 追踪  │Manager            │       │                 │
└───────┬───────┘玩家  └─────────┬─────────┘       └────────┬────────┘
        │                          │                          │
        ▼                          ▼                          ▼
┌─────────────────┐                                ┌─────────────────┐
│  ServerWorld    │                                │  网络层回调      │
│ (光照 flush+send)│                                │ (发送数据包)     │
└─────────────────┘                                └─────────────────┘
```

**协作流程：**
- 区块加载完成 → `ChunkLoadLightTask`（worker）完成光照 → `ServerWorld` 主线程续延 flush + `ChunkSendManager` 发送给追踪玩家
- 方块变化 → `BlockUpdateSyncManager` 缓存 → tick 末 flush 发送
- 实体移动 → `EntitySyncManager` 检测阈值 → 发送位置更新

## 上下游外部依赖关系

### 上游依赖（本模块依赖的模块）

| 模块 | 用途 |
|------|------|
| `common/world/chunk/ChunkData.hpp` | 区块数据结构 |
| `common/world/chunk/ChunkLoadTicketManager.hpp` | 玩家追踪信息 |
| `common/world/lighting/manager/WorldLightManager.hpp` | 光照计算 |
| `common/world/entity/EntityManager.hpp` | 实体管理 |
| `server/world/ServerWorld.hpp` | 服务端世界 |

### 下游依赖（依赖本模块的模块）

| 模块 | 用途 |
|------|------|
| `server/world/ServerDimension.hpp` | 创建和管理同步管理器实例 |
| `server/MinecraftServer.hpp` | 设置网络发送回调 |

## 容易踩的坑

### 1. 线程安全问题

`ChunkSendManager` 的区块序列化**全部在 Worker 线程执行**（主路径提交 `FunctionTask` 到 ServerCompute 池，异步加载回调路径本就在 worker），**不能直接调用网络发送**。两条路径都通过 `submitChunkData()` 提交到加锁队列，主线程通过 `processPendingSends()` 处理。

主路径（区块已在内存）经 `tryToGetChunkSharedInMem` 拷贝 `shared_ptr<ChunkData>` 捕获保活——worker 在途期间即使主线程卸载区块，引用计数维持存活，防 UAF。任务以 `submit(writeRadius=0)` 提交，与 `RuntimeLightTask(writeRadius=2)` 区域互斥串行，保证 serialize 读 `ChunkSection` nibble 不与光照写竞争。`radiusAwareExecutor()` 返回 nullptr（测试/启动早期）时走同步 fallback。

### 2. 回调未设置

如果 `setOnChunkSend` 等回调未设置，区块数据会被序列化但不会发送。必须在 `MinecraftServer::setupWorldCallbacks()` 中为每个维度设置所有回调。

### 3. 区块卸载顺序

区块卸载前必须先发送卸载通知，否则客户端会看到画面闪烁。在 `ServerChunkManager::checkChunkUnloading()` 中调用 `onChunkPreUnload()`，然后再执行卸载。

### 4. 光照数据同步时机

区块加载光照由 `ChunkLoadLightTask` 在 worker 线程完成，dirty section 经 `ServerWorld::_enqueueLightFlush` 入主线程 flush 队列。`ServerWorld::tick` 顺序严格 **flush → send → drain**：先 `_drainPendingLightFlushes`（`_syncLightDataToChunk` 把 visible nibble 同步到 ChunkSection），再 `_drainPendingChunkSends`（提交 serialize 任务，本 tick nibble 写已完成），最后 `processPendingSends` drain `m_readyChunks` 真正发包。

serialize 改异步后，区块包发送延迟 ≤1 tick（~50ms）：`_drainPendingChunkSends` 仅提交 serialize 任务入 `m_readyChunks`，下一 tick `processPendingSends` 才 drain 发包。顺序颠倒或 serialize 读到未 flush 的 nibble 会导致客户端收到全黑区块。

### 5. 方块更新不要直接发包

在 `ServerWorld`、`IntegratedServer` 或 `StandaloneServer` 中直接发送 `BlockUpdatePacket` 会绕过去重和统一 flush。只让 `ServerWorld::setOnBlockChanged()` 产出事件，由 `BlockUpdateSyncManager` 统一处理。

### 6. 区块发送时 validateTracking 参数

当区块可能在序列化期间被卸载时，调用 `sendChunkToPlayers` 时传入 `validateTracking=true`，发送前会过滤掉已不追踪该区块的玩家，避免"幽灵区块"问题。
