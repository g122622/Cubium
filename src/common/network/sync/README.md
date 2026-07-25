# 网络同步模块 (Network Sync Module)

本模块提供 Minecraft 服务端与客户端之间的区块同步功能，包括区块数据序列化、玩家视距管理和区块跟踪。

## 目录结构

```
src/common/network/sync/
├── Sync.hpp           # 统一头文件（便捷包含）
├── ChunkSync.hpp      # 区块同步相关类定义
└── ChunkSync.cpp      # 区块同步相关类实现
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────┐
│                 ChunkSyncManager                     │
│     (管理所有玩家的区块同步，维护订阅关系)            │
└──────────────────────┬──────────────────────────────┘
                       │ 管理
                       ▼
┌─────────────────────────────────────────────────────┐
│              PlayerChunkTracker                      │
│     (跟踪单个玩家已加载的区块和视距状态)              │
└──────────────────────┬──────────────────────────────┘
                       │ 包含
                       ▼
┌─────────────────────────────────────────────────────┐
│                  ChunkView                           │
│     (计算玩家视距范围内的区块)                        │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│               ChunkSerializer                        │
│     (区块数据二进制序列化/反序列化)                   │
└─────────────────────────────────────────────────────┘
```

**组件职责：**
- **ChunkSerializer**：区块数据的二进制序列化与反序列化，用于网络传输
- **ChunkView**：管理玩家的视距范围，计算哪些区块需要加载/卸载
- **PlayerChunkTracker**：跟踪单个玩家已加载的区块，管理玩家的视距状态
- **ChunkSyncManager**：管理所有玩家的区块同步，维护区块到玩家的订阅关系

## 上下游外部依赖关系

### 本模块依赖的外部模块

```cpp
#include "../../world/chunk/ChunkData.hpp"      // ChunkData, ChunkSection
#include "../../world/chunk/ChunkPos.hpp"       // ChunkPos, ChunkCoord, SectionPos
#include "../../world/chunk/ChunkId.hpp"        // ChunkId
#include "../codec/PacketSerializer.hpp"     // PacketSerializer, PacketDeserializer
#include "../../core/Result.hpp"                // Result<T>
```

间接依赖：`BiomeContainer`、`Block`/`BlockState`、`NibbleArray`

### 依赖本模块的外部模块

| 模块 | 使用方式 |
|------|----------|
| `server/sync/ChunkSendManager` | 使用序列化功能发送区块数据 |
| `server/core/PlayerManager` | 包含 `ChunkSyncManager` 实例管理玩家区块同步 |
| `server/core/PositionTracker` | 使用 `ChunkSyncManager` 追踪玩家位置 |
| `server/core/ServerPlayerData` | 持有 `PlayerChunkTracker` |
| `client/world/ClientWorld` | 使用反序列化功能接收区块数据 |

## 容易踩的坑

### 1. 视距范围限制

视距范围是 2-32，超出范围会被 clamp：
- `setViewDistance(1)` 实际设置为 2
- `setViewDistance(100)` 实际设置为 32

### 2. 区块坐标转换的负坐标处理

`blockToChunk` 使用 `floor`，负坐标向下取整：
- `blockToChunk(-0.1)` = -1（不是 0！）
- `blockToChunk(-16.0)` = -1
- `blockToChunk(-16.1)` = -2

### 3. 反序列化坐标验证

反序列化会验证坐标是否匹配。必须使用序列化时的坐标调用 `deserializeChunk`。

### 4. 区块订阅者管理

玩家离开时必须调用 `removeTracker` 或 `markChunkUnloaded` 清理订阅关系，否则会导致内存泄漏和悬垂引用。

### 5. 光照数据大小

序列化区块段时，光照数据固定占用 4096 字节（天空光照 2048 + 方块光照 2048）。

### 6. 线程安全

`ChunkSyncManager` 本身不是线程安全的，在多线程环境中使用时需要外部同步。

### 7. 空区块段处理

`calculateSectionMask` 只包含非空区块段。空段（所有方块都是空气）不会包含在位掩码中，反序列化时未设置的段不会创建。

### 8. 视距默认值

`ChunkView` 的默认视距使用 `world::CHUNK_LOAD_RADIUS` 常量，而非硬编码值。修改视距时需注意该常量的定义。

### 9. ChunkId 的维度字段

`ChunkId` 包含 `dimension` 字段（0=主世界, 1=下界, 2=末地），但 `PlayerChunkTracker` 和 `ChunkView` 主要处理二维区块坐标。跨维度场景需额外处理。
