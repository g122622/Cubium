# network/session - 会话生命周期

管理连接对象的归属、集合、每连接协议状态与 tick 泵。本层是 `server/network` 的**对外门面所在层**。

## 目录结构

```
src/server/network/session/
├── README.md
├── ServerNetwork.hpp/cpp          # 门面：TCP accept + 连接集合 + tick 泵 + 协议表
├── ClientSession.hpp              # 单客户端协议状态（值持握手状态机 + Play 路由 + playerId）
└── ClientSessionManager.hpp/cpp   # 远程会话登记/清理（connect → ready → disconnect）
```

- `ServerNetwork`：进程内唯一。持有 `shared_ptr<ProtocolTables>`（所有连接共享）与
  `vector<unique_ptr<ServerClientConnection>>`（**连接的所有权**）。同时承载 Local 与 Wire 两种模式，
  成员不相交故可共存。
- `ClientSession`：**非拥有**地引用一个 `ServerClientConnection`，值持 `ServerHandshakeStateMachine`
  与 `ServerPlayRouter`，记录 `playerId`。由 `ClientSessionManager` 以 `unique_ptr` 持有。
- `ClientSessionManager`：远程 TCP 玩家的会话簿记。本地客户端（`sessionId == 0`）目前由
  `IntegratedServer` 自行持有 `ClientSession`，尚未注册进本 manager。

## 内部模块关系

```
ServerNetwork  ──owns──▶  ServerClientConnection（base/）
      │                          ▲
      │ onConnect/onDisconnect    │ 非拥有引用
      ▼                          │
ClientSessionManager ──owns──▶ ClientSession
                                   ├─ ServerHandshakeStateMachine（handshake/）
                                   └─ ServerPlayRouter（play/）
```

`ServerNetwork::tick()` 在主线程做三件事：`pumpLocal()` 驱动所有 Local 连接、`drainInbound()` 派发
所有 Wire 连接的入站队列、处理延迟断开列表并回调 `m_onDisconnect`。三者都采用「锁内快照 → 锁外回调」
模式，避免回调内增删连接导致重入死锁。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `base/ServerClientConnection.hpp` | 连接类型与 `ProtocolTables` 别名 |
| `handshake/ServerHandshake.hpp` | `ClientSession` 值持的握手状态机 |
| `play/ServerPlayRouter.hpp` | `ClientSession` 值持的 Play 路由 |
| `handshake/LoginFlow.hpp`、`server/application/MinecraftServer.hpp` | 握手完成后建号 + 回填 playerId |
| `common/network/backend/java/JavaBackend.hpp` | 构造共享协议表 |
| `common/network/transport/{LocalTransport,TcpTransport}.hpp` | Local 配对 / TCP accept |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `server/application/{StandaloneServer,IntegratedServer}` | 建 `ServerNetwork`；`startAccept` / `publishToLan` |
| `server/application/MinecraftServer` | 持 `ServerNetwork` 与 `ClientSessionManager` 成员 |
| `tests/common/network/*` | Local 模式与握手的集成测试夹具 |

## 容易踩的坑

### 1. 销毁顺序：会话必须先于连接

`ClientSession` 持非拥有 `ServerClientConnection*`，连接所有权在 `ServerNetwork::m_connections`。
两子类 `stop()` / `_shutdownRemoteSessions()` 中必须**先清空会话**，再 `m_serverNetwork.reset()`。
顺序颠倒会悬垂。这是本模块唯一的硬性生命周期约束。

### 2. 构造顺序：ClientSession 必须在 playHandler 之后构造

`ClientSession` 构造时取 `MinecraftServer::playHandler()` 的引用。若早于 `initializeCoreManagers()`，
拿到的是空悬引用，表现为**首个 `AcceptTeleportation` 包触发 ACCESS_VIOLATION**（已踩过一次）。

### 3. 不可移动，只能 unique_ptr

`ClientSession` 内含 `ServerHandshakeStateMachine`，后者有引用成员（不可重绑），因此
`ClientSession` 的拷贝与移动全部 `= delete`。必须经 `unique_ptr` 存入容器，不要"顺手"补移动构造。

### 4. `ServerNetwork::tick()` 里禁止重入

入站 handler 内递归调用 `tick()` / `pumpLocal()` 会造成入站队列的重入死循环。

### 5. accept 线程关闭（Linux 关服卡死）

`_beginAccept` 必须用 `async_accept` 回调链 + `io_context::run()`，不能在专用线程里写同步阻塞
`accept()`——Linux 上 `close()` listen socket 的 fd 不会中断阻塞中的 `::accept(fd)`，导致 join 永久
阻塞、关服卡死（Windows 正常，故极易漏测）。析构时 `m_ioContext->stop()` 是唯一可靠的唤醒手段。

### 6. Wire 入站队列是 Wire-only 的

Local 连接不走 `enqueueInbound`/`drainInbound`（`pumpLocal()` 已在主线程直驱）。给 Local 连接也接队列
只会引入一次多余的 tick 延迟。

### 7. 延迟断开队列

`_notifyDisconnect` 在**接收线程**被调用，只允许 push `sessionId`，不得触碰连接或 session map。
`tick()` 在主线程 swap 出来后回调 `m_onDisconnect`，连接从 `m_connections` 的移除也在此完成。
