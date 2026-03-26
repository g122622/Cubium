# Server Core Module

服务端核心模块，提供服务器运行时的基础管理功能。

## 目录结构

```
src/server/core/
├── ServerCoreConfig.hpp      # 服务端核心配置结构
├── ServerPlayerData.hpp      # 服务端玩家数据结构
├── PlayerManager.hpp/cpp     # 玩家生命周期管理器
├── ConnectionManager.hpp/cpp # 连接与消息管理器
├── TimeManager.hpp/cpp       # 游戏时间管理器
├── KeepAliveManager.hpp/cpp  # 心跳管理器
├── TeleportManager.hpp/cpp   # 传送管理器
├── PositionTracker.hpp/cpp   # 位置追踪器
├── GameModeManager.hpp/cpp   # 游戏模式管理器
└── PacketHandler.hpp/cpp     # 统一数据包处理器
```

## 文件详解

### ServerCoreConfig.hpp

服务端核心配置结构，定义服务器运行时参数。

```cpp
struct ServerCoreConfig {
    i32 viewDistance = 10;           // 视距（区块数）
    i32 keepAliveInterval = 15000;   // 心跳间隔（毫秒）
    i32 keepAliveTimeout = 30000;    // 心跳超时（毫秒）
    GameMode defaultGameMode = GameMode::Survival;  // 默认游戏模式
    u64 seed = 12345;                // 世界种子
    i32 maxPlayers = 20;             // 最大玩家数
    i32 tickRate = 20;               // 服务器 TPS
};
```

**常量定义：**
- `TICK_DURATION_MS = 50` - Tick 持续时间（20 TPS）
- `KEEPALIVE_CHECK_INTERVAL_TICKS = 300` - 心跳检查间隔
- `CLEANUP_INTERVAL_TICKS = 20` - 断开连接清理间隔

---

### ServerPlayerData.hpp

服务端玩家数据结构，存储单个玩家的完整状态信息。

**核心字段：**
| 字段 | 类型 | 描述 |
|------|------|------|
| `playerId` | `PlayerId` | 玩家唯一标识 |
| `username` | `String` | 用户名 |
| `connection` | `ConnectionWeakPtr` | 网络连接（弱引用） |
| `sessionId` | `u32` | 会话ID（TCP连接标识） |
| `loggedIn` | `bool` | 登录状态 |
| `chunkTracker` | `shared_ptr<PlayerChunkTracker>` | 区块追踪器 |
| `x, y, z` | `f32` | 世界坐标 |
| `yaw, pitch` | `f32` | 旋转角度 |
| `onGround` | `bool` | 是否在地面 |
| `gameMode` | `GameMode` | 游戏模式 |
| `pendingTeleportId` | `u32` | 待确认传送ID |
| `waitingTeleportConfirm` | `bool` | 是否等待传送确认 |
| `lastKeepAliveSent/Received` | `u64` | 心跳时间戳 |
| `ping` | `u32` | 网络延迟（毫秒） |
| `loadedChunks` | `unordered_set<ChunkId>` | 已加载区块 |
| `openMenu` | `AbstractContainerMenu*` | 打开的容器菜单 |

**便捷方法：**
- `getConnection()` - 获取连接共享指针
- `hasConnection()` - 检查连接有效性
- `send(data, size)` - 发送数据
- `chunkX()/chunkZ()` - 获取区块坐标
- `position()/rotation()` - 获取位置/旋转向量

---

### PlayerManager.hpp/cpp

玩家生命周期管理器，负责玩家的注册、移除、查询、遍历等操作。

**职责：**
- 玩家添加/移除
- 玩家ID和会话ID映射
- 玩家数据查询与遍历
- 区块同步管理器集成
- 线程安全保证

