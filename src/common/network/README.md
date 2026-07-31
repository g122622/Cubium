# Network Module

网络模块提供 Minecraft 客户端-服务器架构的网络通信基础设施。

> **架构已切换**：本目录已从旧的自研 1.16.5 协议迁移到基于"协议无关 IR + 多后端 codec"
> 的新架构（Java 1.21.11 线协议 + 基岩 stub）。新层（`buffer/`+`codec/`+`protocol/`+`ir/`+
> `transport/`+`pipeline/`+`backend/`）是主架构；旧的 `connection/` 子目录已**整体删除**，
> 旧的 `packet/` 子目录也已**整体删除**（Phase8 终态：零旧 packet 代码）。
> 原 `packet/` 中仍被 live 代码引用的工具类（`PacketSerializer`/`PacketDeserializer`/
> `EntityMetadataSerializer`）已 `git mv` 到 `codec/`，继续可用。
>
> - 新架构（主）：`buffer/` `codec/` `protocol/` `ir/` `transport/` `pipeline/` `backend/`
> - 旧 `connection/` 子目录：**已删除**
> - 旧 `packet/` 子目录：**已删除**（工具类已迁入 `codec/`，死代码已清零）

## 目录结构

```
network/
├── buffer/                              # 字节缓冲层（新架构）
│   ├── ByteBuf.hpp/cpp                  # 可读写字节缓冲基类
│   ├── Endian.hpp/cpp                   # 字节序工具
│   ├── NbtIo.hpp/cpp                    # NBT 读写
│   ├── RegistryByteBuf.hpp/cpp          # 携带注册表上下文的 ByteBuf（用于 codec 解析）
│   └── README.md
├── codec/                               # 流式 codec（新架构）+ 自 packet/ 迁入的工具类
│   ├── StreamCodec.hpp                  # codec 概念/接口
│   ├── StreamCodecs.hpp                 # 基础 codec（VarInt、字节数组等）
│   ├── IdDispatchCodec.hpp              # 按 ID 分发的 codec 路由
│   ├── PacketSerializer.hpp/cpp         # 二进制序列化（VarInt/VarLong，自 packet/ 迁入，sync/ 区块序列化仍在用）
│   ├── PacketDeserializer.hpp/cpp       # 二进制反序列化（自 packet/ 迁入）
│   ├── EntityMetadataSerializer.hpp/cpp # 实体元数据序列化（MC 1.21.11 格式，自 packet/ 迁入，产出 ir::play::SetEntityData 的 packedItems）
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
│   │   ├── mappings/                    # 项目内部 id ↔ Java wire id 双向映射（协议对齐层）
│   │   │   ├── JavaItemIdMap.hpp/cpp    # ItemStack wire itemId 翻译
│   │   │   └── JavaBlockStateIdMap.hpp/cpp  # level_chunk_with_light palette 翻译
│   │   ├── codecs/                      # Java wire codec
│   │   ├── handshake/                   # 握手/Login 编解码
│   │   └── generated/                   # 烘焙产物（构建期重生成，不入 git）
│   ├── bedrock/BedrockBackend.hpp       # 基岩 stub
│   └── README.md
├── crypto/                              # 加密原语（新架构，Phase2 决策落地点）
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
```

> 注：原 `packet/` 子目录已整体删除（Phase8 终态），工具类已迁入 `codec/`（见上表）。

**模块交互**：
- 新架构主路径：业务代码 → `pipeline::Connection::send(ir::IrPacket)` → codec 编码 → transport 发送
- `ir/` 是协议无关的包数据模型；`backend/` 提供 Java/基岩 wire codec；`transport/` 负责实际字节流收发
- `sync/` 依赖 `codec/`（`PacketSerializer`，自 packet/ 迁入）进行区块数据序列化

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

> 新架构走 `buffer/`+`codec/` 的 `StreamCodecs`；旧路径的 `PacketSerializer` 已自 `packet/` 迁入 `codec/`，仍被 `sync/` 区块序列化使用。

VarInt 使用可变长度编码，在计算包大小时不能假设固定长度。负数编码后字节数更多。

