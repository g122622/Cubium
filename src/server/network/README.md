# Server Network Module

本目录包含服务端网络通信模块。基于新 IR 层（`common/network/` 的 `pipeline/`+`transport/`+`ir/`+`backend/`），统一门面为 `ServerNetwork`，同时承载：

- **Local 模式**（集成服务器同进程）：经 `createLocalClientSide` 建 `LocalTransportPair`，本地客户端与服务端 `ServerClientConnection` 配对，零拷贝直传 `ir::IrPacket`，不经过序列化。
- **Wire 模式**（独立服务器 / 集成服务器局域网发布）：经 `startAccept(port, max)` 在专用线程 accept TCP 连接，每连接建 `TcpTransport`（asio + VarInt21 帧化）→ Wire 模式 `ServerClientConnection`。

> **已删除**：旧 `TcpServer.hpp/.cpp`、`TcpSession.hpp/.cpp`（裸 Winsock/POSIX socket + 手动 poll + 12 字节头旧帧）已彻底删除，零生产引用。远程 TCP 玩家的连接封装由 `ServerClientConnection`（定义于 `ServerNetwork.hpp`）承担，传输底层为 `common/network/transport/TcpTransport`。
>
> **已删除**：旧 `TcpConnection.hpp/.cpp`（IServerConnection 适配器）已删除。`IServerConnection`/`LocalConnection` 旧传输抽象一并清除。

## 目录结构

```
src/server/network/
├── ServerNetwork.hpp       # 服务端网络门面（accept + 管理连接，定义 ServerClientConnection）
├── ServerNetwork.cpp       # 服务端网络门面实现
├── ServerHandshake.hpp     # 握手状态机（每连接一个，离线/在线模式）
├── ServerHandshake.cpp     # 握手状态机实现
├── ServerPlayRouter.hpp    # 入站 Play 包分发器（std::visit over ir::PlayPacket）
├── ServerPlayRouter.cpp    # 分发器实现
├── RegistryDataBuilder.hpp # Configuration 阶段 RegistryData 构造（data=nullopt，标 TODO Phase6）
├── RegistryDataBuilder.cpp # RegistryData 实现
└── README.md               # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│            StandaloneServer / IntegratedServer              │
│              (应用层，持有 ServerNetwork)                    │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      ServerNetwork                          │
│  (accept + 管理 ServerClientConnection 集合 + Local/Wire    │
│   统一 tick：pumpLocal 主线程派发 / drainInbound 主线程派发)  │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                  ServerClientConnection                     │
│  (per-connection 包装：ClientConn + sessionId + 握手状态)    │
│  ┌─ Local 模式：LocalTransport 直传 ir::IrPacket             │
│  └─ Wire  模式：TcpTransport → pipeline 解帧/解压/解密        │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│  common/network/transport/                                  │
│   LocalTransport (同进程零拷贝 IR)                           │
│   TcpTransport   (asio + VarInt21 帧化)                      │
└─────────────────────────────────────────────────────────────┘
```

**数据流**：
- **入站（Local）**：客户端 `Connection::send(ir)` → `LocalTransport` 直传 → 服务端 `ServerClientConnection::onPacket` → `pumpLocal()` 主线程派发 → 握手/`ServerPlayRouter::handle()`（Play 阶段）按分支调 `MinecraftServer` 既有处理逻辑
- **入站（Wire）**：客户端字节 → `TcpTransport`（接收线程）→ `pipeline::Connection` 解帧/解压/解密 → `ir::IrPacket` → `enqueueInbound`（接收线程入队）→ `drainInbound`（主线程）→ 握手/`ServerPlayRouter::handle()`
- **出站**：上层 → `ServerClientConnection::send(ir::IrPacket)` → `pipeline::Connection::send` → Local 直传 / Wire 编码+压缩+加密 → transport 发送

## 使用场景

### 1. StandaloneServer（独立服务器）
`StandaloneServer` 在 `initialize()` 中创建 `ServerNetwork` 并 `startAccept(serverPort, maxPlayers)`。所有玩家均通过 TCP 连接（Wire 模式）。每 accept 一个连接即建 `RemoteClientSession`（握手状态机 + Play 路由器），握手完成（进入 Play）后由 `onPlayerReady` 回调触发 `createPlayerForConnection` 创建玩家实体。

