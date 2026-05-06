# Network Module

网络模块提供 Minecraft 客户端-服务器架构的网络通信基础设施，包括连接管理、数据包序列化和区块同步功能。

## 目录结构

```
network/
├── connection/                  # 连接管理层
│   ├── Connection.hpp           # 统一头文件
│   ├── IServerConnection.hpp    # 服务端连接接口
│   ├── LocalConnection.hpp/cpp  # 本地进程内连接
│   └── LocalServerConnection.hpp/cpp  # 本地连接适配器
├── packet/                      # 数据包层
│   ├── Packet.hpp/cpp           # 数据包基类
│   ├── PacketSerializer.hpp/cpp # 二进制序列化器
│   ├── PacketModule.hpp         # 模块统一头文件
│   ├── ProtocolPackets.hpp      # 协议包定义
│   ├── EntityPackets.hpp/cpp    # 实体同步包
│   ├── EntityMetadataSerializer.hpp/cpp  # 实体元数据序列化
│   ├── InventoryPackets.hpp/cpp # 背包/创造库存包
│   ├── ContainerPacketHandler.hpp/cpp    # 容器包处理器
│   ├── RecipePackets.hpp        # 配方同步包
│   ├── GameStateChangePacket.hpp/cpp     # 游戏状态包
│   ├── PlayerAbilitiesPacket.hpp/cpp     # 玩家能力包
│   ├── BlockBreakAnimPacket.hpp/cpp      # 方块破坏动画包
│   ├── SetPassengersPacket.hpp/cpp       # 乘客列表同步包
│   ├── TitlePacket.hpp/cpp               # 标题显示包
└── sync/                        # 同步层
    ├── Sync.hpp                 # 统一头文件
    └── ChunkSync.hpp/cpp        # 区块同步管理
```

## 子模块详解

### connection/ - 连接管理层

提供网络连接的抽象接口，支持 TCP 远程连接和本地进程内通信。

#### IServerConnection.hpp

```cpp
// 服务端连接接口
class IServerConnection {
public:
    virtual ~IServerConnection() = default;
    virtual void send(const u8* data, size_t size) = 0;
    virtual void disconnect(const String& reason = "") = 0;
    [[nodiscard]] virtual bool isConnected() const = 0;
    [[nodiscard]] virtual String identifier() const = 0;
    [[nodiscard]] virtual ConnectionType type() const = 0;
};

enum class ConnectionType : u8 { Tcp, Local };
using ConnectionPtr = std::shared_ptr<IServerConnection>;
```

**职责**:
- 定义服务端连接的抽象接口
- 支持 TCP 和 Local 两种连接类型
- 提供 `send()`、`disconnect()`、`isConnected()` 等基础方法

**关键设计**:
- 使用 `ConnectionPtr` 共享指针管理连接生命周期
- 连接标识符用于日志和调试

#### LocalConnection.hpp/cpp

```cpp
// 本地连接端点 - 线程安全的消息队列
class LocalEndpoint {
public:
    void send(const u8* data, size_t size);
    bool receive(std::vector<u8>& outData);           // 非阻塞
    bool receiveWait(std::vector<u8>& outData, u32 timeoutMs = 0);  // 阻塞
    bool hasData() const;
    void connectTo(LocalEndpoint* remote);
    void disconnect();
    bool isConnected() const;
};

// 本地连接对
class LocalConnectionPair {
public:
    void connect();
    void disconnect();
    LocalEndpoint& clientEndpoint();
    LocalEndpoint& serverEndpoint();
};
```

**职责**:
- 实现进程内通信，用于 IntegratedServer（单人游戏）
- 类似 MC Java 的 LocalChannel 机制
- 使用互斥锁和条件变量实现线程安全

**关键设计**:
- `LocalConnectionPair` 管理一对相互连接的端点
- 客户端使用 `clientEndpoint()`，服务端使用 `serverEndpoint()`
- 支持阻塞和非阻塞接收模式

#### LocalServerConnection.hpp/cpp

