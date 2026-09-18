# network/play - Play 阶段入站处理

处理客户端进入 Play 阶段后发来的所有 C→S 包。上游是 `session/ClientSession`，它做完 phase 与
playerId 守卫后把包交给本层。

## 目录结构

```
src/server/network/play/
├── README.md
├── ServerPlayRouter.hpp/cpp       # 每连接守卫（phase != Play / playerId == 0 丢弃）+ 转调
└── ServerPlayHandler.hpp/cpp      # 24 路 std::visit 分发表 + 各 handle*Packet 处理体（持有 MinecraftServer&）
```

- `ServerPlayRouter`：每连接一个实例，只持有 `ServerPlayHandler&`、`playerId`、`sessionId`。
  职责是守卫 + 转发，**不含任何 `std::holds_alternative` 分支**。
- `ServerPlayHandler`：进程级单例门面，持 `MinecraftServer&`，`route()` 内含完整的 24 路
  `std::holds_alternative` 分发表，其余为各 `handle*Packet` 处理体。

## 内部模块关系

```
ClientSession（session/）── 守卫后 ──▶ ServerPlayRouter::handle
                                            │
                                            ▼
                                    ServerPlayHandler::route
                                      ├─ 24 路 holds_alternative
                                      ├─ 多数分支转调本类 handle*Packet
                                      └─ 4 个分支回调 MinecraftServer 的纯虚
                                         （handleHotbarSelect / handleContainerClick
                                          / handleCloseContainer / SetCreativeModeSlot）
```

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/ir/packets/play/*` | C→S 包定义（`std::visit` 的变体） |
| `server/application/MinecraftServer.hpp` | 各处理体的业务入口 |
| `server/core/*`、`server/player/ServerPlayer.hpp`、`server/world/ServerWorld.hpp` | 具体处理逻辑 |
| `server/command/*` | 聊天命令执行 |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `session/ClientSession` | 值持 `ServerPlayRouter` 并在守卫后调用 |
| `server/application/MinecraftServer` | 持 `ServerPlayHandler` 门面；`LoginFlow`/维度切换调 `updateEntityTrackingForPlayer` |

## 容易踩的坑

### 1. 新增 C→S 包必须同时改两处

`ServerPlayHandler::route()` 的 if/else 链是**唯一**的分发表，漏加分支不会编译报错，只会在运行期
打一条 `route: unhandled C->S play variant` 并被静默丢弃。新增 C→S 包时务必同步：
①在 `route()` 加分支；②在对应 handler 加处理体。

### 2. 分发表不能改用 `packet.index()`

`std::variant` 的 index 会随 `PlayPackets.hpp` 增删变体而整体错位，用 index 建静态分发表会导致
静默串包。`holds_alternative` 链虽然啰嗦但是类型安全的。

### 3. 纯虚分发不能下沉

`handleHotbarSelect` / `handleContainerClick` / `handleCloseContainer` / `handleOpenPlayerInventoryPacket`
由 `MinecraftServer` 声明、`IntegratedServer`/`StandaloneServer` override，且**不在** `IServer` 上。
因此处理体必须持 `MinecraftServer&` 而非 `IServer&`——把类型"顺手优化"成 `IServer&` 会直接编译失败。

### 4. 守卫归属：`ServerPlayRouter` 不做业务判断

`ServerPlayRouter` 只做两件事：phase 非 Play 则告警丢弃、playerId 为 0 则丢弃。
业务分支一律在 `ServerPlayHandler`。不要往 Router 里加业务状态。
