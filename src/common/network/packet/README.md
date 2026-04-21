# Packet 模块

网络数据包模块，负责客户端与服务端之间的通信协议实现。

## 目录结构

```
src/common/network/packet/
├── Packet.hpp                     # 数据包基类定义
├── Packet.cpp                     # 数据包基类实现
├── PacketSerializer.hpp           # 序列化/反序列化工具类
├── PacketSerializer.cpp           # 序列化工具实现
├── PacketModule.hpp               # 模块统一头文件
├── ProtocolPackets.hpp            # 协议数据包定义
├── EntityPackets.hpp              # 实体相关数据包
├── EntityPackets.cpp              # 实体数据包实现
├── EntityMetadataSerializer.hpp   # 实体元数据序列化器
├── EntityMetadataSerializer.cpp   # 元数据序列化实现
├── InventoryPackets.hpp           # 背包/创造库存相关数据包
├── InventoryPackets.cpp           # 背包数据包实现
├── ContainerPacketHandler.hpp     # 容器网络处理器
├── ContainerPacketHandler.cpp     # 容器处理器实现
├── RecipePackets.hpp              # 配方同步数据包
├── BlockBreakAnimPacket.hpp       # 方块破坏动画包
├── BlockBreakAnimPacket.cpp       # 破坏动画包实现
├── GameStateChangePacket.hpp      # 游戏状态变化包
├── GameStateChangePacket.cpp      # 状态变化包实现
├── PlayerAbilitiesPacket.hpp      # 玩家能力同步包
├── PlayerAbilitiesPacket.cpp      # 玩家能力包实现
├── DimensionPackets.hpp           # 维度切换数据包
└── DimensionPackets.cpp           # 维度数据包实现
```

## 文件详细说明

### 核心文件

#### Packet.hpp / Packet.cpp

**职责**: 数据包基类定义和基础实现

**主要内容**:
- `PacketType` 枚举: 定义所有数据包类型ID
  - 内部控制包: Handshake, KeepAlive, Disconnect
  - 客户端→服务端: LoginRequest, PlayerMove, TeleportConfirm, ChatMessage, BlockInteraction 等
  - 服务端→客户端: LoginResponse, PlayerSpawn, ChunkData, UnloadChunk, BlockUpdate 等
  - 实体同步包: SpawnEntity, SpawnMob, EntityMetadata, EntityVelocity 等
  - 背包相关包: ContainerContent, ContainerSlot, ContainerClick, CreativeInventoryAction 等

- `PacketHeader` 结构: 12字节固定头部
  - size: 数据包总大小
  - type: 数据包类型
  - flags: 标志位 (压缩、加密、可靠传输)
  - reserved/padding: 保留字段

- `Packet` 抽象基类: 所有数据包的基类
  - `serialize()`: 序列化接口
  - `deserialize()`: 反序列化接口
  - `expectedSize()`: 预期大小 (用于预分配)

- `KeepAlivePacket`: 心跳包（按完整包处理，包含 12 字节包头和 8 字节时间戳）
- `DisconnectPacket`: 断开连接包
- 其余具体数据包（包括 `CommandTreePacket`）只负责包体序列化，外层 12 字节包头由 `ConnectionManager::encapsulatePacket()` 统一添加

#### PacketSerializer.hpp / PacketSerializer.cpp

**职责**: 二进制数据序列化与反序列化工具

**主要内容**:
- `NetworkEndian`: 网络字节序转换工具
  - `hostToNetwork16/32/64()`: 主机序转网络序
  - `networkToHost16/32/64()`: 网络序转主机序

- `PacketSerializer`: 序列化器 (写入)
  - 基本类型写入: `writeU8/U16/U32/U64/I8/I16/I32/I64/F32/F64/Bool`
  - 字符串写入: `writeString/writeStringView`
  - 字节数组写入: `writeBytes`
  - Minecraft VarInt/VarLong: `writeVarInt/writeVarLong/writeVarUInt/writeVarULong`