```cpp
// 本地服务端连接适配器
class LocalServerConnection : public IServerConnection {
public:
    explicit LocalServerConnection(LocalEndpoint* endpoint);
    void send(const u8* data, size_t size) override;
    void disconnect(const String& reason = "") override;
    [[nodiscard]] bool isConnected() const override;
    [[nodiscard]] String identifier() const override;  // "Local:N"
    [[nodiscard]] ConnectionType type() const override; // Local
    [[nodiscard]] LocalEndpoint* endpoint() const;
};
```

**职责**:
- 将 `LocalEndpoint` 适配到 `IServerConnection` 接口
- 用于 IntegratedServer 的进程内通信

---

### packet/ - 数据包层

定义游戏协议数据包的序列化和反序列化。

#### Packet.hpp/cpp

```cpp
// 数据包类型枚举
enum class PacketType : u16 {
    // 内部控制包
    Handshake = 0, KeepAlive = 1, Disconnect = 2,
    // 客户端 -> 服务端
    LoginRequest = 100, PlayerMove = 101, TeleportConfirm = 102,
    ChatMessage = 103, BlockInteraction = 104, PlayerTryUseItemOnBlock = 105,
    // 服务端 -> 客户端
    LoginResponse = 200, PlayerSpawn = 201, PlayerDespawn = 202,
    ChunkData = 203, UnloadChunk = 204, BlockUpdate = 205,
    Teleport = 206, ChatBroadcast = 207, TimeUpdate = 208,
    GameStateChange = 209, SpawnEntity = 210, SpawnMob = 211,
    EntityMetadata = 213, EntityVelocity = 214, EntityTeleport = 215,
    EntityDestroy = 216, EntityAnimation = 217, EntityMove = 218,
    // ... 更多类型
};

// 数据包基类
class Packet {
public:
    explicit Packet(PacketType type);
    virtual ~Packet() = default;

    [[nodiscard]] virtual Result<std::vector<u8>> serialize() const = 0;
    [[nodiscard]] virtual Result<void> deserialize(const u8* data, size_t size) = 0;
    virtual size_t expectedSize() const;

    [[nodiscard]] PacketType type() const;
    [[nodiscard]] static String typeToString(PacketType type);
};
```

**职责**:
- 定义所有数据包类型的枚举
- 提供数据包的抽象基类
- 所有具体数据包继承自此类

**数据包方向**:
| 范围 | 方向 |
|------|------|
| 0-99 | 内部控制包 |
| 100-199 | 客户端 -> 服务端 |
| 200-299 | 服务端 -> 客户端 |
| 300+ | 双向包（背包等） |

#### PacketSerializer.hpp/cpp

```cpp
// 数据包序列化器
class PacketSerializer {
public:
    PacketSerializer(size_t initialCapacity = 256);
    void writeU8(u8 value);
    void writeU16(u16 value);
    void writeU32(u32 value);
    void writeI8(i8 value);
    void writeI16(i16 value);
    void writeI32(i32 value);
    void writeI64(i64 value);
    void writeF32(f32 value);
    void writeF64(f64 value);
    void writeBool(bool value);
    void writeVarInt(i32 value);
    void writeVarUInt(u32 value);
    void writeVarLong(i64 value);
    void writeString(const String& value);  // VarInt长度前缀，最大2097151字节
    void writeBytes(const u8* data, size_t size);
    std::vector<u8> buffer() const;
};

// 数据包反序列化器
class PacketDeserializer {
public:
    PacketDeserializer(const u8* data, size_t size);
    Result<u8> readU8();
    Result<u16> readU16();
    Result<u32> readU32();
    Result<i8> readI8();
    Result<i16> readI16();
    Result<i32> readI32();
    Result<i64> readI64();
    Result<f32> readF32();
    Result<f64> readF64();
    Result<bool> readBool();
    Result<i32> readVarInt();
    Result<u32> readVarUInt();
    Result<i64> readVarLong();
    Result<String> readString();  // VarInt长度前缀，最大2097151字节
    Result<std::vector<u8>> readBytes(size_t size);
    size_t remaining() const;
};
```

