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
├── WhitelistManager.hpp/cpp  # 白名单管理器
├── BannedPlayerList.hpp/cpp  # 玩家封禁列表管理器
├── BannedIpList.hpp/cpp      # IP 封禁列表管理器
├── OpListManager.hpp/cpp     # OP 权限列表管理器
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
| `username` | `std::string` | 用户名 |
| `connection` | `ConnectionWeakPtr` | 网络连接（弱引用） |
| `sessionId` | `u32` | 会话ID（TCP连接标识） |
| `ipAddress` | `std::string` | IP 地址（从连接获取，本地连接为空字符串） |
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
| `addPlayer(playerId, uuid, username, connection)` | 添加玩家（自动从连接获取 IP 地址） |
| `removePlayer(playerId)` | 移除玩家 |
| `removePlayerBySessionId(sessionId)` | 根据会话ID移除玩家 |
| `getPlayer(playerId)` | 获取玩家数据 |
| `findBySessionId(sessionId)` | 根据会话ID查找玩家 |
| `findByUsername(username)` | 根据用户名查找玩家（大小写不敏感） |
| `findByUuid(uuid)` | 根据 UUID 字符串查找玩家 |
| `getPlayerIdsByAddress(ipAddress)` | 根据IP地址获取所有玩家ID |
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

**UUID 查找：**
`findByUuid(uuid)` 方法用于根据玩家 UUID 字符串查找玩家数据。这在成就系统中特别有用，当事件携带 UUID 而非 PlayerId 时（如 `CuredZombieVillagerEvent`），需要通过 UUID 查找玩家：

```cpp
// 成就事件处理器中的 UUID 查找示例
void AdvancementEventHandler::onCuredZombieVillager(const CuredZombieVillagerEvent& e) {
    // 通过 UUID 查找玩家数据
    auto* playerData = m_playerManager->findByUuid(e.starterUuid);
    if (!playerData) {
        return;  // 玩家不在线
    }

    // 获取 PlayerId 后继续处理
    PlayerId playerId = playerData->playerId;
    // ...
}
```

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

### WhitelistManager.hpp/cpp

白名单管理器，负责服务器白名单的管理。

**职责：**
- 白名单开关状态管理
- 添加/移除玩家
- 从文件加载/保存白名单
- 检查玩家是否在白名单中

**数据结构：**
```cpp
struct WhitelistEntry {
    std::string uuid;      // 玩家 UUID
    std::string name;      // 玩家名称
};
```

**存储格式（JSON 数组）：**
```json
[
  {"uuid": "xxx-xxx-xxx", "name": "Player1"},
  {"uuid": "yyy-yyy-yyy", "name": "Player2"}
]
```

**主要方法：**
| 方法 | 描述 |
|------|------|
| `isEnabled()` | 检查白名单是否启用 |
| `setEnabled(enabled)` | 设置白名单开关 |
| `addEntry(entry)` | 添加玩家到白名单 |
| `removeEntry(uuid)` | 通过 UUID 移除玩家 |
| `removeEntryByName(name)` | 通过名称移除玩家 |
| `isWhitelisted(uuid)` | 检查玩家是否在白名单中（UUID） |
| `isNameWhitelisted(name)` | 检查玩家名称是否在白名单中（大小写不敏感） |
| `getEntry(uuid)` | 获取白名单条目 |
| `getEntryByName(name)` | 通过名称获取条目 |
| `getAllEntries()` | 获取所有条目 |
| `getAllNames()` | 获取所有名称 |
| `size()` | 获取玩家数量 |
| `clear()` | 清空白名单 |
| `load(path)` | 从文件加载白名单 |
| `save(path)` | 保存白名单到文件 |
| `reload()` | 重新加载白名单 |

**使用示例：**
```cpp
WhitelistManager whitelist;

// 加载白名单
whitelist.load("whitelist.json");

// 启用白名单
whitelist.setEnabled(true);

// 添加玩家
whitelist.addEntry(WhitelistEntry("uuid-123", "Player1"));
whitelist.save();  // 保存更改

// 检查玩家
if (whitelist.isEnabled() && !whitelist.isNameWhitelisted("Player2")) {
    // 拒绝连接
}
```

**线程安全：**
- 所有公共方法都是线程安全的
- 使用 `std::mutex` 保护内部数据结构

---

### BannedPlayerList.hpp/cpp

玩家封禁列表管理器，负责服务器玩家封禁的管理。

**职责：**
- 添加/移除玩家封禁
- 从文件加载/保存封禁列表
- 检查玩家是否被封禁
- 自动清理过期封禁