```cpp
// 错误：假设 VarInt 总是固定字节
size_t size = 1 + 5;  // 可能不够

// 正确：使用 PacketSerializer 动态计算（codec/PacketSerializer.hpp）
PacketSerializer ser;
ser.writeVarInt(value);
size_t actualSize = ser.size();
```

### 2. BlockUpdate 直接发送

不要直接从服务器应用程序代码发送 `ir::play::BlockUpdate`。`ServerWorld::setOnBlockChanged()` 供给 `BlockUpdateSyncManager`，同坐标去重和 tick 结束刷新必须保持集中化。

### 3. 本地容器点击的活动菜单指针

原 `ContainerPacketHandler` 类已删除；本地客户端的容器点击处理已内联进 `IntegratedServer::handleContainerClickPacket`，但仍依赖存储在集成服务器菜单玩家上的活动菜单指针。
- 打开时保持 `getMenuPlayer().setOpenContainerMenu(...)`
- 关闭时 `clearOpenContainerMenu()`
- 否则客户端点击会在到达菜单前被丢弃

### 4. 玩家背包同步

玩家背包同步必须使用 `ir::play::SetPlayerInventory`。`ir::play::ContainerSetContent` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。

### 5. 创造模式物品库写回

创造模式物品库取物统一走 `ir::play::ContainerClick`（`ClickAction::Clone`，虚拟槽 slotIndex = `ItemPickerMenu::PALETTE_VIRTUAL_BASE + visibleIndex`），服务端 `ItemPickerMenu::clicked` 拦截虚拟索引 clone 到光标，光标经 `ir::play::ContainerSetContent` 末尾 carried 字段回传。不再有独立的创造写回包。

### 6. 容器窗口属性同步 ir::play::ContainerSetData

熔炉燃烧/熔炼进度等动态数据经 `ir::play::ContainerSetData`（windowId + property:i16 + value:i16）下推。服务端 `IntegratedServer::tick` 每 tick 对打开的熔炉菜单调 `FurnaceContainer::syncProgressFromEntity()` 把实体值刷进 tracked int 独立存储，再 `detectAndSendChanges()` 检测变化，经 `addIntListener` 注册的监听器发 `ir::play::ContainerSetData`。客户端回调 `FurnaceContainer::setTrackedInt(property, value)` 写入，`FurnaceScreen` 读 `getLitProgress()`/`getBurnProgress()` 驱动火焰/箭头动画。客户端无熔炉方块实体，进度只能走此通道，不能直接读实体。

### 7. 区块序列化的光照数据

区块段序列化包含天空光照和方块光照数据，必须正确处理 NibbleArray 格式（每方块 4 位）：
- 天空光照: 2048 字节 (4096 方块 / 2)
- 方块光照: 2048 字节

### 8. 实体元数据序列化

实体元数据使用 MC 1.21.11 格式（`codec/EntityMetadataSerializer.hpp/cpp`）：每个条目 `byte(index) + VarInt(serializerId) + value`，结束时写 **0xFF 结束标记**，否则客户端解析失败。serializerId 取值对齐 `SynchedEntityData`（Byte=0/Int=1/Long=2/Float=3/String=4/ItemStack=7/Boolean=8/Rotations=9/BlockPos=10）。

### 9. 区块视图距离计算

`ChunkView::getChunksInView()` 返回**正方形区域**，不是圆形。视距 n 表示以玩家为中心，半径 n 的正方形区域，区块数量 = (2n + 1)²。

### 10. EntityStatus 状态枚举值与 MC 原版一致

`mc::network::EntityStatus` 枚举（定义于 `protocol/EntityEvents.hpp`，自旧 `EntityStatusPacket::Status` 迁出）的取值必须与 Minecraft 原版 EntityStatus byte 值严格对应（如 `IronGolemAttack = 4`、`IronGolemHoldRose = 11`、`IronGolemStopRose = 34`）。新增实体状态时必须查阅原版协议确认正确的数值，不能自行分配。原 `EntityStatusPacket`/`EntityAnimationPacket` 类已删除，业务侧改用 `mc::network::EntityStatus` / `mc::network::EntityAnimation`（同文件）配合 IR 路径。
