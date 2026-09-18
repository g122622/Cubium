# network/base - 连接基座

服务端网络子系统的**依赖链末梢**：只依赖 `common/network/`，不含任何 `server/application` 或
`server/world` 依赖。`session/` 及以上各层都依赖本目录，反向依赖不存在。

## 目录结构

```
src/server/network/base/
├── README.md
├── IServerClientConnection.hpp    # 业务侧最小连接视图接口（5 个纯虚）
├── ServerClientConnection.hpp     # 单连接实现（Local/Wire 双模）+ HandshakeState 枚举 + 两个类型别名
└── ServerClientConnection.cpp
```

- `IServerClientConnection`：只暴露 `send` / `close` / `isConnected` / `peerAddress` / `disconnect`，
  刻意屏蔽 `raw()`、`setInboundHandler`、`drainInbound`、`setState`、`setupEncryption`。它的存在意义是
  让 `server/core` 的 `PlayerManager` / `ServerPlayerData` / `ConnectionManager` 不必拖入 asio 与
  `pipeline::Connection` 模板，同时让测试桩无需构造 transport 与协议表。
- `ServerClientConnection`：包裹 `pipeline::Connection<RegistryByteBuf>` + `HandshakeState` + `sessionId`。
  Local（集成服同进程）与 Wire（TCP）两种模式共用同一套握手/Play 流程，仅构造时注入的 transport 不同。
- `ClientConn` / `ProtocolTables`：Java 后端连接实例与协议表的类型别名，供 `session/ServerNetwork` 复用。

## 内部模块关系

`IServerClientConnection` ← 继承 ← `ServerClientConnection`；后者**值持有** `ClientConn`。

Wire 模式下 `onPacket` 监听器由 transport 接收线程触发，因此 `ServerClientConnection` 自带一个入站队列：
`enqueueInbound`（接收线程，锁内 push）与 `drainInbound`（主线程，锁内 swap 后锁外派发）。
Local 模式不使用该队列，由 `pumpLocal()` 主线程直驱。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/pipeline/Connection.hpp` | 连接管线门面 |
| `common/network/ir/IrPacket.hpp` | IR 包类型 |
| `common/network/crypto/Crypt.hpp` | `kSharedSecretBytes`（RSA 握手） |
| `common/network/transport/{LocalTransport,TcpTransport}.hpp` | 两种 transport（TcpTransport 仅前置声明） |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `server/core/{PlayerManager,ServerPlayerData}` | 持 `IServerClientConnection*` 发包 |
| `server/player/ServerPlayer` | 持 `ServerClientConnection*` |
| `server/network/session/*` | `ServerNetwork` 拥有连接；`ClientSession` 持非拥有指针 |
| `server/network/handshake/*` | `ServerHandshakeStateMachine` 构造时绑 `ServerClientConnection&` |
| `tests/common/{BaseTestServer,network/*}` | 测试桩实现 `IServerClientConnection` |

## 容易踩的坑

### 1. 成员声明顺序：`m_peerAddress` 必须先于 `m_conn`

Wire 构造的初始化列表按**成员声明顺序**求值：先取 `wireTransport->remoteAddress()` 快照地址，再
`move` wireTransport 进 `m_conn`。若把两个成员的声明顺序颠倒（例如"顺手"按字母序整理），
就会在 move 之后读空指针，产生难查的崩溃。`ServerClientConnection.hpp` 中该顺序有显式注释，不要改动。

### 2. `TcpTransport` 只前置声明，不要改成 include

`TcpTransport.hpp` 是全项目唯一拖入 `<asio.hpp>` 的传输头。构造签名里出现
`std::unique_ptr<TcpTransport>` 无需完整类型，故本目录只前置声明它。
改回 `#include` 会让 `LoginFlow.hpp` / `ServerHandshake.hpp` / `ServerPlayer.hpp` /
`ServerPlayerEntityManager.hpp` 等**仅需连接类型**的头全部被迫拉入 asio，显著拖慢编译。
真正需要完整类型的只有 `ServerClientConnection.cpp` 与 `session/ServerNetwork.cpp`，二者自行 include。

### 3. `disconnect()` 的 reason 必须是纯文本

codec 会把 reason 编码为 NBT `StringTag`（对应 vanilla `Component.literal(text)` 的纯文本折叠路径）。
传入 JSON 字符串会让客户端把首字节当 tag id，报 `Invalid tag id`。

### 4. `HandshakeState` 与 `Connection::phase()` 语义部分重合

`ConnectionProtocol` 能表达 `Handshaking`/`Login`/`Play`，只有 Configuration 的"子进度"是额外信息。
`HandshakeState` 是**连接自身的状态成员**，唯一读写方是 `handshake/ServerHandshakeStateMachine`
（经 `state()`/`setState()`），不要在其他层直接改它。