**数据结构：**
```cpp
struct BannedPlayerEntry {
    std::string uuid;      // 玩家 UUID
    std::string name;      // 玩家名称
    std::string created;   // 封禁创建时间
    std::string source;    // 封禁执行者名称
    std::string expires;   // 过期时间（"forever" 表示永久封禁）
    std::string reason;    // 封禁原因
};
```

**存储格式（JSON 数组）：**
```json
[
  {
    "uuid": "xxx-xxx-xxx",
    "name": "Player1",
    "created": "2024-01-15 10:30:00 +0800",
    "source": "ServerAdmin",
    "expires": "forever",
    "reason": "Griefing"
  }
]
```

**主要方法：**
| 方法 | 描述 |
|------|------|
| `addEntry(entry)` | 添加玩家封禁条目 |
| `removeEntry(uuid)` | 通过 UUID 移除封禁 |
| `removeEntryByName(name)` | 通过名称移除封禁（大小写不敏感） |
| `isBanned(uuid)` | 检查玩家是否被封禁（UUID） |
| `isNameBanned(name)` | 检查玩家名称是否被封禁（大小写不敏感） |
| `getEntry(uuid)` | 获取封禁条目 |
| `getEntryByName(name)` | 通过名称获取封禁条目 |
| `getAllEntries()` | 获取所有封禁条目 |
| `getAllBannedNames()` | 获取所有封禁玩家名称 |
| `size()` | 获取封禁玩家数量 |
| `clear()` | 清空封禁列表 |
| `load(path)` | 从文件加载封禁列表 |
| `save(path)` | 保存封禁列表到文件 |
| `reload()` | 重新加载封禁列表 |

**使用示例：**
```cpp
BannedPlayerList banList;
banList.load("banned-players.json");

// 封禁玩家
BannedPlayerEntry entry(
    "uuid-123",                     // UUID
    "Griefer",                      // 名称
    "2024-01-15 10:00:00 +0800",   // 创建时间
    "ServerAdmin",                  // 执行者
    "forever",                      // 过期时间
    "Griefing"                      // 原因
);
banList.addEntry(entry);
banList.save();

// 检查玩家是否被封禁
if (banList.isBanned(uuid)) {
    auto entry = banList.getEntry(uuid);
    // 显示封禁原因：entry->reason
}

// 通过名称检查（大小写不敏感）
if (banList.isNameBanned("GRIEFER")) {  // 匹配成功
    // 玩家被封禁
}
```

**线程安全：**
- 所有公共方法都是线程安全的
- 使用 `std::mutex` 保护内部数据结构

---

### BannedIpList.hpp/cpp

IP 封禁列表管理器，负责服务器 IP 封禁的管理。

**职责：**
- 添加/移除 IP 封禁
- 从文件加载/保存封禁列表
- 检查 IP 是否被封禁
- 自动清理过期封禁

**数据结构：**
```cpp
struct BannedIpEntry {
    std::string ip;        // IP 地址
    std::string created;   // 封禁创建时间
    std::string source;    // 封禁执行者名称
    std::string expires;   // 过期时间（"forever" 表示永久封禁）
    std::string reason;    // 封禁原因
};
```

**存储格式（JSON 数组）：**
```json
[
  {
    "ip": "192.168.1.100",
    "created": "2024-01-15 12:00:00 +0800",
    "source": "ServerAdmin",
    "expires": "forever",
    "reason": "DDoS attack"
  }
]
```

**主要方法：**
| 方法 | 描述 |
|------|------|
| `addEntry(entry)` | 添加 IP 封禁条目 |
| `removeEntry(ip)` | 移除 IP 封禁 |
| `isBanned(ip)` | 检查 IP 是否被封禁 |
| `getEntry(ip)` | 获取封禁条目 |
| `getAllEntries()` | 获取所有封禁条目 |
| `getAllBannedIps()` | 获取所有封禁 IP 地址 |
| `size()` | 获取封禁 IP 数量 |
| `clear()` | 清空封禁列表 |
| `load(path)` | 从文件加载封禁列表 |
| `save(path)` | 保存封禁列表到文件 |
| `reload()` | 重新加载封禁列表 |

**使用示例：**
```cpp
BannedIpList ipBanList;
ipBanList.load("banned-ips.json");

// 封禁 IP
BannedIpEntry entry(
    "192.168.1.100",                // IP 地址
    "2024-01-15 12:00:00 +0800",   // 创建时间
    "ServerAdmin",                  // 执行者
    "forever",                      // 过期时间
    "DDoS attack"                   // 原因
);
ipBanList.addEntry(entry);
ipBanList.save();

// 检查 IP 是否被封禁
if (ipBanList.isBanned("192.168.1.100")) {
    auto entry = ipBanList.getEntry("192.168.1.100");
    // 显示封禁原因：entry->reason
}
```