**主要方法：**
| 方法 | 描述 |
|------|------|
| `addPlayer(playerId, username, connection)` | 添加玩家 |
| `removePlayer(playerId)` | 移除玩家 |
| `removePlayerBySessionId(sessionId)` | 根据会话ID移除玩家 |
| `getPlayer(playerId)` | 获取玩家数据 |
| `findBySessionId(sessionId)` | 根据会话ID查找玩家 |
| `hasPlayer(playerId)` | 检查玩家是否存在 |
| `playerCount()` | 获取玩家数量 |
| `isFull()` | 检查是否已满 |
| `forEachPlayer(func)` | 遍历所有玩家 |
| `nextPlayerId()` | 生成新玩家ID |
| `nextSessionId()` | 生成新会话ID |
| `mapSessionToPlayer(sessionId, playerId)` | 建立会话映射 |
| `chunkSyncManager()` | 获取区块同步管理器 |

**线程安全：**
- 使用 `std::recursive_mutex` 保护内部数据
- `forEachPlayer` 允许在回调中嵌套调用其他方法

---

### ConnectionManager.hpp/cpp

连接管理器，负责消息发送、广播、连接断开、数据包封装。

**职责：**
- 向单个玩家发送数据/数据包
- 广播消息给所有玩家
- 广播消息给除指定玩家外的所有玩家
- 断开玩家连接
- 清理已断开连接的玩家
- 数据包封装

**主要方法：**
| 方法 | 描述 |
|------|------|
| `sendToPlayer(playerId, data, size)` | 发送原始数据 |
| `sendPacketToPlayer(playerId, type, payload)` | 发送封装数据包 |
| `sendSerializedPacket(playerId, packet)` | 发送已序列化数据包 |
| `broadcast(data, size)` | 广播原始数据 |
| `broadcastPacket(type, payload)` | 广播数据包 |
| `broadcastExcept(excludeId, data, size)` | 排除性广播 |
| `disconnectPlayer(playerId, reason)` | 断开玩家连接 |
| `disconnectAll(reason)` | 断开所有连接 |
| `cleanupDisconnectedPlayers(removedPlayers)` | 清理断开连接 |
| `encapsulatePacket(type, payload)` | 封装数据包（静态） |

**数据包格式：**
```
| Size (4B) | Type (2B) | Flags (2B) | Reserved (2B) | Padding (2B) | Payload |
```

---

### TimeManager.hpp/cpp

游戏时间管理器，负责游戏时间、tick 计数、日光周期管理。

**职责：**
- 游戏时间（总 tick 数）管理
- 日光周期（昼夜循环）管理
- 时间流逝控制

**主要方法：**
| 方法 | 描述 |
|------|------|
| `tick()` | 更新时间（每 tick 调用） |
| `gameTime()` | 获取游戏时间 |
| `currentTick()` | 获取当前 tick |
| `setGameTime(time)` | 设置游戏时间 |
| `dayTime()` | 获取日光时间（0-23999） |
| `setDayTime(time)` | 设置日光时间 |
| `addDayTime(ticks)` | 增加日光时间 |
| `dayCount()` | 获取天数 |
| `daylightCycleEnabled()` | 检查日光周期是否启用 |
| `setDaylightCycleEnabled(enabled)` | 启用/禁用日光周期 |

**日光时间对应：**
- 0 = 6:00 AM（日出）
- 6000 = 正午
- 12000 = 6:00 PM（日落）
- 18000 = 午夜

---

### KeepAliveManager.hpp/cpp

心跳管理器，负责心跳计时、超时检测、ping 计算。

**职责：**
- 判断玩家是否需要发送心跳
- 记录心跳发送/接收时间
- 计算 ping 延迟
- 检测超时玩家

**主要方法：**
| 方法 | 描述 |
|------|------|
| `needsKeepAlive(playerId, currentMs)` | 检查是否需要发送心跳 |
| `getPlayersNeedingKeepAlive(currentMs)` | 获取需要心跳的玩家列表 |
| `recordKeepAliveSent(playerId, timestamp, tick)` | 记录心跳发送 |
| `handleKeepAliveResponse(playerId, timestamp, currentMs)` | 处理心跳响应 |
| `updateKeepAlive(playerId, timestamp)` | 更新心跳时间戳（简化版） |
| `isTimedOut(playerId, currentMs)` | 检查是否超时 |
| `getTimedOutPlayers(currentMs)` | 获取超时玩家列表 |
| `getPlayerPing(playerId)` | 获取玩家 ping |

