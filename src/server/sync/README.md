# Server Sync 模块

服务端同步模块，负责将服务端的世界数据同步给客户端。

## 目录结构

```
src/server/sync/
├── ChunkSendManager.hpp    # 区块发送管理器头文件
├── ChunkSendManager.cpp    # 区块发送管理器实现
├── EntitySyncManager.hpp   # 实体同步管理器头文件
├── EntitySyncManager.cpp   # 实体同步管理器实现
├── LightSyncManager.hpp    # 光照同步管理器头文件
└── LightSyncManager.cpp    # 光照同步管理器实现
```

## 文件详解

### ChunkSendManager.hpp/cpp

区块发送管理器，负责将区块数据发送给客户端。

#### 职责

- 区块加载完成时自动发送给追踪该区块的玩家
- 玩家追踪变化时发送/卸载区块
- 区块卸载前发送卸载通知
- 异步区块序列化与主线程发送解耦

#### 与 ChunkLoadTicketManager 协同工作

```
玩家移动 → PlayerChunkTracker 计算新旧区块集合
         → ChunkLoadTicketManager 触发追踪变化回调
         → ChunkSendManager.onPlayerTrackingChange()
             → 区块已加载：立即发送
             → 区块未加载：等待加载完成后发送
```

#### 关键方法

| 方法 | 说明 |
|------|------|
| `sendChunkToPlayers(x, z, players, validateTracking=false)` | 发送区块给指定玩家列表，可选发送前校验追踪状态 |
| `sendChunkToTrackingPlayers(x, z)` | 发送区块给所有追踪该区块的玩家 |
| `unloadChunkFromPlayers(x, z, players)` | 发送卸载通知给指定玩家列表 |
| `unloadChunkFromTrackingPlayers(x, z)` | 发送卸载通知给所有追踪玩家 |
| `onPlayerTrackingChange(player, x, z, isTracking)` | 处理玩家追踪变化 |
| `onChunkPreUnload(x, z)` | 区块卸载前的处理 |
| `submitChunkData(x, z, data, players, validateTracking=false)` | 从 Worker 线程提交序列化数据（线程安全） |
| `processPendingSends()` | 主线程处理待发送队列 |
| `setOnChunkSend(callback)` | 设置区块发送回调 |
| `setOnChunkUnload(callback)` | 设置区块卸载回调 |

#### 线程安全设计

- 区块序列化在 Worker 线程执行
- 序列化完成后通过 `submitChunkData()` 提交到队列
- 主线程通过 `processPendingSends()` 处理队列并发送
- 当 `validateTracking=true` 时，发送前会调用 `isPlayerTracking()` 过滤过期目标，避免“先卸载后晚到加载包”导致客户端重现幽灵区块

---

### EntitySyncManager.hpp/cpp

实体同步管理器，负责实体位置的客户端同步。

#### 职责

- 追踪实体位置变化
- 发送实体生成/移动/销毁包
- 多玩家可见性管理
- 位置变化阈值检测

#### 关键方法

| 方法 | 说明 |
|------|------|
| `tick()` | 每 tick 同步实体位置 |
| `spawnEntity(entity)` | 生成新实体并通知客户端 |
| `removeEntity(entityId)` | 移除实体并通知客户端 |
| `forceFullUpdate(entityId)` | 强制发送完整更新 |
| `setOnEntitySpawn(callback)` | 设置实体生成回调 |
| `setOnEntityRemove(callback)` | 设置实体移除回调 |
| `setOnEntityMove(callback)` | 设置实体移动回调 |

#### 位置变化检测

```cpp
static constexpr f32 POSITION_THRESHOLD = 0.01f;  // 位置变化阈值
static constexpr f32 ROTATION_THRESHOLD = 1.0f;   // 旋转变化阈值（度）
```

只有超过阈值的变化才会触发同步，减少网络带宽。

---

### LightSyncManager.hpp/cpp

光照同步管理器，负责将光照数据从 WorldLightManager 同步到 ChunkSection。

#### 职责

- 区块加载后初始化光照
- 方块变化时触发光照检查
- 同步光照数据到 ChunkSection

#### 关键方法