- `PacketDeserializer`: 反序列化器 (读取)
  - 对应的读取方法
  - `readBytesInto()`: 零拷贝字节读取
  - 状态查询: `remaining()`, `hasRemaining()`

#### PacketModule.hpp

**职责**: 模块统一入口头文件

**主要内容**:
- 包含所有数据包头文件，方便一次性导入:
  - `Packet.hpp`
  - `PacketSerializer.hpp`
  - `ProtocolPackets.hpp`
  - `EntityPackets.hpp`
  - `InventoryPackets.hpp`
  - `RecipePackets.hpp`
  - `ContainerPacketHandler.hpp`
  - `EntityMetadataSerializer.hpp`
  - `GameStateChangePacket.hpp`

### 协议数据包

#### ProtocolPackets.hpp

**职责**: 定义核心协议数据包

**主要内容**:

- **协议常量** (`namespace protocol`)
  - `VERSION`: Minecraft 1.16.5 协议版本 (753)
  - `MAX_USERNAME_LENGTH`: 最大用户名长度 (16)
  - `MAX_CHAT_LENGTH`: 最大聊天消息长度 (256)
  - `POSITION_SCALE`: 位置编码精度 (4096.0f)
  - `ANGLE_SCALE`: 角度编码精度
  - `MAX_CHUNK_DATA_SIZE`: 最大区块数据大小 (1MB)

- **协议状态** (`ProtocolState`)
  - Handshaking, Status, Login, Play

- **玩家位置** (`PlayerPosition`)
  - x, y, z 坐标 (f64)
  - yaw, pitch 旋转角度 (f32)
  - onGround 地面状态
  - `serialize()`/`deserialize()` 方法

- **登录请求包** (`LoginRequestPacket`) [C→S]
  - 协议版本、用户名

- **登录响应包** (`LoginResponsePacket`) [S→C]
  - 成功标志、玩家ID、用户名、消息

- **玩家移动包** (`PlayerMovePacket`) [C→S]
  - 移动类型: Full, Position, Rotation, GroundOnly
  - 位置和旋转数据

- **方块交互包** (`BlockInteractionPacket`) [C→S]
  - 动作类型: StartDestroyBlock, AbortDestroyBlock, StopDestroyBlock
  - 方块坐标、交互面

- **方块放置包** (`PlayerTryUseItemOnBlockPacket`) [C→S]
  - 方块坐标、面、点击位置、手持信息

- **传送包** (`TeleportPacket`) [S→C]
  - 目标位置、传送ID

- **传送确认包** (`TeleportConfirmPacket`) [C→S]
  - 确认传送ID

- **区块数据包** (`ChunkDataPacket`) [S→C]
  - 区块坐标 (x, z)、维度ID、压缩的区块数据

- **卸载区块包** (`UnloadChunkPacket`) [S→C]
  - 区块坐标 (x, z)、维度ID

- **玩家生成包** (`PlayerSpawnPacket`) [S→C]
  - 玩家ID、用户名、初始位置

- **玩家消失包** (`PlayerDespawnPacket`) [S→C]
  - 玩家ID

- **方块更新包** (`BlockUpdatePacket`) [S→C]
  - 方块坐标、方块状态ID

- **聊天消息包** (`ChatMessagePacket`) [双向]
  - 消息内容、发送者ID

- **时间更新包** (`TimeUpdatePacket`) [S→C]
  - gameTime: 游戏总tick数
  - dayTime: 一天内时间
  - daylightCycleEnabled: 日光周期是否启用

- **光照更新包** (`LightUpdatePacket`) [S→C]
  - 区块段坐标、天空光照、方块光照、信任边缘标志

### 维度数据包

#### DimensionPackets.hpp / DimensionPackets.cpp

**职责**: 维度切换和重生相关数据包

**主要内容**:

