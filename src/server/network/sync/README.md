# network/sync - 世界数据下推

负责把服务端的世界数据同步给客户端。每个 `ServerDimension` 实例各自持有一套独立的同步管理器，
确保多维度之间数据隔离。

本目录只做「什么时候、把什么、推给谁」，**不做**编解码——区块的二进制序列化在
`common/network/sync/ChunkSerializer`（客户端 `ClientWorld` 也用它反序列化，双向共用故留在 common）。

## 目录结构

```
src/server/network/sync/
├── README.md
├── ChunkSendManager.hpp/cpp       # 区块发送管理器（区块数据发送、卸载通知）
├── BlockUpdateSyncManager.hpp/cpp # 方块更新同步管理器（方块变化批量发送）
├── WeatherSyncService.hpp/cpp     # 天气同步服务（影子状态 + 主世界天气广播）
└── chunk/                         # 区块推送记账
    ├── ChunkView.hpp              # 正方形视距范围（isChunkInView / getChunksInView / calculateChunkDiff）
    ├── PlayerChunkTracker.hpp/cpp # 单玩家「已下发哪些区块」
    └── ChunkSyncManager.hpp/cpp   # 玩家 ↔ 区块双向索引（谁已收到 / 该区块该发给谁）
```

> 光照数据同步（`markLightChanged`/`_syncLightDataToChunk`）统一由 `ServerWorld` 承担，没有独立的
> `LightSyncManager`。区块加载光照由 `server/world/ChunkLoadLightTask` 在 worker 线程完成后，
> 经 `ServerWorld` 续延队列回主线程 flush + send。
>
> 实体同步/广播也没有独立的 `EntitySyncManager`：实体生成/销毁/移动/可见性由 `EntityTracker::tick`
> 独占，实体状态/动画/拴绳广播经 `ServerWorld` 的 `setOnBroadcastEntity*` 回调 +
> `../outbound/PlayerBroadcaster` 完成。

## 内部模块关系

```
                    ┌─────────────────────────────────────┐
                    │         ServerDimension             │
                    │   （每个维度独立持有以下管理器）        │
                    └──────────────┬──────────────────────┘
                                   │
              ┌────────────────────┴────────────────────┐
              ▼                                         ▼
┌───────────────────────┐                  ┌────────────────────────┐
│   ChunkSendManager    │                  │ BlockUpdateSyncManager │
│  └─ ChunkSyncManager  │                  │  （同 tick 去重后 flush）│
│       ├─ PlayerChunkTracker               └───────────┬────────────┘
│       └─ ChunkView    │                              │
└───────────┬───────────┘                              │
            │                                          │
            ▼                                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ServerWorld（光照 flush + send 顺序编排）                        │
└─────────────────────────────────────────────────────────────────┘
            │ 序列化经 common/network/sync/ChunkSerializer
            ▼
    ../outbound/  →  连接发送
```

**协作流程：**
- 区块加载完成 → `ChunkLoadLightTask`（worker）完成光照 → `ServerWorld` 主线程续延 flush →
  `ChunkSendManager` 发送给追踪该区块的玩家
- 方块变化 → `BlockUpdateSyncManager` 缓存 → tick 末 flush 发送
- 实体同步 → `EntityTracker::tick` + `ServerWorld` 广播回调
- 天气变化 → `WeatherSyncService` 比对影子状态 → 超阈值广播

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块 | 用途 |
|---|---|
| `common/network/sync/ChunkSerializer.hpp` | 区块二进制序列化 |
| `common/network/sync/VanillaChunkWire.hpp` | IR ↔ Java `LevelChunkWithLight` 翻译 |
| `common/world/chunk/ChunkData.hpp` | 区块数据结构 |
| `common/world/chunk/base/{ChunkId,ChunkPos}.hpp` | 区块标识与坐标 |
| `common/world/WorldConstants.hpp` | `CHUNK_LOAD_RADIUS` 等 |
| `common/util/thread/UniversalWorkerPool.hpp` | 序列化任务的 worker 池 |
| `server/world/ServerWorld.hpp`、`server/world/ServerChunkManager.hpp` | 世界与区块状态 |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|---|---|
| `server/dimension/ServerDimension` | 创建并持有各同步管理器实例 |
| `server/world/ServerWorld` | 光照 flush/send 编排、方块变化事件源、地图数据 |
| `server/application/MinecraftServer` | 设置网络发送回调 |
| `server/core/{PlayerManager,ServerPlayerData}` | 持 `chunk/ChunkSyncManager` 与 `chunk/PlayerChunkTracker` |
| `server/network/handshake/LoginFlow` | 经 `WeatherSyncService` 推初始天气 |