| 方法 | 说明 |
|------|------|
| `initializeChunkLighting(x, z)` | 区块加载后初始化光照 |
| `onBlockStateChanged(x, y, z, oldLevel, newLevel)` | 方块变化时触发光照检查 |
| `markLightChanged(type, pos)` | 标记光照变化，同步数据 |
| `syncLightDataToChunk(type, pos)` | 同步光照数据到 ChunkSection |

#### 光照同步流程

```
区块加载完成 → initializeChunkLighting()
             → 遍历所有 ChunkSection
             → 更新 SectionStatus
             → 复制光照数据到 WorldLightManager
             → 启用光源

方块变化 → onBlockStateChanged()
         → WorldLightManager.checkBlock()
         → 如果发光增加：onBlockEmissionIncrease()

光照变化 → markLightChanged()
         → 标记区块为脏
         → syncLightDataToChunk()
         → 从 WorldLightManager 获取光照数据
         → 写入 ChunkSection 的 NibbleArray
```

## 模块整体设计

### 整体职责

sync 模块是服务端数据同步的核心模块，负责将世界数据（区块、实体、光照）同步给客户端。主要职责包括：

1. **区块同步**：管理区块的发送、卸载，与玩家追踪系统集成
2. **实体同步**：追踪实体状态变化，广播给相关玩家
3. **光照同步**：维护光照数据的一致性，确保客户端渲染正确

### 输入和输出

#### 输入

| 数据源 | 数据类型 | 说明 |
|--------|----------|------|
| ServerChunkManager | ChunkData | 区块数据 |
| ChunkLoadTicketManager | 追踪玩家列表 | 区块→玩家映射 |
| EntityManager | Entity 实体 | 实体数据 |
| WorldLightManager | SWMRNibbleArray | 光照数据 |
| 回调函数 | 网络发送 | 由 MinecraftServer 设置 |

#### 输出

| 输出类型 | 说明 |
|----------|------|
| 区块数据包 (ChunkDataPacket) | 发送给客户端的区块数据 |
| 卸载区块包 (UnloadChunkPacket) | 通知客户端卸载区块 |
| 实体生成包 (SpawnEntityPacket) | 实体生成通知 |
| 实体移动包 (EntityMovePacket) | 实体位置更新 |
| 实体移除包 (DestroyEntityPacket) | 实体移除通知 |
| 光照数据 | 写入 ChunkSection |

### 依赖项

```
sync/
├── common/
│   ├── core/Types.hpp
│   ├── world/
│   │   ├── chunk/ChunkData.hpp
│   │   ├── chunk/ChunkPos.hpp
│   │   ├── chunk/ChunkLoadTicketManager.hpp
│   │   ├── lighting/LightType.hpp
│   │   ├── lighting/manager/WorldLightManager.hpp
│   │   ├── lighting/storage/SWMRNibbleArray.hpp
│   │   └── entity/EntityManager.hpp
│   ├── network/sync/ChunkSync.hpp
│   └── util/math/Vector3.hpp
└── server/
    └── world/ServerChunkManager.hpp
```

### 使用方法

sync 模块的管理器由 MinecraftServer 创建和管理：

```cpp
// MinecraftServer.hpp
class MinecraftServer : public IServer {
    // ...
    std::unique_ptr<sync::EntitySyncManager> m_entitySyncManager;
    std::unique_ptr<sync::ChunkSendManager> m_chunkSendManager;
    std::unique_ptr<sync::LightSyncManager> m_lightSyncManager;
    // ...
};

// 初始化（在 MinecraftServer::initializeSyncManagers() 中）
void MinecraftServer::initializeSyncManagers() {
    m_entitySyncManager = std::make_unique<sync::EntitySyncManager>(entityManager());
}

// 初始化区块同步（在 MinecraftServer::initializeChunkSyncManagers() 中）
void MinecraftServer::initializeChunkSyncManagers() {
    m_chunkSendManager = std::make_unique<sync::ChunkSendManager>(
        chunkManager(), chunkManager().ticketManager());
    m_lightSyncManager = std::make_unique<sync::LightSyncManager>(
        *m_lightManager, chunkManager());
}

// 设置回调（在 MinecraftServer::setupWorldCallbacks() 中）
void MinecraftServer::setupWorldCallbacks() {
    // 区块加载完成回调
    chunkManager().setChunkLoadedCallback([this](ChunkCoord x, ChunkCoord z) {
        lightSyncManager().initializeChunkLighting(x, z);
        chunkSendManager().sendChunkToTrackingPlayers(x, z);
    });
    
    // 追踪变化回调
    chunkManager().ticketManager().setTrackingChangeCallback(
        [this](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
            chunkSendManager().onPlayerTrackingChange(player, x, z, isTracking);
        });
}

// 主循环调用
void MinecraftServer::tick() {
    // ...
    m_chunkSendManager->processPendingSends();  // 处理待发送区块
    m_entitySyncManager->tick();                 // 同步实体位置
    // ...
}
```