- **维度切换包** (`ChangeDimensionPacket`) [S→C]
  - 当玩家切换维度时发送
  - 包含: 目标维度ID、目标位置、是否为重生触发
  - 客户端应卸载当前维度的所有区块

- **重生包** (`RespawnPacket`) [S→C]
  - 当玩家死亡重生或从末地返回时发送
  - 包含: 维度ID、位置、朝向、游戏模式、世界类型标志

- **维度信息包** (`DimensionInfoPacket`) [S→C]
  - 在玩家登录时发送
  - 告知客户端服务器支持的所有维度信息
  - 包含: 维度ID、名称、环境属性(天空光、天花板、环境光)

- **确认维度切换包** (`ConfirmDimensionChangePacket`) [C→S]
  - 客户端完成维度切换后发送
  - 通知服务端可以发送新区块数据

### 实体数据包

#### EntityPackets.hpp / EntityPackets.cpp

**职责**: 实体同步相关数据包

**主要内容**:

- **实体生成包** (`SpawnEntityPacket`) [S→C]
  - 用于非生物实体 (物品、经验球等)
  - 包含: 实体ID、UUID、实体类型、位置、旋转、速度
  - 支持 ItemStack 数据 (用于 ItemEntity)

- **Mob生成包** (`SpawnMobPacket`) [S→C]
  - 用于 Mob 实体 (动物、怪物等)
  - 包含: 实体ID、UUID、实体类型、位置、旋转、头部朝向、速度、元数据

- **实体数据同步包** (`EntityMetadataPacket`) [S→C]
  - 同步实体的数据参数 (生命值、姿态、状态等)

- **实体速度包** (`EntityVelocityPacket`) [S→C]
  - 同步实体运动速度 (单位: 1/8000 block/tick)

- **实体传送包** (`EntityTeleportPacket`) [S→C]
  - 传送实体到指定位置

- **实体销毁包** (`EntityDestroyPacket`) [S→C]
  - 通知客户端销毁指定实体

- **实体动画包** (`EntityAnimationPacket`) [S→C]
  - 播放实体动画: SwingMainHand, TakeDamage, LeaveBed, SwingOffHand 等

- **实体相对移动包** (`EntityMovePacket`) [S→C]
  - 同步实体相对移动 (单位: 1/32 block)

- **实体头部朝向包** (`EntityHeadLookPacket`) [S→C]
  - 同步实体头部朝向

- **实体状态包** (`EntityStatusPacket`) [S→C]
  - 状态类型: Hurt, Death, LoveHeart, SheepEatGrass, ChickenLayEgg 等

- **物品拾取动画包** (`CollectItemPacket`) [S→C]
  - 通知客户端播放物品拾取动画

#### EntityMetadataSerializer.hpp / EntityMetadataSerializer.cpp

**职责**: 实体元数据序列化

**主要内容**:
- MC 1.16.5 元数据格式
  - 每个条目: 索引(1字节) + 类型ID(1字节) + 数据(变长)
  - 结束标记: 0xFF

- 类型ID映射 (19种类型)
  - Byte, VarInt, Float, String, TextComponent
  - OptChat, Slot, Boolean, Rotation, Position
  - OptPosition, Direction, OptUUID, OptBlockID, NBT
  - Particle, VillagerData, OptVarInt, Pose

- `serialize()`: EntityDataManager → 网络字节流
- `deserialize()`: 网络字节流 → EntityDataManager
- `serializeEntry()`: 序列化单个数据条目

### 背包数据包

#### InventoryPackets.hpp / InventoryPackets.cpp

**职责**: 背包和容器相关数据包

**主要内容**:

- **协议常量** (`namespace inventory`)
  - `MAX_SLOTS`: 最大槽位数 (256)
  - `MAX_STACK_SIZE`: 最大堆叠数 (64)
  - `PLAYER_INVENTORY_SIZE`: 玩家背包大小 (41)

- **容器内容同步包** (`ContainerContentPacket`) [S→C]
  - 同步整个容器的所有槽位内容