## 容易踩的坑

### 1. `ChunkSendManager` 的序列化在 Worker 线程，不能直接发包

区块序列化全部在 Worker 线程执行（主路径提交 `FunctionTask` 到 ServerCompute 池，异步加载回调路径
本就在 worker），**不能直接调用网络发送**。两条路径都通过 `submitChunkData()` 提交到加锁队列，
主线程通过 `processPendingSends()` 处理。

主路径经 `tryToGetChunkSharedInMem` 拷贝 `shared_ptr<ChunkData>` 捕获保活——worker 在途期间即使主线程
卸载区块，引用计数维持存活，防 UAF。任务以 `submit(writeRadius=0)` 提交，与
`RuntimeLightTask(writeRadius=2)` 区域互斥串行，保证 serialize 读 `ChunkSection` nibble 不与光照写竞争。

### 2. 回调未设置会静默不发

`setOnChunkSend` 等回调未设置时，区块数据会被序列化但不会发送。必须在
`MinecraftServer::setupWorldCallbacks()` 中为每个维度设置全部回调。

### 3. 区块卸载顺序

区块卸载前必须先发送卸载通知，否则客户端会看到画面闪烁。在
`ServerChunkManager::checkChunkUnloading()` 中调用 `onChunkPreUnload()`，然后再执行卸载。

### 4. 光照 flush → send → drain 顺序严格

`ServerWorld::tick` 顺序：先 `_drainPendingLightFlushes`（把 visible nibble 同步进 `ChunkSection`），
再 `_drainPendingChunkSends`（提交 serialize 任务），最后 `processPendingSends` drain 真正发包。
顺序颠倒或 serialize 读到未 flush 的 nibble，客户端会收到**全黑区块**。

serialize 异步化后区块包发送延迟 ≤1 tick（约 50ms）。

### 5. 方块更新不要直接发包

在 `ServerWorld`、`IntegratedServer` 或 `StandaloneServer` 中直接发 `ir::play::BlockUpdate` 会绕过去重
与统一 flush。只让 `ServerWorld::setOnBlockChanged()` 产出事件，由 `BlockUpdateSyncManager` 统一处理。

### 6. `sendChunkToPlayers` 的 `validateTracking` 参数

当区块可能在序列化期间被卸载时，必须传 `validateTracking=true`：发送前过滤掉已不追踪该区块的玩家，
避免"幽灵区块"。

### 7. `chunk/` 的记账是服务端专属，序列化不是

`chunk/` 下的三个类型只被 server 使用（`PlayerManager`/`ServerPlayerData`/`ChunkSendManager`），
命名空间为 `mc::server::sync`。而 `ChunkSerializer` 被客户端 `ClientWorld::deserializeChunk` 使用，
**必须留在 `common/network/sync/`**——拆分时不要把它一并迁过来。

### 8. `ChunkSyncManager` 不是线程安全的

多线程环境下需外部同步。用户的加入/移动/断开都应在主线程串行处理。

### 9. `ChunkView` 的视距默认值来自常量

`ChunkView::viewDistance` 默认取 `world::CHUNK_LOAD_RADIUS`，不要硬编码。改动视距时注意该常量。