**职责**:
- 提供二进制数据的序列化和反序列化
- 支持 MC 协议的 VarInt/VarLong 编码
- 所有读写操作返回 `Result<T>` 进行错误处理

**关键特性**:
- VarInt 编码：可变长度整数，节省带宽
- 字符串自动长度前缀
- 边界检查防止缓冲区溢出

#### EntityPackets.hpp/cpp

实体同步相关的数据包集合：

| 包名 | 方向 | 用途 |
|------|------|------|
| `SpawnEntityPacket` | S->C | 生成非生物实体（物品、经验球等） |
| `SpawnMobPacket` | S->C | 生成 Mob 实体（动物、怪物等） |
| `EntityMetadataPacket` | S->C | 同步实体数据参数 |
| `EntityVelocityPacket` | S->C | 同步实体速度 |
| `EntityTeleportPacket` | S->C | 实体传送 |
| `EntityDestroyPacket` | S->C | 销毁实体 |
| `EntityAnimationPacket` | S->C | 实体动画（挥手、受伤等） |
| `EntityMovePacket` | S->C | 实体相对移动 |
| `EntityHeadLookPacket` | S->C | 实体头部朝向 |
| `EntityStatusPacket` | S->C | 实体状态（受伤、死亡、繁殖等） |
| `CollectItemPacket` | S->C | 物品拾取动画 |

```cpp
// 示例：生成实体包
SpawnEntityPacket packet;
packet.setEntityId(12345);
packet.setEntityTypeId("minecraft:item");
packet.setPosition(100.5f, 64.0f, -200.25f);
packet.setRotation(45.0f, 30.0f);
packet.setVelocity(100, -50, 200);
packet.setItemStack(itemStack);  // 可选：物品数据
auto data = packet.serialize();
```

#### EntityMetadataSerializer.hpp/cpp

实体元数据序列化器，用于 `EntityMetadataPacket` 和 `SpawnMobPacket`。

```cpp
class EntityMetadataSerializer {
public:
    static std::vector<u8> serialize(const EntityDataManager& manager, bool dirtyOnly = true);
    static bool deserialize(const std::vector<u8>& data, EntityDataManager& manager);
    static void serializeEntry(u16 id, const DataValue& value, std::vector<u8>& output);
};
```

**MC 1.16.5 元数据格式**:
- 每个条目：索引(1字节) + 类型ID(1字节) + 数据(变长)
- 结束标记：0xFF

**类型ID映射**:
| ID | 类型 |
|----|------|
| 0 | Byte (i8) |
| 1 | VarInt (i32) |
| 2 | Float (f32) |
| 3 | String (UTF-8) |
| 4 | TextComponent (JSON) |
| 6 | Slot (ItemStack) |
| 7 | Boolean (bool) |
| 9 | Position (BlockPos) |
| 18 | Pose (u8) |

#### InventoryPackets.hpp/cpp

背包和容器相关的数据包：

| 包名 | 方向 | 用途 |
|------|------|------|
| `ContainerContentPacket` | S->C | 同步整个容器内容 |
| `ContainerSlotPacket` | S->C | 同步单个槽位 |
| `PlayerInventoryPacket` | S->C | 同步玩家背包 |
| `ContainerClickPacket` | C->S | 容器点击操作 |
| `CloseContainerPacket` | 双向 | 关闭容器 |
| `OpenContainerPacket` | S->C | 打开容器 |
| `HotbarSelectPacket` | C->S | 快捷栏选择 |
| `HotbarSetPacket` | S->C | 快捷栏设置 |

```cpp
// 示例：容器内容同步
ContainerContentPacket packet(containerId, items);
packet.serialize(serializer);

// 示例：容器点击处理
ContainerClickPacket clickPacket;
clickPacket.setContainerId(0);
clickPacket.setSlotIndex(10);
clickPacket.setButton(0);  // 左键
clickPacket.setAction(ClickAction::Pick);
```

#### ContainerPacketHandler.hpp/cpp

容器包的服务端处理逻辑：

