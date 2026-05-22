# Server Dimension 模块

服务端维度管理，负责多维度实例、玩家维度追踪、维度切换等功能。

## 目录结构

```
server/dimension/
├── ServerDimension.hpp         # 服务端维度实例
├── ServerDimension.cpp         # 服务端维度实现
├── ServerDimensionManager.hpp  # 服务端维度管理器
├── ServerDimensionManager.cpp  # 服务端维度管理器实现
└── README.md                   # 本文档
```

## 文件详解

### ServerDimension.hpp/cpp

**职责**: 服务端维度实例，继承 `Dimension` 基类，添加服务端特有功能。

**主要功能**:
- 持有单个维度的 `ServerWorld` runtime
- 通过 `world()` 转发访问 `ServerChunkManager`
- 通过 `world()` 转发访问 `WorldLightManager`
- 持有维度级同步管理器（EntitySyncManager、ChunkSendManager、BlockUpdateSyncManager、LightSyncManager）
- 持有维度级刷怪管理器（NaturalSpawner、DespawnManager）
- 玩家追踪（添加/移除玩家）
- 传送门位置记录（POI 系统）
- 维度 tick 更新（包含世界 tick、同步管理器 tick、刷怪 tick）

**关键方法**:
- `initialize()` - 初始化维度资源，创建同步管理器和刷怪管理器
- `shutdown()` - 清理维度资源，释放同步管理器和刷怪管理器
- `tick()` - 维度刻更新（包含 ServerWorld::tick()、同步管理器 tick、刷怪管理器 tick）
- `addPlayer()/removePlayer()` - 玩家管理
- `recordPortalPosition()/findNearestPortal()` - 传送门追踪
- `entitySyncManager()` / `chunkSendManager()` / `blockUpdateSyncManager()` / `lightSyncManager()` - 同步管理器访问
- `naturalSpawner()` / `despawnManager()` - 刷怪管理器访问

**使用示例**:
`ServerDimension` 不再拥有独立的 `BiomeProvider` / `ServerChunkManager` / `WorldLightManager` 真相源。
区块生成与光照等 runtime 状态统一挂在内部的 `ServerWorld` 上，`ServerDimension` 负责维度级编排与玩家/传送门附加状态。

同步管理器和刷怪管理器由 `ServerDimension` 独立持有，每个维度有自己的一套实例：

```cpp
// 通过维度访问同步管理器
auto* entitySync = dimension->entitySyncManager();
auto* chunkSend = dimension->chunkSendManager();
auto* blockUpdateSync = dimension->blockUpdateSyncManager();
auto* lightSync = dimension->lightSyncManager();

// 通过维度访问刷怪管理器
auto* spawner = dimension->naturalSpawner();
auto* despawn = dimension->despawnManager();
```

### ServerDimensionManager.hpp/cpp

**职责**: 服务端维度管理器，管理所有 `ServerDimension` 实例。

**主要功能**:
- 创建和管理所有维度实例
- 创建主世界 / 下界 / 末地三个 `ServerWorld` runtime
- 为三个维度注入同一个世界级 `SingleLevelStorageManager`
- 玩家维度映射追踪
- 维度切换逻辑
- 维度加载/卸载

**关键方法**:
- `initialize(seed, viewDistance, overworldType)` - 初始化所有维度与主世界生成模式
- `getDimension(id)` - 获取维度实例
- `getPlayerDimension(playerId)` - 获取玩家当前维度
- `transferPlayerToDimension(playerId, targetDim, position)` - 维度切换
- `tick()` - 更新所有维度

**使用示例**:
```cpp
ServerDimensionManager dimensionManager(&server);
dimensionManager.initialize(seed, viewDistance, WorldType::Default);

ServerDimension* overworld = dimensionManager.getOverworld();
ServerWorld* world = overworld->world();

dimensionManager.playerJoinDimension(playerId, DimensionManager::OVERWORLD);
dimensionManager.transferPlayerToDimension(playerId, DimensionManager::NETHER);
dimensionManager.tick();
```