- **槽位更新包** (`ContainerSlotPacket`) [S→C]
  - 同步单个槽位的内容

- **玩家背包同步包** (`PlayerInventoryPacket`) [S→C]
  - 同步玩家完整背包内容

- **创造库存动作包** (`CreativeInventoryActionPacket`) [C→S]
  - 创造模式下客户端直接写回单个槽位的物品堆

- **容器点击包** (`ContainerClickPacket`) [C→S]
  - 客户端发送点击操作
  - 包含: 容器ID、槽位索引、按钮、点击动作、鼠标物品

- **关闭容器包** (`CloseContainerPacket`) [双向]
  - 客户端或服务端都可以发送

- **打开容器包** (`OpenContainerPacket`) [S→C]
  - 服务端通知客户端打开容器窗口

- **快捷栏选择包** (`HotbarSelectPacket`) [C→S]
  - 客户端通知服务端切换选中的快捷栏槽位

- **快捷栏设置包** (`HotbarSetPacket`) [S→C]
  - 服务端通知客户端设置选中的快捷栏槽位

#### ContainerPacketHandler.hpp / ContainerPacketHandler.cpp

**职责**: 容器网络包处理器

**主要内容**:
- `handleContainerClick()`: 处理容器点击包并驱动当前打开的菜单
- `handleCloseContainer()`: 处理关闭容器包并清理当前菜单引用
- `handleHotbarSelect()`: 处理快捷栏选择包
- `createContentPacket()`: 创建容器内容同步包
- `createSlotPacket()`: 创建槽位更新包
- `createOpenContainerPacket()`: 创建打开容器包
- `createRecipeListPacket()`: 创建配方列表同步包
- `createCraftResultPreview()`: 创建合成结果预览包

- `ContainerTypes` 工具命名空间
  - `getSlotCount()`: 获取容器类型的槽位数
  - `getDefaultTitle()`: 获取容器类型的默认标题
  - `toNetworkType()`: 转换为网络传输值
  - `toClickType()` / `toClickAction()`: 统一容器点击动作与菜单点击类型的映射

### 配方数据包

#### RecipePackets.hpp

**职责**: 配方同步相关数据包

**主要内容**:

- **配方同步包** (`RecipeSyncPacket`) [S→C]
  - 同步单个配方到客户端
  - 包含: 配方ID、配方类型、配方数据

- **配方列表同步包** (`RecipeListSyncPacket`) [S→C]
  - 批量同步配方到客户端

- **配方解锁包** (`RecipeUnlockPacket`) [S→C]
  - 通知客户端解锁新配方

- **合成结果预览包** (`CraftResultPreviewPacket`) [S→C]
  - 同步当前合成网格的匹配结果

### 其他数据包

#### BlockBreakAnimPacket.hpp / BlockBreakAnimPacket.cpp

**职责**: 方块破坏动画同步

**主要内容**:
- 破坏者实体ID、方块位置、破坏阶段 (0-9, -1表示移除)
- 工厂方法: `createUpdate()`, `createRemove()`

#### GameStateChangePacket.hpp / GameStateChangePacket.cpp

**职责**: 游戏状态变化通知

**主要内容**:
- 状态原因枚举:
  - InvalidBed: 床无效
  - EndRaining/BeginRaining: 天气变化
  - ChangeGameMode: 游戏模式改变
  - WinGame: 胜利
  - RainStrengthChange/ThunderStrengthChange: 天气强度变化
  - EnableRespawnScreen: 启用重生屏幕

- 工厂方法: `endRain()`, `beginRain()`, `rainStrength()`, `thunderStrength()`, `gameModeChange()`

#### PlayerAbilitiesPacket.hpp / PlayerAbilitiesPacket.cpp

**职责**: 玩家能力同步

**主要内容**:
- 能力标志位:
  - Invulnerable: 无敌
  - Flying: 飞行中
  - CanFly: 可飞行
  - CreativeMode: 创造模式