### 2. IntegratedServer（集成服务器局域网发布）
`IntegratedServer` 默认使用 `LocalTransport` 与本地客户端同进程零拷贝直传 IR 包（`sessionId == 0`，`initialize()` 内联接线）。当执行 `/publish [port] [allowCheats]` 命令时，`publishToLan()` 调 `ServerNetwork::startAccept` 启动 TCP 监听，接受远程玩家（`sessionId != 0`，Wire 模式）。

**双路径架构**：发布后，单 `ServerNetwork` 同时服务本地客户端（Local，`sessionId == 0`）与远程 TCP 玩家（Wire，`sessionId != 0`）。`startAccept` 触 `m_listenPort/m_ioContext/m_acceptor/m_acceptThread`；`createLocalClientSide` 触 `m_connections/m_onConnect`——成员不相交无冲突。单 `tick()` 经 `isLocalMode()` 分支同时 drain Local(pumpLocal)+Wire(drainInbound)。详见 `src/server/application/README.md` 第 11 节。

**与 StandaloneServer 的差异**：
- `IntegratedServer` 的本地客户端保留 `LocalTransport` 零拷贝优化路径，TCP 仅用于远程玩家。
- 远程玩家走 `InventoryManager` / `ContainerManager` 多玩家路径，本地客户端走 `m_clientInventory` / `m_openMenu` 单玩家优化路径。

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖项 | 路径 | 用途 |
|--------|------|------|
| 基础类型 | `src/common/core/Types.hpp` | u8, u16, u32, u64, std::string 等 |
| 错误处理 | `src/common/core/Result.hpp` | Result<T>, Error, ErrorCode |
| IR 包 | `src/common/network/ir/IrPacket.hpp` | 协议无关 IR 包定义 |
| 连接管线 | `src/common/network/pipeline/Connection.hpp` | `pipeline::Connection<RegistryByteBuf>` 门面 |
| 传输层 | `src/common/network/transport/LocalTransport.hpp` | 同进程零拷贝 IR 传输 |
| 传输层 | `src/common/network/transport/TcpTransport.hpp` | asio TCP + VarInt21 帧化 |
| 日志 | `spdlog` (vcpkg) | 日志输出 |
| 系统网络 | Winsock2 (Windows) / POSIX Socket (Linux) | 底层 socket API（asio 封装） |

### 依赖本模块的外部模块

| 模块 | 路径 | 用途 |
|------|------|------|
| StandaloneServer | `src/server/application/StandaloneServer.hpp` | 使用 `ServerNetwork::startAccept` 接受远程玩家 |
| IntegratedServer | `src/server/application/IntegratedServer.hpp` | `publishToLan()` 调 `ServerNetwork::startAccept` 接受远程玩家 |
| RemoteClientSession | `src/server/application/RemoteClientSession.hpp` | 两子类共用：持握手状态机 + Play 路由器，值持有存入 unique_ptr 容器 |
| ConnectionManager | `src/server/core/ConnectionManager.hpp` | 服务端 IR 发送门面，经 ServerClientConnection 发包 |
| ServerPlayer | `src/server/player/ServerPlayer.hpp` | 通过 ServerClientConnection 发送数据 |
| MinecraftServer | `src/server/application/MinecraftServer.hpp` | `routeInboundPlayPacket` 委托 ServerPlayRouter 分发；`createPlayerForConnection`/`sendLoginResponseForConnection` 共享登录序列 |

## 容易踩的坑

### 1. Wire 入站线程安全（入站队列 + 主线程 drain）

**问题**：`TcpTransport::_receiveLoop` 在接收线程同步调 `Connection::_handleWireBytes` → `_decodeAndDispatch` → `m_listener(packet)`。若直接在监听器里调 `routeInboundPlayPacket`，会在接收线程触碰非线程安全的世界状态。