**线程安全：**
- 所有公共方法都是线程安全的
- 使用 `std::mutex` 保护内部数据结构

---

### OpListManager.hpp/cpp

OP（操作员）权限列表管理器，负责服务器 OP 权限的管理。

**职责：**
- 添加/移除 OP 权限
- 从文件加载/保存 OP 列表
- 检查玩家是否为 OP
- 查询 OP 权限等级

**数据结构：**
```cpp
/**
 * @brief OP 权限等级
 *
 * 参考 MC 1.16.5 net.minecraft.server.MinecraftServer 中的权限等级定义
 */
enum class OpLevel : u8 {
    Normal = 0,      // 普通玩家，无特殊权限
    Moderator = 1,   // 管理员：绕过出生点保护
    GameMaster = 2,  // 游戏管理员：使用命令方块、调试棒等（默认 OP 等级）
    Admin = 3,       // 高级管理员：管理其他玩家、使用危险命令
    Owner = 4        // 服务器所有者：所有权限
};

/**
 * @brief OP 条目
 */
struct OpEntry {
    std::string uuid;                // 玩家 UUID
    std::string name;                // 玩家名称
    OpLevel level = OpLevel::GameMaster;  // 权限等级
    bool bypassesPlayerLimit = false;      // 是否绕过玩家数量限制
};
```

**存储格式（JSON 数组，与 MC 1.16.5 完全兼容）：**
```json
[
  {
    "uuid": "550e8400-e29b-41d4-a716-446655440000",
    "name": "Admin",
    "level": 4,
    "bypassesPlayerLimit": true
  },
  {
    "uuid": "6ba7b810-9dad-11d1-80b4-00c04fd430c8",
    "name": "Moderator",
    "level": 2,
    "bypassesPlayerLimit": false
  }
]
```

**主要方法：**
| 方法 | 描述 |
|------|------|
| `setEntry(entry)` | 添加或更新 OP 条目 |
| `removeEntry(uuid)` | 通过 UUID 移除 OP |
| `isOp(uuid)` | 检查玩家是否为 OP |
| `getLevel(uuid)` | 获取 OP 权限等级（非 OP 返回 Normal） |
| `getEntry(uuid)` | 获取 OP 条目 |
| `getAllEntries()` | 获取所有 OP 条目 |
| `size()` | 获取 OP 数量 |
| `empty()` | 检查列表是否为空 |
| `clear()` | 清空 OP 列表 |
| `load(path)` | 从文件加载 OP 列表 |
| `save(path)` | 保存 OP 列表到文件 |
| `reload()` | 重新加载 OP 列表 |
| `filePath()` | 获取当前文件路径 |

**使用示例：**
```cpp
OpListManager opList;
opList.load("ops.json");

// 添加 OP（默认等级 2）
OpEntry entry(
    "uuid-123",              // UUID
    "Player1",               // 名称
    OpLevel::GameMaster,     // 权限等级
    false                    // 不绕过玩家限制
);
opList.setEntry(entry);
opList.save();  // 保存更改

// 检查玩家是否为 OP
if (opList.isOp(uuid)) {
    OpLevel level = opList.getLevel(uuid);
    // 根据 level 判断权限
}

// 移除 OP
opList.removeEntry(uuid);
opList.save();
```

**权限等级对应命令权限：**
| 等级 | 权限 |
|------|------|
| 0 (Normal) | 基础命令 |
| 1 (Moderator) | 绕过出生点保护 |
| 2 (GameMaster) | 命令方块、调试棒、选择器 |
| 3 (Admin) | 管理其他玩家、封禁、白名单 |
| 4 (Owner) | 所有命令、停止服务器、op/deop |

**线程安全：**
- 所有公共方法都是线程安全的
- 使用 `std::mutex` 保护内部数据结构

---

### PacketHandler.hpp/cpp

统一数据包处理器，协调各管理器处理入站数据包。

**职责：**
- 解析数据包头部
- 分发到对应处理方法
- 处理登录、移动、心跳、传送确认、聊天等数据包
- 提供事件回调机制
- 处理载具输入、载具位置同步、实体交互、实体动作等数据包

**主要方法：**
| 方法 | 描述 |
|------|------|
| `handlePacket(sessionId, data, size)` | 处理数据包 |
| `handleLoginRequest(sessionId, connection, data, size)` | 处理登录请求 |
| `handlePlayerMove(sessionId, data, size)` | 处理玩家移动 |
| `handlePlayerInput(sessionId, data, size)` | 处理骑乘输入 |
| `handleMoveVehicle(sessionId, data, size)` | 处理载具移动同步 |
| `handleEntityAction(sessionId, data, size)` | 处理实体动作 |
| `handleTeleportConfirm(sessionId, data, size)` | 处理传送确认 |
| `handleKeepAlive(sessionId, data, size, currentTimeMs)` | 处理完整心跳响应（包含包头） |
| `handleChatMessage(sessionId, data, size)` | 处理聊天消息 |
| `handleUseEntity(sessionId, data, size)` | 处理实体交互 |
| `setServer(server)` | 设置服务器接口指针 |
| `getServer()` | 获取服务器接口指针 |