- 飞行速度、行走速度
- 工厂方法: `fromPlayer()`, `fromGameMode()`

## 文件关系图

```
PacketModule.hpp (统一入口)
    │
    ├── Packet.hpp (基类)
    │       └── Packet.cpp
    │
    ├── PacketSerializer.hpp (序列化工具)
    │       └── PacketSerializer.cpp
    │
    ├── ProtocolPackets.hpp (核心协议包)
    │       └── 依赖 PacketSerializer.hpp
    │
    ├── EntityPackets.hpp (实体包)
    │       ├── EntityPackets.cpp
    │       └── 依赖 Packet.hpp, PacketSerializer.hpp, ItemStack.hpp
    │
    ├── EntityMetadataSerializer.hpp (元数据序列化)
    │       ├── EntityMetadataSerializer.cpp
    │       └── 依赖 EntityDataManager.hpp
    │
    ├── InventoryPackets.hpp (背包包)
    │       ├── InventoryPackets.cpp
    │       └── 依赖 PacketSerializer.hpp, ItemStack.hpp
    │
    ├── ContainerPacketHandler.hpp (容器处理器)
    │       ├── ContainerPacketHandler.cpp
    │       └── 依赖 InventoryPackets.hpp, RecipePackets.hpp
    │
    ├── RecipePackets.hpp (配方包)
    │       └── 依赖 PacketSerializer.hpp, ResourceLocation.hpp
    │
    ├── BlockBreakAnimPacket.hpp (破坏动画包)
    │       ├── BlockBreakAnimPacket.cpp
    │       └── 依赖 Packet.hpp, BlockPos.hpp
    │
    ├── GameStateChangePacket.hpp (状态变化包)
    │       ├── GameStateChangePacket.cpp
    │       └── 依赖 Packet.hpp
    │
    └── PlayerAbilitiesPacket.hpp (玩家能力包)
            ├── PlayerAbilitiesPacket.cpp
            └── 依赖 Packet.hpp, Player.hpp
```

## 模块整体职责

### 职责概述

本模块负责实现 Minecraft 1.16.5 网络协议的数据包定义与序列化：

1. **数据包定义**: 定义所有网络通信数据包的结构和字段
2. **序列化/反序列化**: 提供二进制格式与数据包对象之间的转换
3. **协议兼容**: 确保与 Minecraft Java Edition 1.16.5 协议兼容
4. **类型安全**: 使用强类型枚举和结构体，避免协议错误

### 输入和输出

**输入**:
- 服务端/客户端的业务数据 (位置、状态、事件等)
- 来自网络的二进制数据流

**输出**:
- 序列化后的二进制数据 (用于网络传输)
- 反序列化后的数据包对象 (供业务逻辑使用)

### 依赖项

| 依赖模块 | 用途 |
|---------|------|
| `common/core/Types.hpp` | 基本类型定义 (i8, i16, i32, u8, f32, String 等) |
| `common/core/Result.hpp` | 错误处理 (Result<T>, Error, ErrorCode) |
| `common/util/math/Vector3.hpp` | 三维向量 |
| `common/util/Direction.hpp` | 方向枚举 |
| `common/world/block/BlockPos.hpp` | 方块位置 |
| `common/entity/EntityDataManager.hpp` | 实体数据管理 |
| `common/entity/Player.hpp` | 玩家实体 |
| `common/entity/inventory/` | 背包系统 |
| `common/item/ItemStack.hpp` | 物品堆 |
| `common/resource/ResourceLocation.hpp` | 资源位置 |

### 使用方法

#### 序列化数据包

```cpp
#include "common/network/packet/PacketModule.hpp"

// 创建数据包
mc::network::EntityVelocityPacket packet;
packet.setEntityId(100);
packet.setVelocity(1000, -500, 2000);

// 序列化
auto result = packet.serialize();
if (result.success()) {
    const auto& data = result.value();
    // 发送 data 到网络层
}
```

#### 反序列化数据包

