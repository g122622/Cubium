# Server Core Module

服务端核心模块，提供服务器运行时的基础管理功能。

## 目录结构

```
src/server/core/
├── ServerPlayerData.hpp      # 服务端玩家数据结构
├── PlayerManager.hpp/cpp     # 玩家生命周期管理器
├── ConnectionManager.hpp/cpp # IR 发送门面（ir::IrPacket 发送/广播/断开）
├── TimeManager.hpp/cpp       # 游戏时间管理器
├── KeepAliveManager.hpp/cpp  # 心跳管理器
├── TeleportManager.hpp/cpp   # 传送管理器
├── PositionTracker.hpp/cpp   # 位置追踪器
├── GameModeManager.hpp/cpp   # 游戏模式管理器
├── WhitelistManager.hpp/cpp  # 白名单管理器
├── BannedPlayerList.hpp/cpp  # 玩家封禁列表管理器
├── BannedIpList.hpp/cpp      # IP 封禁列表管理器
└── OpListManager.hpp/cpp     # OP 权限列表管理器
```

> **注**：旧的 `PacketHandler.hpp/.cpp` 和 `PacketHandlerInternal.hpp` 已删除。入站数据包处理逻辑迁移到 `MinecraftServer::routeInboundPlayPacket` + `src/server/network/ServerPlayRouter`（`std::visit` over `ir::PlayPacket`）。

## 模块关系图