**载具输入处理 (handlePlayerInput)：**
- 通过 `IServer` 接口获取 `ServerPlayerEntityManager` 和 `ServerWorld`
- 验证玩家是否正在骑乘
- 获取载具实体并验证玩家是否为控制者
- 设置玩家移动状态（`setMoveStrafing`、`setMoveForward`、`setJumping`、`setSneaking`）
- 处理跳跃载具（`IJumpingMount` 接口）的跳跃输入

**载具位置验证 (handleMoveVehicle)：**
- 获取最底层载具（支持嵌套骑乘，如玩家骑马、马骑船）
- 验证玩家是否是载具的控制者
- 验证数据包有效性（坐标是否为有限数值）
- 速度验证防止作弊（`MAX_VEHICLE_SPEED_SQ = 100.0`）
- 更新载具位置和旋转
- 同步玩家位置跟随载具

**实体交互处理 (handleUseEntity)：**
- 获取玩家实体和目标实体
- 距离检查（创造模式跳过）
- 处理三种交互类型：
  - `Interact` - 右键交互（调用 `player->interactOn()`）
  - `Attack` - 左键攻击（调用 `player->attack()`）
  - `InteractAt` - 指定位置交互（简化实现）
- 成功交互后触发挥手动画

**实体动作处理 (handleEntityAction)：**
- `PressShiftKey` - 设置潜行状态，骑乘时触发下马
- `ReleaseShiftKey` - 释放潜行状态
- `StartRidingJump` - 处理马跳跃蓄力（`IJumpingMount` 接口）
- `StopRidingJump` - 停止马跳跃蓄力
- `StartSprinting` - 开始疾跑
- `StopSprinting` - 停止疾跑

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

**依赖注入：**
```cpp
// PacketHandler 需要通过 setServer() 设置 IServer 指针
// 以访问 ServerPlayerEntityManager 和 ServerWorld
packetHandler.setServer(&server);
```

**参考 MC 1.16.5：**
- `ServerPlayNetHandler.processInput()` - 玩家输入处理
- `ServerPlayNetHandler.processVehicleMove()` - 载具移动验证
- `ServerPlayNetHandler.processUseEntity()` - 实体交互
- `ServerPlayNetHandler.processEntityAction()` - 实体动作

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
                                  │
         ┌────────────────────────┼────────────────────────┐
         │                        │                        │
         ▼                        ▼                        ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│ WhitelistManager│     │BannedPlayerList │     │  OpListManager  │
│   (白名单管理)   │     │  (玩家封禁管理)  │     │  (OP权限管理)   │
└─────────────────┘     └─────────────────┘     └─────────────────┘
         │                        │                        │
         └────────────────────────┴────────────────────────┘
                                  │
                                  ▼
                       ┌─────────────────────┐
                       │   BannedIpList      │
                       │   (IP封禁管理)       │
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
packetHandler.setOnLoginSuccess([](PlayerId playerId, const std::string& username) {
    spdlog::info("Player {} logged in", username);
});

packetHandler.setOnChat([](PlayerId playerId, const std::string& username, const std::string& message) {
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
| `WhitelistManagerTest.cpp` | 白名单启用/禁用、条目管理、文件加载/保存 |
| `BannedPlayerListTest.cpp` | 玩家封禁条目管理、文件加载/保存、过期检查 |
| `BannedIpListTest.cpp` | IP 封禁条目管理、文件加载/保存、过期检查 |
| `OpListManagerTest.cpp` | OP 权限管理、条目增删改查、文件加载/保存 |

### 运行测试

```powershell
# 构建项目
cmake --build --preset windows-clang-relwithdebinfo

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
using LoginCallback = std::function<void(PlayerId, const std::string&)>;
using ChatCallback = std::function<void(PlayerId, const std::string&, const std::string&)>;
```

---

## 性能考虑

1. **锁粒度** - `PlayerManager` 使用 `recursive_mutex` 允许嵌套调用
2. **遍历优化** - `forEachPlayer` 先复制 ID 列表再遍历，避免死锁
3. **内存管理** - 使用智能指针管理连接和区块追踪器
4. **数据包封装** - `encapsulatePacket` 是静态方法，避免对象创建开销