## 模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                        MinecraftServer                           │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                ServerDimensionManager                     │    │
│  │  ┌─────────────────┐ ┌─────────────────┐ ┌─────────────┐ │    │
│  │  │ ServerDimension │ │ ServerDimension │ │ServerDimens.│ │    │
│  │  │   (Overworld)   │ │    (Nether)     │ │  (TheEnd)   │ │    │
│  │  │  ┌───────────┐  │ │  ┌───────────┐  │ │┌───────────┐│ │    │
│  │  │  │ServerWorld│  │ │  │ServerWorld│  │ ││ServerWorld││ │    │
│  │  │  └───────────┘  │ │  └───────────┘  │ │└───────────┘│ │    │
│  │  │  ┌───────────┐  │ │  ┌───────────┐  │ │┌───────────┐│ │    │
│  │  │  │sync 管理器│  │ │  │sync 管理器│  │ ││sync 管理器││ │    │
│  │  │  │ EntitySync│  │ │  │ EntitySync│  │ ││ EntitySync││ │    │
│  │  │  │ ChunkSend │  │ │  │ ChunkSend │  │ ││ ChunkSend ││ │    │
│  │  │  │ BlockUpd  │  │ │  │ BlockUpd  │  │ ││ BlockUpd  ││ │    │
│  │  │  │ LightSync │  │ │  │ LightSync │  │ ││ LightSync ││ │    │
│  │  │  └───────────┘  │ │  └───────────┘  │ │└───────────┘│ │    │
│  │  │  ┌───────────┐  │ │  ┌───────────┐  │ │┌───────────┐│ │    │
│  │  │  │刷怪管理器│  │ │  │刷怪管理器│  │ ││刷怪管理器││ │    │
│  │  │  │NaturalSpn │  │ │  │NaturalSpn │  │ ││NaturalSpn ││ │    │
│  │  │  │DespawnMgr │  │ │  │DespawnMgr │  │ ││DespawnMgr ││ │    │
│  │  │  └───────────┘  │ │  └───────────┘  │ │└───────────┘│ │    │
│  │  │  │共享 Storage │ │ │  │共享 Storage │ │ ││共享 Storage ││ │    │
│  │  │  │共享 SaveMgr │ │ │  │共享 SaveMgr │ │ ││共享 SaveMgr ││ │    │
│  │  └─────────────────┘ └─────────────────┘ └─────────────┘ │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                     玩家维度映射                                  │
│            m_playerDimensions: PlayerId → DimensionId            │
│            m_dimensionPlayers: DimensionId → Set<PlayerId>       │
└─────────────────────────────────────────────────────────────────┘
```

## 维度切换流程

```
玩家请求切换维度
        │
        ▼
ServerDimensionManager::transferPlayerToDimension()
        │
        ├─ 检查目标维度是否存在
        │
        ├─ 获取玩家当前维度
        │
        ├─ 计算目标位置
        │   │
        │   ├─ 传送门搜索 (NetherTeleporter/EndTeleporter)
        │   │
        │   └─ 坐标转换 (主世界 ↔ 下界 1:8)
        │
        ├─ unloadPlayerChunks() - 卸载旧维度区块
        │
        ├─ playerLeaveDimension() - 从旧维度移除
        │
        ├─ playerJoinDimension() - 添加到新维度
        │
        ├─ sendDimensionChangePacket() - 发送维度切换包
        │
        ├─ loadPlayerChunks() - 加载新维度区块
        │
        └─ 触发 dimensionChangeCallback
```

## 传送门追踪

服务端维护传送门位置的 POI (Point of Interest) 系统：

```cpp
// 记录传送门位置
dimension->recordPortalPosition(portalPos);

// 查找最近的传送门
auto nearestPortal = dimension->findNearestPortal(playerPos, 128);