```cpp
class ContainerPacketHandler {
public:
    static bool handleContainerClick(Player& player, const ContainerClickPacket& packet);
    static void handleCloseContainer(Player& player, const CloseContainerPacket& packet);
    static void handleHotbarSelect(Player& player, const HotbarSelectPacket& packet);
    static ContainerContentPacket createContentPacket(const AbstractContainerMenu& menu);
    static ContainerSlotPacket createSlotPacket(const AbstractContainerMenu& menu, i32 slotIndex);
    static OpenContainerPacket createOpenContainerPacket(...);
    static RecipeListSyncPacket createRecipeListPacket();
};
```

#### RecipePackets.hpp

配方同步相关的数据包：

| 包名 | 方向 | 用途 |
|------|------|------|
| `RecipeSyncPacket` | S->C | 同步单个配方 |
| `RecipeListSyncPacket` | S->C | 批量同步配方列表 |
| `RecipeUnlockPacket` | S->C | 解锁配方通知 |
| `CraftResultPreviewPacket` | S->C | 合成结果预览 |

#### GameStateChangePacket.hpp/cpp

游戏状态变化通知：

```cpp
enum class GameStateChangeReason : u8 {
    InvalidBed = 0,          // 床无效
    EndRaining = 1,          // 雨停
    BeginRaining = 2,        // 开始下雨
    ChangeGameMode = 3,      // 游戏模式改变
    WinGame = 4,             // 胜利
    RainStrengthChange = 7,  // 降雨强度变化
    ThunderStrengthChange = 8, // 雷暴强度变化
    // ...
};

// 工厂方法
auto packet = GameStateChangePacket::beginRain();
auto packet = GameStateChangePacket::rainStrength(0.5f);
auto packet = GameStateChangePacket::gameModeChange(GameMode::Creative);
```

#### PlayerAbilitiesPacket.hpp/cpp

玩家能力同步：

```cpp
enum class PlayerAbilityFlags : u8 {
    Invulnerable = 0x01,  // 无敌
    Flying = 0x02,        // 飞行中
    CanFly = 0x04,        // 可飞行
    CreativeMode = 0x08   // 创造模式
};

class PlayerAbilitiesPacket : public Packet {
public:
    static PlayerAbilitiesPacket fromPlayer(const Player& player);
    static PlayerAbilitiesPacket fromGameMode(GameMode mode);
    bool invulnerable() const;
    bool flying() const;
    bool canFly() const;
    bool creativeMode() const;
    f32 flySpeed() const;
    f32 walkSpeed() const;
};
```

#### BlockBreakAnimPacket.hpp/cpp

方块破坏动画广播：

```cpp
class BlockBreakAnimPacket : public Packet {
public:
    EntityId breakerEntityId() const;
    const BlockPos& position() const;
    i8 stage() const;  // 0-9 表示破坏阶段，-1 表示移除

    static BlockBreakAnimPacket createUpdate(EntityId breakerId, const BlockPos& pos, u8 stage);
    static BlockBreakAnimPacket createRemove(EntityId breakerId, const BlockPos& pos);
};
```

#### SetPassengersPacket.hpp/cpp

乘客列表同步包，用于同步实体的乘客关系（如玩家骑乘矿车）：

```cpp
class SetPassengersPacket : public Packet {
public:
    SetPassengersPacket();
    explicit SetPassengersPacket(u32 entityId, const std::vector<u32>& passengerIds);

    [[nodiscard]] u32 entityId() const;           // 载具实体ID
    [[nodiscard]] const std::vector<u32>& passengerIds() const;  // 乘客实体ID列表

    void setEntityId(u32 entityId);
    void setPassengerIds(const std::vector<u32>& ids);
    void addPassengerId(u32 id);
};
```

**职责**:
- 服务端向客户端同步实体的乘客列表
- 当实体骑乘/离开载具时发送
- 客户端接收后更新 `ClientEntity` 的 `vehicleId` 和骑乘状态
- 触发音频系统的骑乘状态变化（如矿车音效）

