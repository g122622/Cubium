# network/play - Play 阶段入站处理

处理客户端进入 Play 阶段后发来的所有 C→S 包。上游是 `session/ClientSession::handleInbound`，
它做完握手消费与 phase / playerId 双重守卫后，把未被消费的 Play 包交给本层。

## 目录结构

```
src/server/network/play/
├── README.md
└── ServerPlayHandler.hpp/cpp   # 24 路 std::visit 分发表 + 各 handle*Packet 处理体（持 MinecraftServer&）
```

`ServerPlayHandler` 是进程级单例门面，持 `MinecraftServer&`。`route(playerId, packet)` 内含完整的
24 路 `std::holds_alternative` 分发表，多数分支转调本类的 `handle*Packet` 处理体，4 个分支回调
`MinecraftServer` 的纯虚（`handleHotbarSelect` / `handleContainerClick` / `handleCloseContainer` /
`SetCreativeModeSlot`）。

## 内部模块关系

```
ClientSession::handleInbound（session/）
        │ 握手已消费则返回；否则 phase / playerId 守卫
        ▼
ServerPlayHandler::route
        ├─ 24 路 holds_alternative
        ├─ 多数分支 ──▶ 本类 handle*Packet 处理体
        └─ 4 个分支 ──▶ MinecraftServer 纯虚（子类 override）
```

守卫之所以在 `ClientSession` 而不在本层：phase 守卫需要连接状态、playerId 守卫需要会话状态，
二者都只有会话层持有。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/ir/packets/play/*` | C→S 包定义（`std::visit` 的变体） |
| `server/application/MinecraftServer.hpp` | 各处理体的业务入口与 4 个纯虚 |
| `server/core/*`、`server/player/ServerPlayer.hpp`、`server/world/ServerWorld.hpp` | 具体处理逻辑 |
| `server/command/*` | 聊天命令执行 |
| `server/network/outbound/*` | 部分处理体经其构造回包 |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `session/ClientSession` | 引本类门面并在守卫后调用 `route` |
| `server/application/MinecraftServer` | 持本类门面；`LoginFlow` 与维度切换调 `updateEntityTrackingForPlayer` |

## 容易踩的坑

### 1. 新增 C→S 包必须同时改两处

`route()` 的 if/else 链是**唯一**的分发表。漏加分支不会编译报错，只会在运行期打一条
`route: unhandled C->S play variant` 并被静默丢弃。新增 C→S 包时务必同步：①在 `route()` 加分支；
②在对应处理体加实现。

### 2. 分发表不能改用 `packet.index()`

`std::variant` 的 index 会随 `PlayPackets.hpp` 增删变体而整体错位，用 index 建静态分发表会导致
静默串包。`holds_alternative` 链虽然啰嗦但是类型安全的。

### 3. 纯虚分发不能下沉

`handleHotbarSelect` / `handleContainerClick` / `handleCloseContainer` / `handleOpenPlayerInventoryPacket`
由 `MinecraftServer` 声明、`IntegratedServer`/`StandaloneServer` override，且**不在** `IServer` 上。
因此本类必须持 `MinecraftServer&` 而非 `IServer&`——把类型"顺手优化"成 `IServer&` 会直接编译失败。

### 4. 守卫不要在 `ClientSession` 与本层重复

`ClientSession::handleInbound` 已做 phase 与 playerId 守卫，本层不再重复判断；反之也不要往
`ClientSession` 里加业务分支——它只做协议层守卫，不解释包内容。