// 移除传送门记录
dimension->forgetPortalPosition(portalPos);
```

## 共享存储

`ServerDimensionManager` 现在负责为三个维度创建 runtime `ServerWorld`，并把同一份世界级存储资源注入进去：

- 一份 `SingleLevelStorageManager`
- 一次 `WorldSessionLock`

这意味着：

- 主世界、下界、末地不会重复打开同一个世界目录
- `ServerWorld::storage()` 在三个维度上返回同一个门面对象
- `ServerWorld::storage()` 在三个维度上返回同一个单存档存储门面

## ServerDimension::tick() 流程

`ServerDimension::tick()` 方法按顺序执行以下逻辑，参考 MC 1.16.5 `MinecraftServer.tickServer()` 中每个维度的处理：

1. `Dimension::tick()` - 基类 tick
2. `m_world->tick()` - 世界 tick（区块管理器、光照、天气、实体、物理等）
3. `m_entitySyncManager->tick()` - 同步该维度的实体位置
4. `m_chunkSendManager->processPendingSends()` - 发送该维度的待发区块
5. `m_blockUpdateSyncManager->flushPendingUpdates()` - 发送该维度的待处理方块更新
6. `m_naturalSpawner->tick()` - 自然刷怪（仅主世界和下界有 hostile 刷怪，仅主世界有 passive 刷怪）
7. `m_despawnManager->tick()` - 生物消失检查

**光照更新要点**：
- 世界 tick 内部通过 `hasLightWork()` 检查是否有待处理的光照工作，避免不必要的处理
- 使用 `std::numeric_limits<i32>::max()` 作为最大更新数量，处理所有待处理的光照
- 根据 `type().hasSkyLight()` 动态决定是否更新天空光照（下界和末地没有天空光照）
- 方块光照始终更新

## 与其他模块的关系

| 模块 | 关系 |
|------|------|
| `common/world/dimension` | 继承 `Dimension` 和 `DimensionManager` |
| `server/application/MinecraftServer` | 持有 `ServerDimensionManager`；不再持有同步管理器和刷怪管理器 |
| `server/sync/` | 同步管理器由 `ServerDimension` 持有和 tick |
| `server/world/spawn/` | 刷怪管理器由 `ServerDimension` 持有和 tick |
| `server/world/ServerWorld` | 每个维度持有一个 runtime `ServerWorld` |
| `common/world/storage/SingleLevelStorageManager` | 三个维度共享同一个世界级单存档存储门面 |
| `server/player/ServerPlayer` | 通过 `m_playerDimensions` 追踪玩家维度 |
| `server/core/TeleportManager` | 处理同维度传送，维度切换由 `ServerDimensionManager` 处理 |

## 维度切换包 (RespawnPacket)

维度切换时发送的 `RespawnPacket` 包含以下关键数据：

| 字段 | 类型 | 说明 |
|------|------|------|
| `dimensionType` | i32 | 维度类型 ID（0=主世界, 1=下界, 2=末地） |
| `dimension` | DimensionId | 维度 ID |
| `hashedSeed` | u64 | 世界种子的 SHA-256 哈希前 8 字节 |
| `gameMode` | GameMode | 游戏模式 |
| `previousGameMode` | GameMode | 之前游戏模式 |
| `isDebug` | bool | 是否调试世界 |
| `isFlat` | bool | 是否超平坦 |
| `keepData` | bool | 是否保留数据 |

### hashedSeed 计算

`hashedSeed` 是世界种子的 SHA-256 哈希值的前 8 字节，用于客户端验证世界的真实性：

```cpp
#include "common/util/crypto/Sha256.hpp"

// 发送维度切换包时计算 hashedSeed
packet.setHashedSeed(util::crypto::Sha256::hashWorldSeed(m_seed));
```

**实现细节**（参考 MC 1.16.5 `BiomeManager.func_235200_a_`）：
1. 将世界种子（u64）以大端序转换为 8 字节
2. 计算 SHA-256 哈希得到 32 字节
3. 取前 8 字节以小端序解释为 u64 返回

**注意**: 直接使用原始种子会导致客户端收到错误的种子哈希，可能引发协议不一致问题。

## 容易踩的坑

1. **维度 ID 约定**: 主世界=0, 下界=1, 末地=2，与 MC 1.16.5 一致
2. **玩家维度同步**: 维度切换时必须同时更新 `m_playerDimensions` 和 `m_dimensionPlayers`
3. **区块卸载顺序**: 必须先卸载旧区块，再加载新区块，避免内存泄漏
4. **主世界不可卸载**: 主世界维度是默认出生点，不能卸载
5. **线程安全**: 维度切换涉及多个管理器，需要在主线程执行
6. **游戏模式获取**: 维度切换包中的游戏模式应从 `ServerPlayerData::gameMode` 获取，而非硬编码
7. **同步管理器是维度级的**: 同步管理器（EntitySyncManager、ChunkSendManager、BlockUpdateSyncManager、LightSyncManager）由各 `ServerDimension` 独立持有，不能从 `MinecraftServer` 直接访问。访问时需先获取目标维度：`dimensionManager.getDimension(id)->entitySyncManager()`
8. **刷怪管理器是维度级的**: NaturalSpawner 和 DespawnManager 由各 `ServerDimension` 独立持有，tick 时根据维度类型决定是否执行（仅主世界和下界有 hostile 刷怪，仅主世界有 passive 刷怪）
9. **初始化顺序**: `ServerDimension::initialize()` 中同步管理器的创建必须在 `ServerWorld::initialize()` 之后，因为同步管理器依赖 `ServerChunkManager` 和 `WorldLightManager`

## 测试用例

### ServerDimensionManagerTest.cpp

| 测试名称 | 说明 |
|----------|------|
| `DimensionConstantsAreCorrect` | 测试维度 ID 常量正确性 |
| `DefaultConstructor` | 测试默认构造函数 |
| `OverworldDimensionId` | 测试主世界维度 ID |
| `NetherDimensionId` | 测试下界维度 ID |
| `TheEndDimensionId` | 测试末地维度 ID |
| `GameModePreservedInDimensionPacket` | 测试维度切换时游戏模式应该从玩家数据获取 |
| `ServerPlayerDataGameMode` | 测试 ServerPlayerData 的游戏模式字段 |
| `DimensionTypeIdMapping` | 测试维度类型 ID 映射（协议用） |
| `PlayerManagerPlayerRetrieval` | 测试 PlayerManager 玩家获取 |