**MC 1.16.5 参考**: `SSetPassengersPacket`
```

#### 骑乘相关数据包

##### PlayerInputPacket (C->S)

客户端发送玩家输入状态，用于骑乘控制：

```cpp
class PlayerInputPacket : public Packet {
public:
    f32 strafeSpeed() const;    // 左右移动 (-1.0 到 1.0)
    f32 forwardSpeed() const;   // 前后移动 (-1.0 到 1.0)
    bool isJumping() const;     // 跳跃状态
    bool isSneaking() const;    // 潜行状态
};
```

**参考**: MC 1.16.5 `CInputPacket`

##### MoveVehiclePacket (C->S)

客户端发送载具位置同步：

```cpp
class MoveVehiclePacket : public Packet {
public:
    f64 x() const;    f64 y() const;    f64 z() const;
    f32 yaw() const;  f32 pitch() const;
};
```

**参考**: MC 1.16.5 `CMoveVehiclePacket`

##### VehicleMovePacket (S->C)

服务端校正载具位置：

```cpp
class VehicleMovePacket : public Packet {
public:
    f64 x() const;    f64 y() const;    f64 z() const;
    f32 yaw() const;  f32 pitch() const;
};
```

**参考**: MC 1.16.5 `SMoveVehiclePacket`

##### EntityActionPacket (C->S)

客户端发送实体动作（潜行、疾跑、马跳跃等）：

```cpp
enum class EntityActionType : i32 {
    PressShiftKey = 0,      // 按下潜行键
    ReleaseShiftKey = 1,    // 释放潜行键
    StopSleeping = 2,       // 停止睡觉
    StartSprinting = 3,     // 开始疾跑
    StopSprinting = 4,      // 停止疾跑
    StartRidingJump = 5,    // 开始骑乘跳跃（马跳跃蓄力）
    StopRidingJump = 6,     // 停止骑乘跳跃（马跳跃释放）
    OpenInventory = 7,      // 打开背包
    StartFallFlying = 8     // 开始滑翔（鞘翅）
};

class EntityActionPacket : public Packet {
public:
    u32 entityId() const;
    EntityActionType action() const;
    i32 auxData() const;  // 辅助数据（如跳跃力度）
};
```

**参考**: MC 1.16.5 `CEntityActionPacket`

---

### sync/ - 同步层

管理客户端和服务器之间的世界数据同步。

#### ChunkSync.hpp/cpp

区块同步管理，负责跟踪每个玩家已加载的区块并计算需要发送/卸载的区块。

```cpp
// 区块序列化器
class ChunkSerializer {
public:
    static Result<std::vector<u8>> serializeChunk(const ChunkData& chunk);
    static std::vector<u8> serializeSection(const ChunkSection& section);
    static Result<std::unique_ptr<ChunkData>> deserializeChunk(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    static u16 calculateSectionMask(const ChunkData& chunk);
};

// 区块视图
struct ChunkView {
    ChunkCoord centerX = 0;
    ChunkCoord centerZ = 0;
    i32 viewDistance = 10;

