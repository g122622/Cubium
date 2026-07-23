# Network Module

网络模块提供 Minecraft 客户端-服务器架构的网络通信基础设施，包括连接管理、数据包序列化和区块同步功能。

## 目录结构

```
network/
├── connection/                          # 连接管理层
│   ├── Connection.hpp                   # 统一头文件
│   ├── IServerConnection.hpp            # 服务端连接抽象接口
│   ├── LocalConnection.hpp/cpp          # 本地连接端点（线程安全消息队列，用于 IntegratedServer）
│   ├── LocalServerConnection.hpp/cpp    # 本地连接适配器（将 LocalEndpoint 适配到 IServerConnection）
│   └── README.md
├── packet/                              # 数据包层
│   ├── Packet.hpp/cpp                   # 数据包基类定义
│   ├── PacketSerializer.hpp/cpp         # 二进制序列化工具（支持 VarInt/VarLong）
│   ├── PacketDeserializer.hpp/cpp       # 二进制反序列化工具
│   ├── PacketModule.hpp                 # 模块统一头文件
│   ├── ProtocolPackets.hpp              # 核心协议数据包（登录、移动、区块、聊天等）
│   ├── EntityPackets.hpp/cpp            # 实体同步包（生成、移动、销毁、元数据等）
│   ├── EntityMetadataSerializer.hpp/cpp # 实体元数据序列化（MC 1.16.5 格式）
│   ├── InventoryPackets.hpp/cpp         # 背包/创造库存包
│   ├── ContainerPacketHandler.hpp/cpp   # 容器包处理器（服务端处理容器点击）
│   ├── RecipePackets.hpp                # 配方同步包
│   ├── AdvancementPackets.hpp/cpp       # 成就系统数据包
│   ├── BlockBreakAnimPacket.hpp/cpp     # 方块破坏动画广播包
│   ├── GameStateChangePacket.hpp/cpp    # 游戏状态变化通知（下雨、游戏模式等）
│   ├── PlayerAbilitiesPacket.hpp/cpp    # 玩家能力同步（飞行、无敌等）
│   ├── ServerDifficultyPacket.hpp/cpp   # 难度同步包
│   ├── DimensionPackets.hpp/cpp         # 维度切换数据包
│   ├── SpawnPositionPacket.hpp/cpp      # 世界出生点数据包
│   ├── ExplosionPacket.hpp/cpp          # 爆炸事件数据包
│   ├── TitlePacket.hpp/cpp              # 标题显示包
│   ├── BossInfoPacket.hpp/cpp           # Boss 栏同步包
│   ├── SetPassengersPacket.hpp/cpp      # 乘客列表同步包（骑乘关系）
│   ├── SleepPacket.hpp/cpp              # 睡眠状态同步包
│   ├── WorldBorderPacket.hpp/cpp        # 世界边界同步包
│   ├── CommandTreePacket.hpp/cpp        # 命令树同步包
│   ├── ExperiencePackets.hpp/cpp        # 经验值同步包
│   ├── MapDataPacket.hpp/cpp            # 地图数据包
│   ├── ParticlePacket.hpp/cpp           # 粒子效果包
│   ├── SignPackets.hpp                  # 告示牌编辑包（OpenSignEditor S→C、UpdateSign C→S）
│   └── README.md
└── sync/                                # 同步层
    ├── Sync.hpp                         # 统一头文件
    ├── ChunkSync.hpp/cpp                # 区块同步管理（序列化、视距、玩家跟踪）
    └── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Game Logic (Server/Client)                          │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │
          ┌───────────────────────┼───────────────────────┐
          │                       │                       │
          ▼                       ▼                       ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│     sync/        │    │    packet/       │    │  connection/     │
│                  │    │                  │    │                  │
│ ChunkSerializer  │◄──►│ Packet (基类)    │    │ IServerConnection│◄── 抽象接口
│ ChunkSyncManager │    │ PacketSerializer │    │ LocalConnection  │◄── 本地IPC
│ PlayerTracker    │    │ *Packets (子类)  │    │ LocalServerConn  │◄── 适配器
└────────┬─────────┘    └────────┬─────────┘    └────────┬─────────┘
         │                       │                       │
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                                 ▼
                        ┌────────────────┐
                        │  External TCP  │
                        │  (Standalone)  │
                        └────────────────┘
```