```
                    ┌─────────────────────┐
                    │    ServerSettings   │
                    └──────────┬──────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│              MinecraftServer::routeInboundPlayPacket          │
│        + server/network/ServerPlayRouter (ir::PlayPacket)     │
│  (入站数据包分发入口，协调各管理器；旧 PacketHandler 已删除)    │
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
- 入站包分发（`ServerPlayRouter` + `MinecraftServer::routeInboundPlayPacket`）依赖所有其他管理器
- `ConnectionManager` 依赖 `PlayerManager`
- `KeepAliveManager` 依赖 `PlayerManager`
- `TeleportManager` 依赖 `PlayerManager`、`ConnectionManager`
- `PositionTracker` 依赖 `PlayerManager`
- `GameModeManager` 依赖 `PlayerManager`、`ConnectionManager`

## 内部模块职责

| 模块 | 职责 |
|------|------|
| `PlayerManager` | 玩家注册、移除、查询、会话映射、线程安全访问 |
| `ConnectionManager` | IR 发送门面：`ir::IrPacket` 发送、广播、连接断开 |
| `TimeManager` | 游戏 tick、昼夜循环、时间流逝控制 |
| `KeepAliveManager` | 心跳计时、超时检测、ping 计算 |
| `TeleportManager` | 传送请求、确认、ID 生成 |
| `PositionTracker` | 玩家位置更新、区块订阅、移动验证 |
| `GameModeManager` | 游戏模式切换、能力同步 |
| `WhitelistManager` | 白名单开关、条目增删查、文件加载保存 |
| `BannedPlayerList` | 玩家封禁条目管理、过期检查、文件持久化 |
| `BannedIpList` | IP 封禁条目管理、过期检查、文件持久化 |
| `OpListManager` | OP 权限管理、权限等级查询、文件持久化 |
| `ServerPlayerData` | 单个玩家的完整状态数据结构 |

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义 |
| `common/core/Constants.hpp` | 游戏常量 |
| `common/network/ir/IrPacket.hpp` | IR 包定义（ConnectionManager 发送门面使用） |
| `common/network/sync/ChunkSync.hpp` | 区块同步管理器 |
| `common/network/codec/PacketSerializer.hpp`<br>`common/network/codec/PacketDeserializer.hpp` | 编解码器（从旧 packet/ 迁出后的存活件，供 IR codec 与残余 wire 桥接使用） |
| `common/world/time/GameTime.hpp` | 游戏时间类 |
| `common/entity/GameModeUtils.hpp` | 游戏模式工具 |
| `common/entity/inventory/ContainerTypes.hpp` | 容器类型 |
| `spdlog` | 日志输出 |
| STL | 容器、智能指针、线程同步 |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `server/MinecraftServer` | 服务器主类，创建和协调所有管理器 |
| `server/IntegratedServer` | 内置服务器，使用核心管理器处理玩家 |
| `server/StandaloneServer` | 独立服务器，使用核心管理器处理网络连接 |

## 容易踩的坑

### 1. 线程安全

`PlayerManager` 使用 `recursive_mutex`，但其他管理器并非完全线程安全。在多线程环境下，确保对外部调用进行同步。`forEachPlayer` 允许嵌套调用，但不要在回调中长时间持有锁。

### 2. 连接生命周期

`ServerPlayerData::connection` 是弱引用，连接可能随时失效。每次使用前必须通过 `hasConnection()` 检查有效性：

```cpp
auto* player = playerManager.getPlayer(playerId);
if (player && player->hasConnection()) {
    player->send(ir::IrPacket{ ir::play::SetTime{...} });  // 走 IR，旧 send(u8*,size) 12 字节头漏斗已删除
}
```

### 3. 传送确认流程

客户端必须确认传送，否则服务端状态不一致。发起传送后需验证确认状态：

```cpp
u32 teleportId = teleportManager.requestTeleport(playerId, x, y, z);
// 收到 ir::play::AcceptTeleportation 时验证
if (!teleportManager.confirmTeleport(playerId, packet.teleportId())) {
    // 可能是过期或伪造的确认
}
```

### 4. 区块追踪器初始化

`ServerPlayerData::chunkTracker` 需要在添加玩家时创建。`PlayerManager::addPlayer` 内部会自动创建，无需手动处理。

### 5. 时间戳单位

心跳使用毫秒时间戳，其他地方可能用 tick。转换时注意单位：`tick * 50 = ms`。

### 6. 数据包处理顺序

未登录玩家发送某些数据包会导致逻辑错误。入站 Play 包在 `MinecraftServer::routeInboundPlayPacket` + `ServerPlayRouter::handle` 中分发，需检查登录状态：

```cpp
PlayerId playerId = m_playerManager.getPlayerIdBySession(sessionId);
if (playerId == 0) {
    return;  // 未登录
}
```

### 7. 日光时间 API 选择（TimeManager）

`dayTime()` 返回无边界累积值，可能超过 24000。天体计算、时间显示、睡眠检测等场景应使用 `dayTimeOfDay()` 获取归一化的一天内时间 (0-23999)。

### 8. UUID 查找（PlayerManager）

`findByUuid(uuid)` 方法用于根据玩家 UUID 字符串查找玩家数据。这在成就系统中特别有用，当事件携带 UUID 而非 PlayerId 时（如 `CuredZombieVillagerEvent`），需要通过 UUID 查找玩家。

### 9. 封禁列表大小写敏感性

`BannedPlayerList::isNameBanned()` 和 `WhitelistManager::isNameWhitelisted()` 都是大写小不敏感的，检查时会忽略大小写。

### 10. OP 权限等级含义

OpLevel 枚举参考 MC 1.16.5：
- `Normal (0)`: 普通玩家，无特殊权限
- `Moderator (1)`: 管理员，绕过出生点保护
- `GameMaster (2)`: 游戏管理员，使用命令方块、调试棒等（默认 OP 等级）
- `Admin (3)`: 高级管理员，管理其他玩家、使用危险命令
- `Owner (4)`: 服务器所有者，所有权限

单机主机作弊提升：`applyOwnerCheatsBoost` 在 OP 列表等级之上叠加「主机 + 开启作弊 → Owner」的运行时判定（不写 `ops.json`）。命令分发与登录权限解析统一走 `MinecraftServer::resolveOpLevel`，`IntegratedServer` override 之；专用服务器继承默认实现（仅读 OP 列表）。

### 11. 载具移动速度验证（ServerPlayRouter ServerboundMoveVehicle 分支）

`ServerPlayRouter` 的 `ServerboundMoveVehicle` 分支中有速度验证防止作弊，`MAX_VEHICLE_SPEED_SQ = 100.0`，超过此速度的移动数据包会被拒绝，同时发送 `ir::play::ClientboundMoveVehicle` 校正包将客户端载具位置恢复到服务端已知位置，防止客户端与服务端脱节。（此逻辑从已删除的 `PacketHandler::handleMoveVehicle` 迁入，旧 `VehicleMovePacket` 已由 IR `ServerboundMoveVehicle`/`ClientboundMoveVehicle` 取代。）