**配置依赖：**
- `keepAliveInterval` - 心跳发送间隔
- `keepAliveTimeout` - 超时判定时间

---

### TeleportManager.hpp/cpp

传送管理器，负责玩家传送请求、确认、ID 生成。

**职责：**
- 发起传送请求（更新位置、生成传送ID）
- 发送传送数据包
- 确认传送（客户端响应）
- 追踪传送状态

**主要方法：**
| 方法 | 描述 |
|------|------|
| `requestTeleport(playerId, x, y, z, yaw, pitch)` | 请求传送 |
| `confirmTeleport(playerId, teleportId)` | 确认传送 |
| `isWaitingForConfirm(playerId)` | 检查是否等待确认 |
| `getPendingTeleportId(playerId)` | 获取待确认传送ID |

**传送流程：**
1. 服务端调用 `requestTeleport()` 发起传送
2. 服务端更新玩家位置并发送 `TeleportPacket`
3. 客户端收到传送包后处理位置更新
4. 客户端发送 `TeleportConfirmPacket`
5. 服务端调用 `confirmTeleport()` 确认传送

---

### PositionTracker.hpp/cpp

位置追踪器，负责玩家位置更新、区块订阅、移动验证。

**职责：**
- 更新玩家位置和旋转
- 计算区块加载/卸载更新
- 管理区块发送状态
- 查询区块订阅者

**主要方法：**
| 方法 | 描述 |
|------|------|
| `updatePosition(playerId, x, y, z, yaw, pitch, onGround)` | 更新完整位置 |
| `updatePosition(playerId, x, y, z)` | 仅更新坐标 |
| `updateRotation(playerId, yaw, pitch)` | 仅更新旋转 |
| `calculateChunkUpdates(playerId, toLoad, toUnload)` | 计算区块更新 |
| `markChunkSent(playerId, x, z)` | 标记区块已发送 |
| `markChunkUnloaded(playerId, x, z)` | 标记区块已卸载 |
| `getChunkSubscribers(x, z)` | 获取区块订阅者 |
| `getPosition(playerId)` | 获取玩家位置 |
| `getRotation(playerId)` | 获取玩家旋转 |
| `getChunkPosition(playerId)` | 获取区块坐标 |
| `isOnGround(playerId)` | 检查是否在地面 |
| `setViewDistance(playerId, distance)` | 设置视距 |
| `getViewDistance(playerId)` | 获取视距 |

---

### GameModeManager.hpp/cpp

游戏模式管理器，负责游戏模式切换和能力同步。

**职责：**
- 设置玩家游戏模式
- 发送 `GameStateChangePacket` 和 `PlayerAbilitiesPacket`
- 根据游戏模式计算玩家能力
- 支持游戏模式变化回调

**主要方法：**
| 方法 | 描述 |
|------|------|
| `setGameMode(playerId, mode)` | 设置游戏模式（带网络同步） |
| `setGameModeLocal(playerId, mode)` | 仅设置本地模式 |
| `getGameMode(playerId)` | 获取游戏模式 |
| `syncAbilities(playerId)` | 同步能力到客户端 |
| `getAbilitiesForGameMode(mode)` | 获取游戏模式对应能力 |
| `setOnGameModeChange(callback)` | 设置变化回调 |

**游戏模式能力映射：**
| 模式 | 无敌 | 飞行 | 可飞行 | 创造模式 |
|------|------|------|--------|----------|
| Survival | 否 | 否 | 否 | 否 |
| Creative | 是 | 否 | 是 | 是 |
| Adventure | 否 | 否 | 否 | 否 |
| Spectator | 是 | 是 | 是 | 否 |

---

### PacketHandler.hpp/cpp