### 容易踩的坑

#### 1. 线程安全问题

**问题描述**：ChunkSendManager 的区块序列化在 Worker 线程执行，直接调用网络发送会导致线程安全问题。

**解决方案**：使用 `submitChunkData()` 提交到队列，主线程通过 `processPendingSends()` 处理。

```cpp
// 错误：在 Worker 线程直接发送
void ChunkSendManager::sendChunkToPlayers(...) {
    // 不要在这里直接调用 m_onChunkSend！
}

// 正确：提交到队列
void ChunkSendManager::submitChunkData(ChunkCoord x, ChunkCoord z, 
                                        std::vector<u8> data, 
                                        std::vector<PlayerId> players) {
    std::lock_guard<std::mutex> lock(m_readyChunksMutex);
    m_readyChunks.emplace_back(x, z, std::move(data), std::move(players));
}
```

#### 2. 回调未设置

**问题描述**：如果 `setOnChunkSend` 等回调未设置，区块数据会被序列化但不会发送。

**解决方案**：在 MinecraftServer 初始化时设置所有回调。

```cpp
m_chunkSendManager->setOnChunkSend([this](PlayerId playerId, ChunkCoord x, ChunkCoord z, 
                                           const std::vector<u8>& data) {
    sendChunkDataPacket(playerId, x, z, data);
});
```

#### 3. 区块卸载顺序

**问题描述**：如果在区块卸载后才发送卸载通知，客户端可能会看到短暂的画面闪烁。

**解决方案**：在 ServerChunkManager::checkChunkUnloading() 中调用 `onChunkPreUnload()`，在区块卸载前发送通知。

```cpp
// ServerChunkManager.cpp
if (!holder->shouldLoad() && !ticketManager.hasTrackingPlayers(key)) {
    chunkSendManager->onChunkPreUnload(x, z);  // 先通知客户端
    unloadChunk(x, z);                          // 再卸载
}
```

#### 4. 光照数据同步时机

**问题描述**：如果在 WorldLightManager 计算完成前同步光照数据，客户端会看到错误的照明。

**解决方案**：确保光照引擎计算完成后再调用 `syncLightDataToChunk()`。

#### 5. 实体位置阈值过小

**问题描述**：如果位置变化阈值设置过小，会导致频繁发送位置更新，增加网络带宽。

**解决方案**：使用合理的阈值（位置 0.01，旋转 1 度），并在需要强制同步时使用 `forceFullUpdate()`。

### 涉及的测试用例

#### common/network/sync/ChunkSync 测试 (test_chunksync.cpp)

测试区块同步相关的数据结构和方法：

- **ChunkView 测试**：区块视距计算、区块差异计算
- **PlayerChunkTracker 测试**：玩家区块追踪、位置更新、视距变化
- **ChunkSyncManager 测试**：多玩家追踪、区块订阅、玩家断开连接清理
- **ChunkSerializer 测试**：区块序列化/反序列化、光照数据保留

#### server/LightSyncTests.cpp

测试光照同步相关功能：