```cpp
#include "common/network/packet/PacketModule.hpp"

// 从网络接收数据
std::vector<u8> networkData = receiveFromNetwork();

// 反序列化
mc::network::EntityVelocityPacket packet;
auto result = packet.deserialize(networkData.data(), networkData.size());
if (result.success()) {
    // 使用数据
    u32 entityId = packet.entityId();
    i16 vx = packet.velocityX();
}
```

#### 使用序列化器

```cpp
#include "common/network/packet/PacketSerializer.hpp"

// 序列化
mc::network::PacketSerializer serializer;
serializer.writeVarInt(12345);
serializer.writeString("hello");
serializer.writeF32(3.14f);

const auto& buffer = serializer.buffer();

// 反序列化
mc::network::PacketDeserializer deserializer(buffer.data(), buffer.size());
auto value = deserializer.readVarInt();  // 12345
auto str = deserializer.readString();    // "hello"
auto f = deserializer.readF32();         // 3.14f
```

### 容易踩的坑

1. **字节序问题**
   - 所有网络传输使用大端序 (Big-Endian)
   - 使用 `NetworkEndian` 工具类进行转换
   - 注意: VarInt/VarLong 不需要字节序转换

2. **VarInt 编码**
   - VarInt 使用变长编码，最多5字节
   - VarLong 使用变长编码，最多10字节
   - 负数编码后字节数更多，注意缓冲区大小

3. **字符串长度限制**
   - 用户名最大16字符
   - 聊天消息最大256字符
   - 断开原因最大1024字符
   - 使用 `MAX_STRING_LENGTH` (65535) 作为通用上限

4. **数据包大小限制**
   - 区块数据最大1MB (`MAX_CHUNK_DATA_SIZE`)
   - 光照数据最大4096字节 (每个区块段)

5. **实体速度单位**
   - 速度单位为 1/8000 block/tick
   - 相对移动单位为 1/32 block

6. **数据包类型ID分配**
   - 0-99: 内部控制包
   - 100-199: 客户端→服务端
   - 200-299: 服务端→客户端
   - 300+: 特殊用途包

7. **反序列化错误处理**
   - 所有 `deserialize()` 方法返回 `Result<T>`
   - 必须检查返回值是否成功
   - 数据不足、数据格式错误都会导致失败

8. **EntityMetadata 格式**
   - MC 1.16.5 使用特定的元数据格式
   - 结束标记为 0xFF
   - 类型ID映射必须正确

### 涉及的测试用例

测试文件位于 `tests/network/`:

| 测试文件 | 测试内容 |
|---------|---------|
| `EntityPacketsTest.cpp` | 实体数据包序列化/反序列化测试 |
| `LocalServerConnectionTest.cpp` | 本地连接测试 |

`tests/common/test_container.cpp` 也包含 `CreativeInventoryActionPacket` 的序列化/反序列化测试，以及创造模式物品库辅助函数测试。

**EntityPacketsTest.cpp 测试覆盖**:
- `SpawnEntityPacket`: 序列化/反序列化、包类型验证
- `SpawnMobPacket`: 序列化/反序列化、元数据处理
- `EntityVelocityPacket`: 速度同步测试
- `EntityTeleportPacket`: 传送测试
- `EntityDestroyPacket`: 销毁测试 (包括空列表)
- `EntityAnimationPacket`: 所有动画类型测试
- `EntityMovePacket`: 相对移动测试
- `EntityHeadLookPacket`: 头部朝向测试
- `EntityStatusPacket`: 所有状态类型测试
- `EntityMetadataPacket`: 元数据测试 (包括空元数据)
- 错误处理: 数据不足的情况

## 协议版本

本模块实现 Minecraft Java Edition 1.16.5 协议 (版本号 753)。

主要协议特性:
- VarInt/VarLong 变长整数编码
- 大端序网络字节序
- 字符串 UTF-8 编码，带长度前缀
- 实体元数据格式
- 区块数据压缩格式
