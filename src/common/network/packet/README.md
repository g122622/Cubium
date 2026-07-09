# Packet 模块

网络数据包模块，负责客户端与服务端之间的通信协议实现。实现 Minecraft Java Edition 1.16.5 协议（版本号 753）。

## 目录结构

```
src/common/network/packet/
├── Packet.hpp                     # 数据包基类定义
├── Packet.cpp                     # 数据包基类实现
├── PacketSerializer.hpp           # 序列化/反序列化工具类
├── PacketSerializer.cpp           # 序列化工具实现
├── PacketModule.hpp               # 模块统一头文件
├── ProtocolPackets.hpp            # 核心协议数据包（登录、移动、区块、聊天等）
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
├── ServerDifficultyPacket.hpp     # 难度同步包
├── ServerDifficultyPacket.cpp     # 难度同步包实现
├── DimensionPackets.hpp           # 维度切换数据包（含 RespawnPacket 的 lastDeathLocation 同步）
├── DimensionPackets.cpp           # 维度数据包实现
├── SpawnPositionPacket.hpp        # 世界出生点数据包
├── SpawnPositionPacket.cpp        # 世界出生点包实现
├── ExplosionPacket.hpp            # 爆炸事件数据包
├── ExplosionPacket.cpp            # 爆炸事件包实现
├── TitlePacket.hpp                # 标题显示包
├── TitlePacket.cpp                # 标题显示包实现
├── BossInfoPacket.hpp             # Boss 栏同步包
├── BossInfoPacket.cpp             # Boss 栏同步包实现
├── SleepPacket.hpp                # 睡眠状态同步包
├── WorldBorderPacket.hpp          # 世界边界同步包
├── WorldBorderPacket.cpp          # 世界边界同步包实现
├── AdvancementPackets.hpp         # 成就系统数据包
└── AdvancementPackets.cpp         # 成就数据包实现
├── SetCameraPacket.hpp            # 旁观者摄像机同步包 (S2C)
├── SetCameraPacket.cpp            # 旁观者摄像机同步包实现
├── BlockEventPacket.hpp           # 方块事件同步包 (S2C)
├── BlockEventPacket.cpp           # 方块事件同步包实现
├── ParticlePacket.hpp             # 粒子同步包 (S2C)（普通/方块/物品/EntityEffect/Vibration/Trail）
└── ParticlePacket.cpp             # 粒子同步包实现
```

## 内部模块关系

```
PacketModule.hpp (统一入口)
    │
    ├── Packet.hpp (基类) ─── 所有数据包都继承此类
    │
    ├── PacketSerializer.hpp (序列化工具) ─── 所有数据包依赖
    │
    ├── ProtocolPackets.hpp (核心协议包)
    │
    ├── EntityPackets.hpp (实体包) ─── 依赖 ItemStack.hpp
    │
    ├── EntityMetadataSerializer.hpp (元数据序列化) ─── 依赖 EntityDataManager.hpp
    │
    ├── InventoryPackets.hpp (背包包) ─── 依赖 ItemStack.hpp
    │
    ├── ContainerPacketHandler.hpp (容器处理器) ─── 依赖 InventoryPackets.hpp, RecipePackets.hpp
    │
    ├── RecipePackets.hpp (配方包) ─── 依赖 ResourceLocation.hpp
    │
    └── 其他独立数据包（BossInfo、Title、Explosion等）
```

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖模块 | 用途 |
|---------|------|
| `common/core/Types.hpp` | 基本类型定义 (i8, i16, i32, u8, f32, std::string 等) |
| `common/core/Result.hpp` | 错误处理 (Result<T>, Error, ErrorCode) |
| `common/util/math/Vector3.hpp` | 三维向量 |
| `common/util/Direction.hpp` | 方向枚举 |
| `common/world/block/BlockPos.hpp` | 方块位置 |
| `common/entity/EntityDataManager.hpp` | 实体数据管理 |
| `common/entity/Player.hpp` | 玩家实体 |
| `common/entity/inventory/` | 背包系统 |
| `common/item/ItemStack.hpp` | 物品堆 |
| `common/resource/ResourceLocation.hpp` | 资源位置 |

### 依赖本模块的外部模块

| 使用模块 | 用途 |
|---------|------|
| `server/core/ConnectionManager.hpp` | 服务端连接管理，封装数据包发送 |
| `server/core/PacketHandler.hpp` | 服务端数据包处理 |
| `client/network/NetworkClient.hpp` | 客户端网络通信 |
| `server/player/ServerPlayer.hpp` | 服务端玩家数据包发送 |
| `server/world/ServerWorld.hpp` | 服务端世界同步（区块、方块更新） |
| `client/world/ClientWorld.hpp` | 客户端世界数据接收处理 |

## 容易踩的坑

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
   - 字符串使用 VarInt 编码长度前缀，支持大字符串（如命令树JSON）

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

9. **包头封装**
   - `KeepAlivePacket` 和 `DisconnectPacket` 按完整包处理（包含12字节包头）
   - 其余数据包只负责包体序列化，12字节包头由 `ConnectionManager::encapsulatePacket()` 统一添加

10. **SetCameraPacket 旁观者摄像机同步**
    - 方向：服务端→客户端 (S2C)
    - PacketType::SetCamera = 233
    - 字段：`u32 cameraEntityId`（VarInt 编码）
    - 当 cameraEntityId 为玩家自身实体 ID 时表示恢复正常视角
    - 服务端由 `ServerPlayer::_sendSetCameraPacket()` 发送
    - 客户端由 `NetworkClient::_handleSetCamera()` 接收，通过 `onSetCamera` 回调更新 `Player::m_cameraEntityId`
    - 对应 MC Java 的 `ClientboundSetCameraPacket`

11. **ParticlePacket 粒子同步包**
    - 方向：服务端→客户端 (S2C)
    - 字段：粒子类型 ID、位置、速度、偏移、数量、可选数据（`m_optionalData` 字节流）
    - 工厂方法（按粒子数据类型区分）：
      - `create()` - 普通粒子（无附加数据）
      - `createBlock()` - 方块粒子，`m_optionalData` 存储 BlockState ID（VarInt）
      - `createItem()` - 物品粒子（Item/ItemSlime/ItemCobweb/ItemSnowball），`m_optionalData` 存储 `ItemStack::serialize()` 的完整字节流（包含 present 标志 + 物品 ID + 数量 + NBT）
      - `createEntityEffect()` - 实体效果粒子，`m_optionalData` 存储 ARGB 颜色（4 字节）
      - `createVibration()` / `createTrail()` - 振动/轨迹粒子，`m_optionalData` 存储目标位置等参数
    - 辅助方法：`isBlockParticle()` / `isItemParticle()` / `isEntityEffectParticle()` / `isVibrationParticle()` / `isTrailParticle()` 判断可选数据类型；`decodeBlockState()` / `decodeItemStack()` / `decodeColor()` / `decodeVibrationData()` / `decodeTrailData()` 从 `m_optionalData` 反序列化
    - **物品粒子序列化**：`createItem()` 通过 `PacketSerializer` 临时序列化 `ItemStack`，再将字节流拷贝到 `m_optionalData`；`decodeItemStack()` 用 `PacketDeserializer` 包装 `m_optionalData` 调用 `ItemStack::deserialize()` 还原
    - **判空约定**：`isItemParticle()` 要求 `requiresItemData(type) && !m_optionalData.empty()`；空 `ItemStack` 序列化后为单字节 `0x00`（present=false），仍视为有效物品粒子数据
