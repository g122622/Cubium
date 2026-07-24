# Network Module

网络模块提供 Minecraft 客户端-服务器架构的网络通信基础设施。

> **架构已切换**：本目录已从旧的自研 1.16.5 协议迁移到基于"协议无关 IR + 多后端 codec"
> 的新架构（Java 1.21.11 线协议 + 基岩 stub）。新层（`buffer/`+`codec/`+`protocol/`+`ir/`+
> `transport/`+`pipeline/`+`backend/`）是主架构；旧的 `connection/` 子目录已**整体删除**，
> `packet/` 收缩为 Phase6 桥接包（仍接 live 代码，待 Phase6 迁移完成后删除）。
>
> - 新架构（主）：`buffer/` `codec/` `protocol/` `ir/` `transport/` `pipeline/` `backend/`
> - 旧 `packet/` Phase6 桥接包（保留中，调用方逐步迁移）：`packet/`
> - 旧 `connection/` 子目录：**已删除**

## 目录结构

```
network/
├── buffer/                              # 字节缓冲层（新架构）
│   ├── ByteBuf.hpp/cpp                  # 可读写字节缓冲基类
│   ├── Endian.hpp/cpp                   # 字节序工具
│   ├── NbtIo.hpp/cpp                    # NBT 读写
│   ├── RegistryByteBuf.hpp/cpp          # 携带注册表上下文的 ByteBuf（用于 codec 解析）
│   └── README.md
├── codec/                               # 流式 codec（新架构）
│   ├── StreamCodec.hpp                  # codec 概念/接口
│   ├── StreamCodecs.hpp                 # 基础 codec（VarInt、字节数组等）
│   ├── IdDispatchCodec.hpp              # 按 ID 分发的 codec 路由
│   └── README.md
├── protocol/                            # 协议元数据（新架构）
│   ├── ConnectionProtocol.hpp           # 协议枚举（Handshake/Login/Status/Configuration/Play）
│   ├── PacketFlow.hpp                   # 包方向（C→S / S→C）
│   ├── PacketType.hpp                   # 包类型抽象
│   ├── ProtocolInfo.hpp                 # 协议信息（版本、ID 表）
│   ├── ProtocolInfoBuilder.hpp          # 协议信息构建器
│   ├── GameActions.hpp / TitleActions.hpp # 协议级动作枚举
│   └── README.md
├── ir/                                  # 协议无关 IR 包定义（新架构核心）
│   ├── IrPacketBase.hpp                 # IR 包基类
│   ├── IrPacket.hpp                     # 顶层 IrPacket variant（含阶段变体）
│   ├── ItemStackBridge.hpp/cpp          # ItemStack ↔ IR 组件桥接
│   ├── packets/                         # 各阶段 IR 包
│   │   ├── handshake/HandshakePackets.hpp
│   │   ├── login/LoginPackets.hpp
│   │   ├── status/StatusPackets.hpp
│   │   ├── configuration/ConfigurationPackets.hpp
│   │   └── play/PlayPackets.hpp + PlayPacketsExtended.hpp
│   └── README.md
├── transport/                           # 传输层（新架构，取代旧 connection/）
│   ├── ITransport.hpp                   # 传输抽象接口
│   ├── Endpoint.hpp                     # 传输端点标识
│   ├── DeliveryHint.hpp                 # 投递语义提示
│   ├── LocalTransport.hpp/cpp           # 同进程零拷贝 IR 包传输（取代旧 LocalEndpoint/LocalConnectionPair）
│   ├── TcpTransport.hpp/cpp             # asio TCP + VarInt21 长度前缀帧化
│   ├── RakNetTransport.hpp              # 基岩 RakNet stub
│   └── README.md
├── pipeline/                            # 连接管线（新架构）
│   ├── Connection.hpp / Connection.inl  # pipeline::Connection<RegistryByteBuf> 门面（Wire/Local 双模）
│   ├── CipherHandlers.hpp/cpp           # 加密 handler
│   ├── CompressionHandlers.hpp/cpp      # 压缩 handler
│   ├── VarintFraming.hpp/cpp            # VarInt21 长度前缀帧化 handler
│   ├── ProtocolSwapHandler.hpp/cpp      # 协议切换 handler
│   ├── ProtocolTableSet.hpp             # 协议表集合
│   └── README.md
├── backend/                             # 协议后端（新架构）
│   ├── IProtocolBackend.hpp             # 后端抽象
│   ├── java/                            # Java 1.21.11 线协议后端
│   │   ├── JavaBackend.hpp/cpp
│   │   ├── JavaProtocolTables.hpp/cpp
│   │   └── codecs/                      # Java wire codec
│   ├── bedrock/BedrockBackend.hpp       # 基岩 stub
│   └── README.md
├── crypto/                              # 加密原语（新架构，Phase2 决策落地点）
├── packet/                              # 【Phase6 桥接包】旧 1.16.5 自研协议残留，仍接 live 代码
│   ├── Packet.hpp/cpp                   # 旧数据包基类
│   ├── PacketSerializer.hpp/cpp         # 旧二进制序列化（VarInt/VarLong）
│   ├── PacketDeserializer.hpp/cpp       # 旧二进制反序列化
│   ├── ProtocolPackets.hpp              # 核心协议包（登录、移动、区块、聊天、BlockUpdate 等）
│   ├── EntityPackets.hpp/cpp            # 实体同步包（Spawn/Move/Destroy/Metadata/EntityStatus 等）
│   ├── EntityMetadataSerializer.hpp/cpp # 实体元数据序列化（MC 1.16.5 格式）
│   ├── InventoryPackets.hpp/cpp         # 背包/容器包（含 WindowPropertyPacket、ContainerClickPacket）
│   ├── ContainerPacketHandler.hpp/cpp   # 容器包处理器（live，IntegratedServer/ContainerManager 使用）
│   ├── MapDataPacket.hpp/cpp            # 地图数据包
│   └── README.md
└── sync/                                # 同步层（旧，区块序列化）
    ├── Sync.hpp
    ├── ChunkSync.hpp/cpp                # 区块同步管理（序列化、视距、玩家跟踪）
    └── README.md
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Game Logic (Server/Client)                          │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │ send(ir::IrPacket) / onPacket(listener)
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                     pipeline/Connection<RegistryByteBuf>                    │
│             (Wire/Local 双模门面，挂载 codec/compress/cipher handler)         │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │
          ┌───────────────────────┼───────────────────────┐
          │                       │                       │
          ▼                       ▼                       ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   transport/     │    │     ir/          │    │   backend/       │
│ LocalTransport   │    │ IrPacket variant │    │ JavaBackend      │
│ TcpTransport     │    │ (阶段+叶子包)     │    │ BedrockBackend   │
└────────┬─────────┘    └────────┬─────────┘    └────────┬─────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   buffer/        │    │     codec/       │    │   protocol/      │
│ ByteBuf/NbtIo    │    │ StreamCodec      │    │ ProtocolInfo     │
│ RegistryByteBuf  │    │ IdDispatchCodec  │    │ PacketFlow/Type  │
└──────────────────┘    └──────────────────┘    └──────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│            packet/ (Phase6 桥接包，旧 1.16.5 协议残留，仍接 live)            │
│   Packet / PacketSerializer / ProtocolPackets / EntityPackets /             │
│   InventoryPackets / ContainerPacketHandler / ...         │
└─────────────────────────────────────────────────────────────────────────────┘
```

