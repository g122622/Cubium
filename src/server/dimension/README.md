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
- `ServerWorld` 管理
- `ServerChunkManager` 管理
- `WorldLightManager` 管理
- 玩家追踪（添加/移除玩家）
- 传送门位置记录（POI 系统）
- 维度 tick 更新

**关键方法**:
- `initialize()` - 初始化维度资源
- `shutdown()` - 清理维度资源
- `tick()` - 维度刻更新（包含区块管理器和光照管理器更新）
- `addPlayer()/removePlayer()` - 玩家管理
- `recordPortalPosition()/findNearestPortal()` - 传送门追踪

**使用示例**:
```cpp
auto dimension = std::make_unique<ServerDimension>(
    DimensionManager::OVERWORLD,
    DimensionType::overworld(),
    std::move(generator),
    std::move(biomeProvider),
    seed,
    viewDistance
);

dimension->initialize();
dimension->addPlayer(playerId);
dimension->tick();
```

### ServerDimensionManager.hpp/cpp

**职责**: 服务端维度管理器，管理所有 `ServerDimension` 实例。

**主要功能**:
- 创建和管理所有维度实例
- 玩家维度映射追踪
- 维度切换逻辑
- 维度加载/卸载

**关键方法**:
- `initialize(seed, viewDistance)` - 初始化所有维度
- `getDimension(id)` - 获取维度实例
- `getPlayerDimension(playerId)` - 获取玩家当前维度
- `transferPlayerToDimension(playerId, targetDim, position)` - 维度切换
- `tick()` - 更新所有维度

**使用示例**:
```cpp
ServerDimensionManager dimensionManager(&server);
dimensionManager.initialize(seed, viewDistance);

// 玩家进入维度
dimensionManager.playerJoinDimension(playerId, DimensionManager::OVERWORLD);

// 维度切换
dimensionManager.transferPlayerToDimension(playerId, DimensionManager::NETHER);

// 每帧更新
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
│  │  │  ┌────────────┐ │ │  ┌────────────┐ │ │┌────────────┐│ │    │
│  │  │  │ChunkManager│ │ │  │ChunkManager│ │ ││ChunkManager││ │    │
│  │  │  └────────────┘ │ │  └────────────┘ │ │└────────────┘│ │    │
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

## 光照更新

`ServerDimension::tick()` 方法中包含光照更新逻辑，参考 MC 1.16.5 `ServerChunkProvider.ChunkExecutor.driveOne()` 实现：

```cpp
void ServerDimension::tick() {
    Dimension::tick();

    // 更新区块管理器
    if (m_chunkManager) {
        m_chunkManager->tick();
    }

    // 更新光照管理器
    if (m_lightManager) {
        if (m_lightManager->hasLightWork()) {
            // 处理所有待处理的光照更新
            // 参数：maxUpdates=最大值（处理所有）, updateSkyLight=根据维度类型, updateBlockLight=true
            m_lightManager->tick(std::numeric_limits<i32>::max(), type().hasSkyLight(), true);
        }
    }
}
```

**光照更新要点**：
- 使用 `hasLightWork()` 检查是否有待处理的光照工作，避免不必要的处理
- 使用 `std::numeric_limits<i32>::max()` 作为最大更新数量，处理所有待处理的光照
- 根据 `type().hasSkyLight()` 动态决定是否更新天空光照（下界和末地没有天空光照）
- 方块光照始终更新

## 与其他模块的关系

| 模块 | 关系 |
|------|------|
| `common/world/dimension` | 继承 `Dimension` 和 `DimensionManager` |
| `server/application/MinecraftServer` | 持有 `ServerDimensionManager` |
| `server/world/ServerWorld` | 每个维度持有一个 `ServerWorld` |
| `server/world/ServerChunkManager` | 每个维度持有一个区块管理器 |
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
