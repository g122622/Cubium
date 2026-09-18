# network/handshake - 连接建立

覆盖从 TCP/本地连接建立到玩家进入 Play 的全过程：Handshake → Status → Login → Configuration，
以及 Configuration 结束后的 Play 入场序列。

## 目录结构

```
src/server/network/handshake/
├── README.md
├── ServerHandshake.hpp/cpp        # 四阶段状态机（每连接一个实例）
├── RegistryDataBuilder.hpp/cpp    # Configuration 阶段：registry / UpdateTags / KnownPacks 载荷
├── EnchantmentNbtBuilder.hpp/cpp  # datapack 附魔 JSON → 内联 NBT 的 RegistryEntry
└── LoginFlow.hpp/cpp              # 进入 Play 的入场序列（建号 + 初始状态推送整簇）
```

- `ServerHandshakeStateMachine`：唯一入站入口 `handleInbound`，返回 `true` 表示握手/Configuration
  阶段已消费该包，`false` 表示应转交 Play 路由。终点是 `_pushConfigurationData`（推 RegistryData →
  UpdateTags → FinishConfiguration）后触发 `PlayerReadyCallback`。
- `RegistryDataBuilder`：自由函数集合，构造 Configuration 阶段 23 个 registry 列表 + UpdateTags +
  KnownPacks。
- `EnchantmentNbtBuilder`：把 datapack 中约 43 个附魔 JSON 展平为带内联 NBT 的 `RegistryEntry`，
  带 `std::call_once` 进程级缓存。
- `LoginFlow`：从 `PlayerReadyCallback` 开始，负责 Play 阶段入口——建号、`play::Login`、权限、
  命令树、初始游戏状态。持 `MinecraftServer&`。

## 内部模块关系

```
ServerClientConnection（base/）
        │ handleInbound(pkt)
        ▼
ServerHandshakeStateMachine ──_pushConfigurationData──▶ RegistryDataBuilder
        │                                                      │
        │                                                      ▼
        │                                            EnchantmentNbtBuilder
        │ PlayerReadyCallback（= ClientSessionManager::onPlayerReady）
        ▼
LoginFlow.createPlayerForConnection ──▶ play::Login / 权限 / 命令树 / 初始状态
        │
        └─ 回填 playerId 到 ClientSession（session/）
```

按**协议阶段**切分，无功能重叠：状态机负责 Handshake/Status/Login/Configuration 四阶段，
`LoginFlow` 只负责进入 Play 之后的事。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `base/ServerClientConnection.hpp` | `setState`/`setPhase`/`send` |
| `common/network/backend/java/handshake/JavaLoginHandshaker.hpp` | Login 阶段包编解码（双向共用，留在 common） |
| `common/network/ir/packets/{handshake,login,status,configuration}/*` | 各阶段 IR 包 |
| `server/application/MinecraftServer.hpp` | `LoginFlow` 的处理体入口 |
| `server/core/PlayerManager.hpp`、`server/world/ServerWorld.hpp` | 建号与初始世界状态 |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `session/ClientSession` | 值持 `ServerHandshakeStateMachine` |
| `session/ClientSessionManager` | 握手完成回调里调 `LoginFlow::createPlayerForConnection` |
| `session/ServerNetwork` | 入站分流到握手状态机 |
| `tests/common/network/{NetworkTestFixtures,test_server_handshake}` | 逐包 pump 断言状态转换 |

## 容易踩的坑

### 1. `handleInbound` 的返回值是分流契约

返回 `true` = 已消费（握手/Status/Login/Configuration 包），返回 `false` = 应交给 Play 路由。
调用方（`session/ClientSession`）据此分流；不要改成"抛异常"或"回写成员变量"式的隐式约定。

### 2. Configuration 卡死先查 `SelectKnownPacks`

`RegistryDataBuilder` 目前对所有 registry 条目发 `data=nullopt`（声明"客户端已知"）。这仅在我方互通
双方都硬编码 vanilla registry 且 `SelectKnownPacks{minecraft:core}` **命中**时合法。若客户端卡在
Configuration 阶段不动，首先确认 `KnownPacks` 是否命中 core。

### 3. `EnchantmentNbtBuilder` 有进程级缓存

`buildEnchantmentRegistryEntries` 用 `std::call_once` 缓存结果，datapack 路径在首次调用后即固化。
测试里若需要换 datapack 源，必须用 `buildEnchantmentRegistryEntriesUncached`，否则会拿到上一次的缓存。

### 4. 命令树编码只有一份实现

命令树的二进制编码在 `outbound/CommandTreeEncoder.hpp`。`LoginFlow` 的入场序列与
`/op`、`/deop` 命令都经 `outbound/PacketBuilders::buildCommandsIr` 走同一实现，不要在 `LoginFlow`
里另写一份——两份实现漂移会导致"进服时补全正常、`/op` 后补全失效"这类难查的不一致。

### 5. `LoginFlow` 与 `PacketBuilders` 的分工

`LoginFlow` 负责**编排**（何时发、发给谁、失败后是否中断序列），`outbound/PacketBuilders` 负责
**纯构造**（只返回 IR 包，不发包、不依赖 `MinecraftServer`）。命令树编码失败时 `LoginFlow` 记录日志
并继续（玩家已进入 Play，可恢复），不要升级为中断登录。