- **NibbleArrayCopy**：NibbleArray 复制功能
- **ChunkSectionLightAccess**：ChunkSection 光照数组访问
- **ChunkSectionLightFill**：ChunkSection 光照填充
- **ChunkDataLightAccess**：ChunkData 光照访问
- **ChunkSectionSerializePreservesLight**：序列化保留光照数据
- **SectionPosEncodeDecode**：SectionPos 编码解码
- **WorldLightManagerCreation**：WorldLightManager 创建
- **WorldLightManagerTickWithoutWorkReturnsBudget**：无光照任务时 tick 返回预算
- **BlockLightStorageAppliesPendingSectionData**：光照存储应用待处理数据
- **BlockLightStorageSetLightMarksNeighborSections**：设置光照标记相邻区块段

#### 测试数量统计

| 测试文件 | 测试用例数 |
|----------|-----------|
| test_chunksync.cpp | 45+ |
| LightSyncTests.cpp | 15 |
| **总计** | **60+** |

## 数据流图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              MinecraftServer                                 │
│                                                                             │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐         │
│  │ PlayerManager   │    │ ServerChunk     │    │ EntityManager   │         │
│  │                 │    │ Manager         │    │                 │         │
│  └────────┬────────┘    └────────┬────────┘    └────────┬────────┘         │
│           │                      │                      │                   │
│           │ 玩家位置更新          │ 区块加载/卸载         │ 实体生成/移动      │
│           ▼                      ▼                      ▼                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         sync 模块                                    │   │
│  │                                                                     │   │
│  │  ┌───────────────────┐                                           │   │
│  │  │ ChunkSendManager  │◄──── ChunkLoadTicketManager (追踪玩家)     │   │
│  │  │                   │                                           │   │
│  │  │ - sendChunk       │                                           │   │
│  │  │ - unloadChunk     │                                           │   │
│  │  │ - processPending  │                                           │   │
│  │  └─────────┬─────────┘                                           │   │
│  │            │                                                      │   │
│  │            │ 区块数据包                                           │   │
│  │            ▼                                                      │   │
│  │  ┌───────────────────┐    ┌───────────────────┐                  │   │
│  │  │ EntitySyncManager │    │ LightSyncManager  │                  │   │
│  │  │                   │    │                   │                  │   │
│  │  │ - tick()          │    │ - initLighting    │                  │   │
│  │  │ - spawnEntity     │    │ - markChanged     │                  │   │
│  │  │ - removeEntity    │    │ - syncToChunk     │                  │   │
│  │  └─────────┬─────────┘    └─────────┬─────────┘                  │   │
│  │            │                        │                             │   │
│  │            │ 实体数据包              │ 光照数据                    │   │
│  │            ▼                        ▼                             │   │
│  └────────────┼────────────────────────┼─────────────────────────────┘   │
│               │                        │                                  │
│               ▼                        ▼                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         网络层                                       │   │
│  │                                                                     │   │
│  │  ChunkDataPacket, UnloadChunkPacket, SpawnEntityPacket, etc.       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
                              ┌─────────────────┐
                              │     Client      │
                              └─────────────────┘
```

## 初始化顺序

sync 模块的初始化有严格的顺序要求：

```
1. initializeSyncManagers()
   └── 创建 EntitySyncManager（仅依赖 EntityManager）

2. initializeWorld()
   └── 创建 ServerWorld
   └── 创建 ServerChunkManager
   └── 创建 WorldLightManager

3. initializeChunkSyncManagers()
   └── 创建 ChunkSendManager（依赖 ServerChunkManager + ChunkLoadTicketManager）
   └── 创建 LightSyncManager（依赖 WorldLightManager + ServerChunkManager）

4. setupWorldCallbacks()
   └── 设置区块加载回调 → LightSyncManager.initializeChunkLighting()
                        → ChunkSendManager.sendChunkToTrackingPlayers()
   └── 设置追踪变化回调 → ChunkSendManager.onPlayerTrackingChange()
   └── 设置光照变化回调 → (网络发送光照更新)
```

## 性能考虑

1. **区块序列化**：在 Worker 线程异步执行，避免阻塞主线程
2. **位置阈值检测**：只有超过阈值才发送更新，减少网络带宽
3. **追踪玩家列表**：通过 ChunkLoadTicketManager 高效查询，避免遍历所有玩家
4. **光照同步时机**：仅在光照变化时同步，避免每帧同步