**模块交互**：
- 新架构主路径：业务代码 → `pipeline::Connection::send(ir::IrPacket)` → codec 编码 → transport 发送
- `ir/` 是协议无关的包数据模型；`backend/` 提供 Java/基岩 wire codec；`transport/` 负责实际字节流收发
- `packet/` 是 Phase6 桥接层，仍被 `ContainerPacketHandler` 等 live 路径使用，新代码不应再依赖
- `sync/` 依赖 `packet/` 进行区块数据序列化（Phase6 待迁移）

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
| `server/core/ConnectionManager.hpp` | 服务端 IR 发送门面，封装 `ir::IrPacket` 发送/广播 |
| `server/network/ServerNetwork.hpp` | 服务端网络门面（accept + 管理连接），持有 `ServerClientConnection` |
| `server/network/ServerPlayRouter.hpp` | 入站 Play 包分发器（`std::visit` over `ir::PlayPacket`） |
| `server/core/PlayerManager.hpp` | 玩家管理器，包含 ChunkSyncManager |
| `server/player/ServerPlayer.hpp` | 服务端玩家持有连接引用 |
| `server/sync/ChunkSendManager.hpp` | 服务端区块发送管理 |
| `client/network/ClientNetwork.hpp` | 客户端网络通信门面（取代旧 `NetworkClient`） |
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

### 2. VarInt 编码长度不固定（packet/ Phase6 桥接包）