**模块交互**：
- `packet/` 是核心，所有数据包定义和序列化逻辑在此
- `sync/` 依赖 `packet/` 进行区块数据序列化
- `connection/` 是底层传输抽象，被上层 `server/network/` 使用

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块/库 | 用途 |
|---------|------|
| `common/core/Types.hpp` | 基础类型定义（u8, u16, i32, f32, std::string 等） |
| `common/core/Result.hpp` | 错误处理（Result<T>, Error, ErrorCode） |
| `common/world/chunk/ChunkData.hpp` | 区块数据结构 |
| `common/world/block/BlockPos.hpp` | 方块位置 |
| `common/entity/EntityDataManager.hpp` | 实体数据管理 |
| `common/entity/Player.hpp` | 玩家实体 |
| `common/entity/inventory/` | 背包系统 |
| `common/item/ItemStack.hpp` | 物品堆 |
| `common/resource/ResourceLocation.hpp` | 资源位置 |
| `spdlog` | 日志 |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `server/core/ConnectionManager.hpp` | 服务端连接管理，封装数据包发送 |
| `server/core/PacketHandler.hpp` | 服务端数据包处理 |
| `server/core/PlayerManager.hpp` | 玩家管理器，包含 ChunkSyncManager |
| `server/player/ServerPlayer.hpp` | 服务端玩家持有连接引用 |
| `server/network/TcpConnection.hpp` | TCP 连接实现 IServerConnection |
| `server/sync/ChunkSendManager.hpp` | 服务端区块发送管理 |
| `client/network/NetworkClient.hpp` | 客户端网络通信 |
| `client/world/ClientWorld.hpp` | 客户端世界数据接收处理 |

## 容易踩的坑

### 1. VarInt 编码长度不固定

VarInt 使用可变长度编码，在计算包大小时不能假设固定长度。负数编码后字节数更多。

```cpp
// 错误：假设 VarInt 总是固定字节
size_t size = 1 + 5;  // 可能不够

// 正确：使用 PacketSerializer 动态计算
PacketSerializer ser;
ser.writeVarInt(value);
size_t actualSize = ser.size();
```

### 2. 数据包类型枚举范围

数据包类型有明确的方向性：
- 0-99: 内部控制包
- 100-199: 客户端→服务端
- 200-299: 服务端→客户端
- 300+: 双向包

添加新包时应遵循此约定。

### 3. KeepAlivePacket 反序列化

`KeepAlivePacket::deserialize()` 期望完整包（12 字节头 + 8 字节时间戳）。服务端处理心跳响应时**不要**先剥掉头部，否则单人模式下会把正常的 KeepAlive 回复误报成 `Packet too small for keep alive`。

### 4. CommandTreePacket 封装

`CommandTreePacket` 只序列化包体，双重封装会使内部头看起来像空 JSON 字符串。
- 服务器代码必须用 `ConnectionManager::encapsulatePacket()` **恰好包装一次**
- 客户端代码必须在调用 `handleCommandTree()` 之前剥离外部网络头

### 5. BlockUpdatePacket 直接发送

不要直接从服务器应用程序代码发送 `BlockUpdatePacket`。`ServerWorld::setOnBlockChanged()` 供给 `BlockUpdateSyncManager`，同坐标去重和 tick 结束刷新必须保持集中化。

### 6. ContainerPacketHandler 活动菜单指针

`ContainerPacketHandler::handleContainerClick()` 依赖存储在集成服务器菜单玩家上的活动菜单指针。
- 打开时保持 `getMenuPlayer().setOpenContainerMenu(...)`
- 关闭时 `clearOpenContainerMenu()`
- 否则客户端点击会在到达菜单前被丢弃

### 7. 玩家背包同步

玩家背包同步必须使用 `PlayerInventoryPacket`。`ContainerContentPacket` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。

### 8. 创造模式物品库写回

创造模式物品库取物统一走 `ContainerClickPacket`（`ClickAction::Clone`，虚拟槽 slotIndex = `ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex`），服务端 `ItemPickerMenu::clicked` 拦截虚拟索引 clone 到光标，光标经 `ContainerContentPacket` 末尾 carried 字段回传。不再有独立的创造写回包。

### 9. LocalServerConnection 不持有端点所有权

`LocalServerConnection` 使用裸指针持有 `LocalEndpoint`，不管理其生命周期。确保 `LocalConnectionPair` 的生命周期长于 `LocalServerConnection`。

### 10. 区块序列化的光照数据

区块段序列化包含天空光照和方块光照数据，必须正确处理 NibbleArray 格式（每方块 4 位）：
- 天空光照: 2048 字节 (4096 方块 / 2)
- 方块光照: 2048 字节

### 11. 实体元数据序列化

实体元数据使用 MC 1.16.5 格式，结束时必须写入 **0xFF 结束标记**，否则客户端解析失败。

### 12. 区块视图距离计算

`ChunkView::getChunksInView()` 返回**正方形区域**，不是圆形。视距 n 表示以玩家为中心，半径 n 的正方形区域，区块数量 = (2n + 1)²。

### 13. EntityStatusPacket 状态枚举值与 MC 原版一致

`EntityStatusPacket::Status` 枚举值必须与 Minecraft 原版 EntityStatus byte 值严格对应（如 `IronGolemAttack = 4`、`IronGolemHoldRose = 11`、`IronGolemStopRose = 34`）。新增实体状态时必须查阅原版协议确认正确的数值，不能自行分配。