统一数据包处理器，协调各管理器处理入站数据包。

**职责：**
- 解析数据包头部
- 分发到对应处理方法
- 处理登录、移动、心跳、传送确认、聊天等数据包
- 提供事件回调机制

**主要方法：**
| 方法 | 描述 |
|------|------|
| `handlePacket(sessionId, data, size)` | 处理数据包 |
| `handleLoginRequest(sessionId, connection, data, size)` | 处理登录请求 |
| `handlePlayerMove(sessionId, data, size)` | 处理玩家移动 |
| `handleTeleportConfirm(sessionId, data, size)` | 处理传送确认 |
| `handleKeepAlive(sessionId, data, size, currentTimeMs)` | 处理心跳响应 |
| `handleChatMessage(sessionId, data, size)` | 处理聊天消息 |

**处理结果枚举：**
```cpp
enum class PacketHandleResult {
    Success,    // 处理成功
    Ignore,     // 忽略（未登录等）
    Disconnect, // 需要断开连接
    Error       // 处理错误
};
```

**回调类型：**
- `LoginCallback` - 登录成功回调
- `LoginFailCallback` - 登录失败回调
- `DisconnectCallback` - 断开连接回调
- `ChatCallback` - 聊天消息回调

---

## 模块关系图

```
                    ┌─────────────────────┐
                    │  ServerCoreConfig   │
                    └──────────┬──────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                        PacketHandler                          │
│  (统一数据包处理入口，协调所有管理器)                            │
└────────┬─────────────────────────────────────────────────────┘
         │
         ├─────────────────────────────────────────────────────┐
         │                         │                           │
         ▼                         ▼                           ▼
┌─────────────────┐     ┌───────────────────┐     ┌─────────────────┐
│  PlayerManager  │◄────│ ConnectionManager │     │   TimeManager   │
│  (玩家生命周期)  │     │   (网络通信)       │     │   (游戏时间)    │
└────────┬────────┘     └─────────┬─────────┘     └─────────────────┘
         │                        │
         │    ┌───────────────────┼───────────────────┐
         │    │                   │                   │
         ▼    ▼                   ▼                   ▼
┌─────────────────┐     ┌─────────────────┐ ┌─────────────────┐
│KeepAliveManager │     │TeleportManager  │ │PositionTracker  │
│   (心跳检测)     │     │   (传送管理)     │ │  (位置追踪)     │
└─────────────────┘     └─────────────────┘ └─────────────────┘
         │                        │                   │
         └────────────────────────┴───────────────────┘
                                  │
                                  ▼
                       ┌─────────────────────┐
                       │   GameModeManager   │
                       │   (游戏模式管理)     │
                       └─────────────────────┘
```

**依赖关系：**
- `PacketHandler` 依赖所有其他管理器
- `ConnectionManager` 依赖 `PlayerManager`
- `KeepAliveManager` 依赖 `PlayerManager`
- `TeleportManager` 依赖 `PlayerManager`、`ConnectionManager`
- `PositionTracker` 依赖 `PlayerManager`
- `GameModeManager` 依赖 `PlayerManager`、`ConnectionManager`

---

## 模块整体职责

服务端核心模块负责服务器运行时的**基础管理功能**：

1. **玩家管理** - 玩家注册、移除、查询、会话映射
2. **连接管理** - 消息发送、广播、连接断开
3. **时间管理** - 游戏 tick、昼夜循环
4. **心跳检测** - 网络延迟监控、超时检测
5. **传送系统** - 传送请求与确认
6. **位置追踪** - 玩家位置、区块订阅
7. **游戏模式** - 模式切换、能力同步
8. **数据包处理** - 统一入站数据处理

---

## 输入和输出

### 输入

| 来源 | 数据类型 | 处理方式 |
|------|----------|----------|
| 网络层 | 原始数据包 | `PacketHandler::handlePacket()` |
| 配置 | `ServerCoreConfig` | 构造函数注入 |
| 外部调用 | 玩家操作请求 | 各管理器公共方法 |

