# network/sync/chunk - 区块推送记账

回答两个问题：**某个玩家已经收到哪些区块**、**某个区块该发给哪些玩家**。
纯记账，不含序列化、不含发包——序列化在 `common/network/sync/ChunkSerializer`，
发包由上一层 `ChunkSendManager` 驱动。

本目录的类型只被 server 使用，命名空间为 `mc::server::sync`。

## 目录结构

```
src/server/network/sync/chunk/
├── README.md
├── ChunkView.hpp              # 以某坐标为中心的正方形区块视距范围
├── PlayerChunkTracker.hpp/cpp # 单玩家的「已下发区块」集合 + 自身视距视图
└── ChunkSyncManager.hpp/cpp   # 全体玩家的双向索引：玩家→区块、区块→订阅玩家
```

三层是单向依赖：`ChunkSyncManager` → `PlayerChunkTracker` → `ChunkView`。

- `ChunkView`：`isChunkInView` / `getChunksInView` / `calculateChunkDiff`（算出该加载/卸载哪些区块）。
- `PlayerChunkTracker`：`m_loadedChunks`（`unordered_set<ChunkId>`）+ 一个 `ChunkView`。
- `ChunkSyncManager`：`m_trackers`（`PlayerId → shared_ptr<PlayerChunkTracker>`）与
  `m_chunkSubscribers`（`ChunkId → unordered_set<PlayerId>`）双向索引，缺一不可——前者算某玩家的推送
  差集，后者在区块卸载时反查需要通知的玩家。

## 上下游外部依赖关系

| 依赖 | 用途 |
|---|---|
| `common/world/chunk/base/ChunkId.hpp`、`ChunkPos.hpp` | 区块标识与坐标 |
| `common/world/WorldConstants.hpp` | `CHUNK_LOAD_RADIUS`、`CHUNK_WIDTH` |

| 依赖本目录 | 用途 |
|---|---|
| `server/core/PlayerManager` | 持 `ChunkSyncManager`，驱动 `updatePlayerPosition`/`calculateUpdates` |
| `server/core/ServerPlayerData` | 持 `shared_ptr<PlayerChunkTracker>` |
| `server/network/sync/ChunkSendManager` | 经 `markChunkSent`/`markChunkUnloaded`/`getChunkSubscribers` 记账 |

## 容易踩的坑

### 1. 视距是正方形不是圆形

`ChunkView::getChunksInView()` 返回以中心为原点、半径 `viewDistance` 的**正方形**，
区块数量为 `(2n+1)²`。按圆形理解会算错差集。

### 2. `setViewDistance` 会夹取到 [2, 32]

`PlayerChunkTracker::setViewDistance` 内部 `std::clamp(distance, 2, 32)`。传入 0 或 64 会被静默改写，
测试断言视距时要注意。

### 3. `ChunkId` 的 dimension 恒为 0

本目录的三个类型只处理二维区块坐标，构造 `ChunkId` 时 dimension 一律填 0。跨维度场景由上层
按维度各持一套 `ChunkSyncManager` 来解决，不要试图在这里编码维度。

### 4. 双向索引必须同步维护

`markChunkSent` / `markChunkUnloaded` 同时改 `PlayerChunkTracker` 与 `m_chunkSubscribers`；
`removeTracker` 会把该玩家的全部区块从订阅索引里摘掉。若绕过这三个方法直接改其中一个索引，
会留下"订阅者已离线却仍在列表里"的悬垂记账。

### 5. 非线程安全

`ChunkSyncManager` 无锁。玩家的加入 / 移动 / 断开必须在主线程串行处理。