> 此坑仅适用于 `packet/` 中保留的旧 1.16.5 序列化路径（`PacketSerializer`）。新架构走 `buffer/`+`codec/` 的 `StreamCodecs`。

VarInt 使用可变长度编码，在计算包大小时不能假设固定长度。负数编码后字节数更多。

```cpp
// 错误：假设 VarInt 总是固定字节
size_t size = 1 + 5;  // 可能不够

// 正确：使用 PacketSerializer 动态计算
PacketSerializer ser;
ser.writeVarInt(value);
size_t actualSize = ser.size();
```

### 3. KeepAlivePacket / CommandTreePacket 旧封装约定（已弃用）

> 以下描述的是旧 `packet/` 自研协议的封装约定。新架构（`ir/`+`pipeline/`）不再使用 12 字节头与 `encapsulatePacket` 漏斗，KeepAlive/CommandTree 已走 IR 路径。仅当仍在维护 Phase6 桥接包时参考。

- 旧 `KeepAlivePacket::deserialize()` 期望完整包（12 字节头 + 8 字节时间戳），处理心跳响应时不要先剥掉头部。
- 旧 `CommandTreePacket` 只序列化包体，须用 `ConnectionManager::encapsulatePacket()` 恰好包装一次，客户端在 `handleCommandTree()` 前剥离外部网络头。

### 4. BlockUpdatePacket 直接发送（Phase6 桥接包，live）

不要直接从服务器应用程序代码发送 `BlockUpdatePacket`。`ServerWorld::setOnBlockChanged()` 供给 `BlockUpdateSyncManager`，同坐标去重和 tick 结束刷新必须保持集中化。

### 5. ContainerPacketHandler 活动菜单指针（Phase6 桥接包，live）

`ContainerPacketHandler::handleContainerClick()` 依赖存储在集成服务器菜单玩家上的活动菜单指针。
- 打开时保持 `getMenuPlayer().setOpenContainerMenu(...)`
- 关闭时 `clearOpenContainerMenu()`
- 否则客户端点击会在到达菜单前被丢弃

### 6. 玩家背包同步（Phase6 桥接包，live）

玩家背包同步必须使用 `PlayerInventoryPacket`。`ContainerContentPacket` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。

### 7. 创造模式物品库写回（Phase6 桥接包，live）

创造模式物品库取物统一走 `ContainerClickPacket`（`ClickAction::Clone`，虚拟槽 slotIndex = `ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex`），服务端 `ItemPickerMenu::clicked` 拦截虚拟索引 clone 到光标，光标经 `ContainerContentPacket` 末尾 carried 字段回传。不再有独立的创造写回包。

### 8. 容器窗口属性同步 WindowPropertyPacket（Phase6 桥接包，live）

熔炉燃烧/熔炼进度等动态数据经 `WindowPropertyPacket`（windowId + property:i16 + value:i16）下推。服务端 `IntegratedServer::tick` 每 tick 对打开的熔炉菜单调 `FurnaceContainer::syncProgressFromEntity()` 把实体值刷进 tracked int 独立存储，再 `detectAndSendChanges()` 检测变化，经 `addIntListener` 注册的监听器发 `WindowPropertyPacket`。客户端回调 `FurnaceContainer::setTrackedInt(property, value)` 写入，`FurnaceScreen` 读 `getLitProgress()`/`getBurnProgress()` 驱动火焰/箭头动画。客户端无熔炉方块实体，进度只能走此通道，不能直接读实体。

### 9. 区块序列化的光照数据

区块段序列化包含天空光照和方块光照数据，必须正确处理 NibbleArray 格式（每方块 4 位）：
- 天空光照: 2048 字节 (4096 方块 / 2)
- 方块光照: 2048 字节

### 10. 实体元数据序列化

实体元数据使用 MC 1.16.5 格式，结束时必须写入 **0xFF 结束标记**，否则客户端解析失败。

### 11. 区块视图距离计算

`ChunkView::getChunksInView()` 返回**正方形区域**，不是圆形。视距 n 表示以玩家为中心，半径 n 的正方形区域，区块数量 = (2n + 1)²。

### 12. EntityStatusPacket 状态枚举值与 MC 原版一致（Phase6 桥接包，live）

`EntityStatusPacket::Status` 枚举值必须与 Minecraft 原版 EntityStatus byte 值严格对应（如 `IronGolemAttack = 4`、`IronGolemHoldRose = 11`、`IronGolemStopRose = 34`）。新增实体状态时必须查阅原版协议确认正确的数值，不能自行分配。
