# BossBar 模块

Boss 栏系统，用于创建和管理自定义 Boss 血量条和末影龙 Boss 栏。

## 目录结构

```
bossbar/
├── BossInfo.hpp/cpp               # Boss 栏基类（颜色、样式枚举、核心属性）
├── ServerBossInfo.hpp/cpp         # 服务端 Boss 栏（玩家可见性管理、属性变更通知）
├── CustomServerBossInfo.hpp/cpp   # /bossbar 命令创建的自定义 Boss 栏（持久化、数值管理）
├── CustomServerBossInfoManager.hpp/cpp  # Boss 栏管理器（生命周期、网络同步）
├── ServerDragonBossBar.hpp/cpp    # 末影龙 Boss 栏（IDragonBossBar 的服务端实现）
└── README.md
```

## 内部模块关系

```
BossInfo (基类)
    │
    └── ServerBossInfo (服务端扩展：玩家管理)
            │
            └── CustomServerBossInfo (自定义 Boss 栏：持久化、数值管理)
                    │
                    └── 由 CustomServerBossInfoManager 管理

IDragonBossBar (common 层抽象接口，见 src/common/world/dimension/end/)
    │
    └── ServerDragonBossBar (本模块：直接通过 ir::play::BossEvent 同步)
```

- **BossInfo**：定义核心属性（UUID、名称、百分比、颜色、样式、标志位）
- **ServerBossInfo**：添加玩家可见性管理，属性变更时标记更新类型
- **CustomServerBossInfo**：添加资源位置 ID、value/max 数值管理、NBT 持久化、玩家 UUID 集合
- **CustomServerBossInfoManager**：管理所有自定义 Boss 栏的生命周期
- **ServerDragonBossBar**：实现 `IDragonBossBar` 接口，为 `EndDragonFight` 提供末影龙 Boss 栏的网络同步

## ServerDragonBossBar

`ServerDragonBossBar` 是 `IDragonBossBar`（定义在 `src/common/world/dimension/end/IDragonBossBar.hpp`）的服务端实现，用于解耦 `EndDragonFight`（common 层）与 `ServerBossInfo`（server 层）。

### 为什么不继承 ServerBossInfo？

`ServerBossInfo` 的 `broadcastUpdate`/`sendAddPacket`/`sendRemovePacket` 是空实现（只有 `CustomServerBossInfo` 通过其 Manager 覆写才生效）。`ServerDragonBossBar` 不需要持久化、`/bossbar` 命令管理等功能，只需要将状态变更直接发送给追踪玩家，因此直接持有 Boss 栏状态并通过 `ir::play::BossEvent` 发送网络包，而非继承 `ServerBossInfo`。

### 生命周期

1. `MinecraftServer::setupDragonFightBossBar()` 在服务端启动时创建 `ServerDragonBossBar`
2. 通过 `EndDragonFight::setDragonBossBar()` 注入到 `EndDragonFight`
3. `EndDragonFight` 在 `updateDragon()`/`tick()`/`setDragonKilled()` 中调用接口方法
4. `ServerDragonBossBar` 将变更通过 `IServer::connectionManager().sendPacketToPlayer()` 发送给追踪玩家

### replacePlayers 增量更新

`replacePlayers(const std::set<PlayerId>&)` 计算新旧玩家列表的差集：
- 旧集合中不在新集合中的玩家：发送 `Remove` 包
- 新集合中不在旧集合中的玩家：发送 `Add` 包
- 两集合交集中的玩家：不发送任何包（避免客户端闪烁）

这对应 MC Java `EndDragonFight.updatePlayers()` 中的 add/remove 差集逻辑。

## 外部依赖

### 依赖的上游模块

- `common/core/Types.hpp` - 基础类型（u64, f32, i32, PlayerId 等）
- `common/command/ICommandSource.hpp` - Uuid 类型定义（std::array<u8, 16>）
- `common/util/UuidUtils.hpp` - UUID 工具函数（generateRandomUuid 等）
- `common/resource/ResourceLocation.hpp` - 资源位置 ID
- `common/util/text/ITextComponent.hpp` - 文本组件
- `common/util/text/ComponentUtils.hpp` - wrapInSquareBrackets 方括号包裹工具（formattedName 使用）
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `common/world/dimension/end/IDragonBossBar.hpp` - 末影龙 Boss 栏抽象接口（ServerDragonBossBar 依赖）
- `server/application/MinecraftServer.hpp` - IServer 访问（connectionManager）

### 被谁依赖

- `EndDragonFight`（common）通过 `IDragonBossBar` 接口依赖 `ServerDragonBossBar`（注入时依赖）
- `MinecraftServer::setupDragonFightBossBar()` 创建并注入 `ServerDragonBossBar`
- 未来 `/bossbar` 命令将依赖 `CustomServerBossInfoManager`

## 容易踩的坑

### UUID 生成

`CustomServerBossInfo` 使用 `util::generateRandomUuid()` 生成 128 位随机 UUID v4，与 MC Java 的 `Mth.createInsecureUUID()` 一致。UUID 在 `BossInfo` 中存储为 `Uuid`（即 `std::array<u8, 16>`），在 `ir::play::BossEvent` 中以两个 i64（MSB + LSB）序列化到网络。

`ServerDragonBossBar` 在构造时由 `MinecraftServer::setupDragonFightBossBar()` 通过 `util::generateRandomUuid()` 生成一个随机 UUID，用于在客户端标识此 Boss 栏。

### 网络包

`ir::play::BossEvent` 是 1.21.11 单包 ID + u8 operation 的 Boss 栏同步包，支持六种操作（Add、Remove、UpdatePercent、UpdateName、UpdateStyle、UpdateProperties）。UUID 以 128 位（两个 i64）格式在网络包中传输。

`ServerDragonBossBar` 内部构建 `ir::play::BossEvent` 并通过 `IServer::connectionManager().sendPacketToPlayer()` 直接发送，不经过 `ServerBossInfo` 的（空）广播方法。

### 玩家登出处理

`onPlayerLogout()` 不发送网络包，因为玩家已经断开连接。只清理内存中的可见性状态，但保留 UUID 记录用于重连恢复。

`ServerDragonBossBar` 的 `replacePlayers()` 会自动将已离开的玩家从追踪集合中移除（因为新玩家列表不再包含他们），并在下次玩家扫描时通过 `Remove` 包通知（如果玩家仍在线但在追踪范围外）。

### 线程安全

`CustomServerBossInfoManager` 不是线程安全的。如果需要在多线程环境使用，需要添加互斥锁保护。

`ServerDragonBossBar` 也不是线程安全的，所有调用必须发生在主服务器线程（与 `EndDragonFight::tick()` 同一线程）。

### 玩家 UUID 集合

`CustomServerBossInfo` 维护两套玩家集合：
- `m_players`（继承自 ServerBossInfo）：当前在线可见玩家的 PlayerId
- `m_playerUuids`：持久化的玩家 UUID 字符串集合，用于玩家重连后恢复可见性

两者需要同步维护。

`ServerDragonBossBar` 只维护一套 `m_players`（`std::set<PlayerId>`），因为末影龙 Boss 栏不需要持久化（每次服务端启动重新创建）。