**处理**：Wire 模式连接的 `onPacket` 监听器仅 `enqueueInbound`（接收线程，mutex 守 deque push），`ServerNetwork::tick()` 在主线程 `drainInbound`（锁内 swap 出本地 deque，锁外逐个调 `setInboundHandler` 装配的握手/Play 分支）。Local 模式不经队列，`pumpLocal()` 直接主线程派发。队列是 Wire-only 关注点。

### 2. 断开检测（onClientDisconnect + 延迟 sid 列表）

**问题**：`TcpTransport::onDisconnect` 在接收线程触发。若跨线程直接改 session map 会与主线程 tick 竞争。

**处理**：`ServerNetwork` 有 `onClientDisconnect(cb)`；`_notifyDisconnect` 在接收线程锁内 push `sessionId` 到 `m_disconnectedSessions`（不碰 session map）。`tick()` 末尾锁内 swap 出 sid 列表，锁外逐 sid 调 `m_onDisconnect`，子类主线程做 session map 清理 + 移除玩家 + 清库存。所有 `RemoteClientSession` map 变动在主线程。

### 3. RemoteClientSession 生命周期（非拥有指针 + 销毁顺序）

**问题**：`RemoteClientSession` 持 `ServerClientConnection&`（非拥有，所有权归 `ServerNetwork::m_connections`）。若连接先于 session 销毁则悬垂。

**处理**：子类 `stop()` 中 `m_remoteSessions.clear()` 须先于 `m_serverNetwork.reset()`。`ServerHandshakeStateMachine` 含引用成员不可重绑，故 `RemoteClientSession` 删除移动语义，经 `unique_ptr` 存入容器。

### 4. 包大小限制

`TcpTransport`/`pipeline` 走 VarInt21 帧化 + zlib 压缩（threshold=256）。解压后有最大包大小上限，超过判非法断开。

### 5. 线程安全

`tick()` 在主线程调用，采用快照-后-pump 模式（锁内收集连接/sid 列表，锁外回调），避免 handler 重入死锁。回调中避免阻塞操作。

### 6. accept 线程关闭（Linux 关服卡死坑）

**问题**：旧实现 `_beginAccept` 用同步阻塞 `m_acceptor->accept(socket, ec)`。关服时 `~ServerNetwork` 调 `m_acceptor->close()` 试图唤醒 accept 线程，但 **Linux 上 `close()` 一个 listen socket 的 fd 并不会中断正阻塞在 `::accept(fd)` 系统调用中的线程**，导致 `m_acceptThread->join()` 永久阻塞、关服卡死。Windows 上 `closesocket()` 会立即让阻塞的 `accept()` 返回错误，故 Windows 正常、Linux 卡死。

**处理**：`_beginAccept` 改为 `async_accept` 回调链 + `m_ioContext->run()` 驱动。`~ServerNetwork` 改为 `m_ioContext->stop()` + `join()`——`stop()` 让 `io_context::run()` 在处理完当前回调后返回，可靠唤醒 accept 线程，跨平台一致。新增连接的 accept 必须用异步链而非同步循环，否则同一坑会复发。

### 7. Winsock 初始化

Windows 需要 Winsock。asio 在 `io_context` 运行时自动管理，无需手动 `WSAStartup`。`ws2_32` 链接库仍需在 CMake 中保留（`src/server/CMakeLists.txt`）。

### 8. RegistryData NBT（刻意保留，非阻塞）

`RegistryDataBuilder` 当前以 `RegistryEntry{id, data=nullopt}` 发送所有条目（声明"客户端已知"），仅在我方互通双方均硬编码 vanilla registry 且 `SelectKnownPacks{minecraft:core}` 命中时合法。客户端命中 core 后依赖本地硬编码 vanilla registry，无需消费 NBT；真 Java 互通时我方服务端同样发 data=nullopt，真客户端用其本地 registry——故 NBT 消费路径在 core 命中前提下永不触发，刻意保留为占位。若卡在 Configuration 挂死先查此（确认 SelectKnownPacks 命中 core）。未来支持非 core 数据包协商再补 NBT 推送与消费。