### 输出

| 目标 | 数据类型 | 产生方式 |
|------|----------|----------|
| 网络层 | 封装数据包 | `ConnectionManager::encapsulatePacket()` |
| 回调 | 事件通知 | 各管理器回调函数 |
| 状态查询 | 玩家/服务器状态 | 各管理器查询方法 |

---

## 依赖项

### 内部依赖

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义 |
| `common/core/Constants.hpp` | 游戏常量 |
| `common/network/connection/IServerConnection.hpp` | 连接接口 |
| `common/network/sync/ChunkSync.hpp` | 区块同步管理器 |
| `common/network/packet/*` | 数据包定义 |
| `common/world/time/GameTime.hpp` | 游戏时间类 |
| `common/entity/GameModeUtils.hpp` | 游戏模式工具 |
| `common/entity/inventory/ContainerTypes.hpp` | 容器类型 |

### 外部依赖

| 库 | 用途 |
|---|------|
| `spdlog` | 日志输出 |
| STL | 容器、智能指针、线程同步 |

---

## 使用方法

### 基本初始化

```cpp
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/PacketHandler.hpp"

using namespace mc::server::core;

// 创建配置
ServerCoreConfig config;
config.viewDistance = 12;
config.maxPlayers = 50;
config.keepAliveInterval = 15000;
config.keepAliveTimeout = 30000;

// 创建管理器
PlayerManager playerManager(config);
ConnectionManager connectionManager(playerManager);
TimeManager timeManager;
KeepAliveManager keepAliveManager(playerManager, config);
TeleportManager teleportManager(playerManager);
PositionTracker positionTracker(playerManager, config);
GameModeManager gameModeManager(playerManager, connectionManager);

// 创建数据包处理器
PacketHandler packetHandler(
    playerManager,
    connectionManager,
    teleportManager,
    keepAliveManager,
    positionTracker,
    timeManager,
    config
);

// 设置回调
packetHandler.setOnLoginSuccess([](PlayerId playerId, const String& username) {
    spdlog::info("Player {} logged in", username);
});

packetHandler.setOnChat([](PlayerId playerId, const String& username, const String& message) {
    spdlog::info("[Chat] {}: {}", username, message);
});
```

### 游戏主循环

```cpp
void tick(u64 currentTimeMs) {
    // 更新游戏时间
    timeManager.tick();

    // 发送心跳
    auto players = keepAliveManager.getPlayersNeedingKeepAlive(currentTimeMs);
    for (PlayerId playerId : players) {
        keepAliveManager.recordKeepAliveSent(playerId, currentTimeMs, timeManager.currentTick());
        // 发送 KeepAlivePacket...
    }

    // 检查超时
    auto timedOut = keepAliveManager.getTimedOutPlayers(currentTimeMs);
    for (PlayerId playerId : timedOut) {
        connectionManager.disconnectPlayer(playerId, "Timeout");
    }

    // 清理断开连接
    connectionManager.cleanupDisconnectedPlayers();
}
```

---

## 容易踩的坑

### 1. 线程安全

**问题：** `PlayerManager` 使用 `recursive_mutex`，但其他管理器并非完全线程安全。

**建议：**
- 在多线程环境下，确保对外部调用进行同步
- `forEachPlayer` 允许嵌套调用，但不要在回调中长时间持有锁

### 2. 连接生命周期

**问题：** `ServerPlayerData::connection` 是弱引用，连接可能随时失效。

**建议：**
```cpp
// 正确做法：每次使用前检查
auto* player = playerManager.getPlayer(playerId);
if (player && player->hasConnection()) {
    player->send(data, size);
}

// 错误做法：直接使用弱引用
auto conn = player->connection.lock();  // 可能返回空
```

### 3. 传送确认流程

**问题：** 客户端必须确认传送，否则服务端状态不一致。