    bool isChunkInView(ChunkCoord x, ChunkCoord z) const;
    void getChunksInView(std::vector<ChunkPos>& out) const;
    void calculateChunkDiff(const std::unordered_set<ChunkId>& currentChunks,
                            std::vector<ChunkPos>& chunksToLoad,
                            std::vector<ChunkPos>& chunksToUnload) const;
};

// 玩家区块跟踪器
class PlayerChunkTracker {
public:
    explicit PlayerChunkTracker(PlayerId playerId);
    void addLoadedChunk(ChunkCoord x, ChunkCoord z);
    void removeLoadedChunk(ChunkCoord x, ChunkCoord z);
    void updateCenter(ChunkCoord x, ChunkCoord z);
    void setViewDistance(i32 distance);
    void calculateChunkUpdates(std::vector<ChunkPos>& chunksToLoad,
                               std::vector<ChunkPos>& chunksToUnload);
};

// 区块同步管理器
class ChunkSyncManager {
public:
    std::shared_ptr<PlayerChunkTracker> getTracker(PlayerId playerId);
    void removeTracker(PlayerId playerId);
    void updatePlayerPosition(PlayerId playerId, f64 x, f64 z);
    void calculateUpdates(PlayerId playerId,
                         std::vector<ChunkPos>& chunksToLoad,
                         std::vector<ChunkPos>& chunksToUnload);
    void markChunkSent(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void markChunkUnloaded(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void getChunkSubscribers(ChunkCoord x, ChunkCoord z, std::vector<PlayerId>& out) const;
};
```

**职责**:
- `ChunkSerializer`: 区块数据二进制序列化
- `ChunkView`: 计算视距内的区块集合
- `PlayerChunkTracker`: 跟踪单个玩家的已加载区块
- `ChunkSyncManager`: 管理所有玩家的区块同步

**区块数据格式**:
```
| 字段 | 类型 | 说明 |
|------|------|------|
| x | i32 | 区块 X 坐标 |
| z | i32 | 区块 Z 坐标 |
| sectionMask | u16 | 区块段位掩码 |
| heightmap | u8[256] | 高度图 |
| biomeSize | u8 | 生物群系数据大小 |
| biomeData | bytes | 生物群系数据 |
| sections | ... | 区块段数据（按位掩码） |
```

---

## 模块整体职责

### 输入

| 来源 | 数据类型 |
|------|----------|
| 服务端 | 游戏状态、实体数据、区块数据、玩家操作 |
| 客户端 | 玩家移动、交互、背包操作 |

### 输出

| 目标 | 数据类型 |
|------|----------|
| 网络层 | 二进制数据包 |
| 游戏逻辑 | 反序列化后的数据结构 |

### 依赖项

| 模块 | 依赖内容 |
|------|----------|
| `common/core` | Types, Result, Error |
| `common/world` | ChunkData, ChunkSection, BlockPos, BiomeContainer |
| `common/entity` | EntityDataManager, Player, PlayerInventory |
| `common/item` | ItemStack |
| `common/resource` | ResourceLocation |
| `spdlog` | 日志 |

---

## 使用方法

### 发送数据包

```cpp
#include "common/network/packet/PacketModule.hpp"

// 创建数据包
mc::network::SpawnEntityPacket packet;
packet.setEntityId(12345);
packet.setEntityTypeId("minecraft:item");
packet.setPosition(100.0f, 64.0f, 200.0f);

// 序列化
auto result = packet.serialize();
if (result.success()) {
    const auto& data = result.value();
    connection->send(data.data(), data.size());
}
```

### 接收数据包

```cpp
// 从网络接收数据
std::vector<u8> buffer = receiveFromNetwork();

// 反序列化
mc::network::SpawnEntityPacket packet;
auto result = packet.deserialize(buffer.data(), buffer.size());
if (result.success()) {
    // 使用数据
    processEntity(packet.entityId(), packet.x(), packet.y(), packet.z());
}
```

### 本地连接（IntegratedServer）

```cpp
#include "common/network/connection/Connection.hpp"

// 创建本地连接对
auto pair = std::make_unique<mc::network::LocalConnectionPair>();
pair->connect();

// 服务端使用 serverEndpoint
auto serverConn = std::make_shared<mc::network::LocalServerConnection>(&pair->serverEndpoint());

// 客户端使用 clientEndpoint
pair->clientEndpoint().send(data.data(), data.size());

std::vector<u8> received;
if (pair->clientEndpoint().receive(received)) {
    // 处理接收到的数据
}
```

### 区块同步

```cpp
#include "common/network/sync/Sync.hpp"

mc::network::ChunkSyncManager syncManager;

// 玩家加入
auto tracker = syncManager.getTracker(playerId);
tracker->setViewDistance(10);

// 玩家移动
syncManager.updatePlayerPosition(playerId, playerX, playerZ);

// 计算区块更新
std::vector<mc::ChunkPos> toLoad, toUnload;
syncManager.calculateUpdates(playerId, toLoad, toUnload);

// 发送区块
for (const auto& pos : toLoad) {
    auto chunkData = world.getChunk(pos.x, pos.z);
    auto serialized = mc::network::ChunkSerializer::serializeChunk(*chunkData);
    sendToPlayer(playerId, serialized.value());
    syncManager.markChunkSent(playerId, pos.x, pos.z);
}

// 卸载区块
for (const auto& pos : toUnload) {
    sendUnloadPacket(playerId, pos.x, pos.z);
    syncManager.markChunkUnloaded(playerId, pos.x, pos.z);
}
```

---

## 容易踩的坑

### 1. VarInt 编码长度不固定

VarInt 使用可变长度编码，在计算包大小时不能假设固定长度。

```cpp
// 错误：假设 VarInt 总是 5 字节
size_t size = 1 + 5;  // 可能不够

// 正确：使用 PacketSerializer 动态计算
PacketSerializer ser;
ser.writeVarInt(value);
size_t actualSize = ser.size();
```

### 2. 数据包类型枚举范围

数据包类型有明确的方向性：
- 0-99: 内部控制
- 100-199: 客户端->服务端
- 200-299: 服务端->客户端
- 300+: 双向

添加新包时应遵循此约定。

### 3. 区块序列化的光照数据

区块段序列化包含天空光照和方块光照数据，必须正确处理 NibbleArray 格式（每方块 4 位）：

```cpp
// 天空光照: 2048 字节 (4096 方块 / 2)
// 方块光照: 2048 字节
constexpr size_t LIGHT_DATA_SIZE = NibbleArray::BYTE_SIZE * 2;  // 4096 字节
```

### 4. LocalConnection 的线程安全

`LocalEndpoint` 使用互斥锁保护队列，但 `send()` 和 `receive()` 可以在不同线程调用：

```cpp
// 线程 A：服务端发送
serverEndpoint.send(data.data(), data.size());

// 线程 B：客户端接收
clientEndpoint.receiveWait(buffer, 1000);  // 1 秒超时
```

### 5. 实体元数据序列化

实体元数据使用 MC 1.16.5 格式，结束时必须写入 0xFF 结束标记：

```cpp
// 正确：写入结束标记
output.push_back(0xFF);

// 错误：忘记结束标记会导致客户端解析失败
```

### 6. 连接断开后的操作

在连接断开后调用 `send()` 不会崩溃但也不会发送数据：

```cpp
connection->disconnect("Player quit");
connection->send(data, size);  // 安全但无效果
```

### 7. 区块视图距离计算

`ChunkView::getChunksInView()` 返回正方形区域，不是圆形：

```cpp
// 正方形区域：视距 10 时返回 21x21 = 441 个区块
// 中心在 (0, 0)，范围为 [-10, 10] x [-10, 10]
```

### 8. KeepAlivePacket 反序列化

**问题**：`KeepAlivePacket::deserialize()` 期望完整包（12 字节头 + 8 字节时间戳），服务端处理心跳响应时先剥掉头部会导致解析失败。

**解决方案**：服务端处理心跳响应时不要先剥掉头部，否则单人模式下会把正常的 KeepAlive 回复误报成 `Packet too small for keep alive`。

### 9. CommandTreePacket 封装

**问题**：`CommandTreePacket` 只序列化包体，双重封装会使内部头看起来像空 JSON 字符串。

**解决方案**：
- 服务器代码必须用 `ConnectionManager::encapsulatePacket()` 恰好包装一次
- 客户端代码必须在调用 `handleCommandTree()` 之前剥离外部网络头

### 10. BlockUpdatePacket 直接发送

**问题**：直接从服务器应用程序代码发送 `BlockUpdatePacket` 会绕过去重和批处理逻辑。

**解决方案**：不要直接从服务器应用程序代码发送 `BlockUpdatePacket`。`ServerWorld::setOnBlockChanged()` 现在供给 `BlockUpdateSyncManager`；同坐标去重和 tick 结束刷新必须保持集中化。

### 11. ContainerPacketHandler 活动菜单指针

**问题**：`ContainerPacketHandler::handleContainerClick()` 依赖存储在集成服务器菜单玩家上的活动菜单指针。

**解决方案**：
- 打开时保持 `getMenuPlayer().setOpenContainerMenu(...)`
- 关闭时 `clearOpenContainerMenu()`
- 否则客户端点击会在到达菜单前被丢弃

### 12. 玩家背包同步

**问题**：使用错误的数据包同步玩家背包。

**解决方案**：玩家背包同步必须使用 `PlayerInventoryPacket`。`ContainerContentPacket` 只保留给真正打开的容器菜单；玩家物品栏刷新、拾取同步和 `/clear` 这类操作都应走玩家背包包。

### 13. 创造模式物品库写回

**问题**：`CreativeScreen` 负责本地搜索、滚动和槽位编辑，复用普通容器点击包会导致协议不匹配。

**解决方案**：创造模式物品库写回必须使用 `CreativeInventoryActionPacket`，不要复用普通容器点击包。

---

## 涉及的测试用例

| 测试文件 | 测试内容 |
|----------|----------|
| `tests/network/LocalServerConnectionTest.cpp` | 本地连接测试 |
| `tests/network/EntityPacketsTest.cpp` | 实体数据包序列化/反序列化测试 |

### LocalServerConnectionTest.cpp 测试项

- `BasicSendReceive`: 基本发送接收
- `Disconnect`: 断开连接
- `Identifier`: 连接标识符格式
- `SendWhenDisconnected`: 断开后发送的安全性
- `NullEndpoint`: 空端点处理
- `UseThroughInterface`: 通过接口使用

### EntityPacketsTest.cpp 测试项

- `SpawnEntityPacket.SerializeDeserialize`: 实体生成包序列化
- `SpawnMobPacket.SerializeDeserialize`: Mob 生成包序列化
- `EntityVelocityPacket.SerializeDeserialize`: 速度包序列化
- `EntityTeleportPacket.SerializeDeserialize`: 传送包序列化
- `EntityDestroyPacket.SerializeDeserialize`: 销毁包序列化
- `EntityAnimationPacket.AllAnimationTypes`: 所有动画类型
- `EntityMovePacket.SerializeDeserialize`: 移动包序列化
- `EntityHeadLookPacket.SerializeDeserialize`: 头部朝向包
- `EntityStatusPacket.AllStatusTypes`: 所有状态类型
- `EntityMetadataPacket.SerializeDeserialize`: 元数据包
- 错误处理测试：数据不足时的反序列化失败

---

## 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         Server / Client                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐                    │
│  │   sync/         │    │   packet/       │                    │
│  │                 │    │                 │                    │
│  │ ChunkSerializer │◄──►│ Packet          │◄─── Game Logic     │
│  │ ChunkSyncManager│    │ PacketSerializer│                    │
│  │ PlayerTracker   │    │ *Packet types   │                    │
│  └────────┬────────┘    └────────┬────────┘                    │
│           │                      │                              │
│           │                      │                              │
│           └──────────┬───────────┘                              │
│                      │                                          │
│                      ▼                                          │
│           ┌─────────────────────┐                               │
│           │   connection/       │                               │
│           │                     │                               │
│           │ IServerConnection   │◄── Abstract Interface         │
│           │ LocalConnection     │◄── IPC (IntegratedServer)     │
│           │ LocalServerConnection│                              │
│           │ TcpConnection       │◄── Network (StandaloneServer) │
│           └──────────┬──────────┘                               │
│                      │                                          │
└──────────────────────┼──────────────────────────────────────────┘
                       │
                       ▼
              ┌────────────────┐
              │   Network      │
              │   (TCP/Local)  │
              └────────────────┘
```

---

## 扩展指南

### 添加新数据包

1. 在 `PacketType` 枚举中添加新类型（遵循方向约定）
2. 创建新的数据包类继承 `Packet`
3. 实现 `serialize()` 和 `deserialize()` 方法
4. 在 `ProtocolPackets.hpp` 或相应文件中添加类定义
5. 编写单元测试

### 添加新连接类型

1. 继承 `IServerConnection` 接口
2. 实现 `send()`、`disconnect()`、`isConnected()` 等方法
3. 在 `Connection.hpp` 中添加新类的包含

### 添加新同步逻辑

1. 在 `sync/` 目录下创建新模块
2. 使用 `ChunkSyncManager` 作为参考模式
3. 实现跟踪器和广播逻辑