**建议：**
```cpp
// 发起传送后检查确认状态
u32 teleportId = teleportManager.requestTeleport(playerId, x, y, z);

// 在下次收到 TeleportConfirmPacket 时验证
if (!teleportManager.confirmTeleport(playerId, packet.teleportId())) {
    // 可能是过期或伪造的确认
}
```

### 4. 区块追踪器初始化

**问题：** `ServerPlayerData::chunkTracker` 需要在添加玩家时创建。

**建议：**
```cpp
// PlayerManager::addPlayer 内部会自动创建 chunkTracker
auto* player = playerManager.addPlayer(playerId, username, connection);
// player->chunkTracker 已经初始化
```

### 5. 时间戳单位

**问题：** 心跳使用毫秒时间戳，其他地方可能用 tick。

**建议：**
- `KeepAliveManager` 使用毫秒时间戳
- `TimeManager` 使用 tick
- 转换时注意单位：`tick * 50 = ms`

### 6. 数据包处理顺序

**问题：** 未登录玩家发送某些数据包会导致逻辑错误。

**建议：**
```cpp
// 在 PacketHandler::handlePacket 中检查登录状态
PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
if (playerId == 0) {
    return PacketHandleResult::Ignore;  // 未登录
}
```

---

## 测试用例

模块包含完整的单元测试，位于 `tests/server/core/` 目录：

| 测试文件 | 测试内容 |
|----------|----------|
| `PlayerManagerTest.cpp` | 玩家添加/移除、会话映射、遍历、嵌套调用 |
| `ConnectionManagerTest.cpp` | 发送、广播、断开连接、数据包封装 |
| `TimeManagerTest.cpp` | 时间更新、日光周期、天数计算 |
| `KeepAliveManagerTest.cpp` | 心跳发送/响应、超时检测、ping 计算 |
| `TeleportManagerTest.cpp` | 传送请求/确认、ID 验证、多次传送 |
| `PositionTrackerTest.cpp` | 位置更新、区块计算、视距管理 |

### 运行测试

```powershell
# 构建项目
cmake --build build --config Release

# 运行所有测试
./build/bin/Release/mc_tests.exe

# 运行特定测试
./build/bin/Release/mc_tests.exe --gtest_filter="PlayerManagerTest.*"
./build/bin/Release/mc_tests.exe --gtest_filter="KeepAliveManagerTest.*"
./build/bin/Release/mc_tests.exe --gtest_filter="TeleportManagerTest.*"
```

### 测试覆盖

- **PlayerManager**: 添加/移除、重复添加、满员、会话映射、嵌套遍历
- **ConnectionManager**: 单播、广播、排除广播、断开连接、数据包封装
- **TimeManager**: 默认构造、时间更新、日光周期、天数计算
- **KeepAliveManager**: 心跳间隔、响应处理、超时检测、ping 计算
- **TeleportManager**: 传送请求、确认验证、ID 匹配、多次传送
- **PositionTracker**: 位置更新、区块计算、视距设置、区块发送状态

---

## 设计模式

### 管理器模式

每个管理器负责单一职责：
- `PlayerManager` - 玩家生命周期
- `ConnectionManager` - 网络通信
- `KeepAliveManager` - 心跳检测
- 等等...

### 依赖注入

管理器通过构造函数接收依赖：
```cpp
KeepAliveManager(PlayerManager& playerManager, const ServerCoreConfig& config);
PacketHandler(PlayerManager&, ConnectionManager&, TeleportManager&, ...);
```

### 回调机制

使用 `std::function` 实现事件通知：
```cpp
using LoginCallback = std::function<void(PlayerId, const String&)>;
using ChatCallback = std::function<void(PlayerId, const String&, const String&)>;
```

---

## 性能考虑

1. **锁粒度** - `PlayerManager` 使用 `recursive_mutex` 允许嵌套调用
2. **遍历优化** - `forEachPlayer` 先复制 ID 列表再遍历，避免死锁
3. **内存管理** - 使用智能指针管理连接和区块追踪器
4. **数据包封装** - `encapsulatePacket` 是静态方法，避免对象创建开销
