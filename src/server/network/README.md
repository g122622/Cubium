# Server Network Module

本目录包含服务端网络通信模块。当前为过渡状态：新 IR 层（`common/network/` 的 `pipeline/`+`transport/`+`ir/`+`backend/`）已落地并承载集成服务器的本地通信，旧的 `TcpServer`/`TcpSession` 仍保留用于独立服务器和局域网发布的远程 TCP 路径（Phase6 待迁移到 `ServerNetwork::startAccept` + `TcpTransport`）。

> **已删除**：旧 `TcpConnection.hpp/.cpp`（IServerConnection 适配器）已删除，零生产引用。远程 TCP 玩家的连接封装由 `ServerClientConnection`（定义于 `ServerNetwork.hpp`）承担。
>
> **新层（主架构）**：`ServerNetwork.hpp/.cpp`（accept + 管理连接）、`ServerHandshake.hpp/.cpp`（握手状态机）、`ServerPlayRouter.hpp/.cpp`（入站 Play 分发器，`std::visit` over `ir::PlayPacket`）。

## 目录结构

```
src/server/network/
├── ServerNetwork.hpp       # 服务端网络门面（accept + 管理连接，定义 ServerClientConnection）
├── ServerNetwork.cpp       # 服务端网络门面实现
├── ServerHandshake.hpp     # 握手状态机
├── ServerHandshake.cpp     # 握手状态机实现
├── ServerPlayRouter.hpp    # 入站 Play 包分发器（std::visit over ir::PlayPacket）
├── ServerPlayRouter.cpp    # 分发器实现
├── TcpServer.hpp           # 【Phase6 保留】TCP 服务器核心，监听端口、接受连接、管理会话
├── TcpServer.cpp           # TCP 服务器实现（跨平台：Windows Winsock2 / Linux POSIX）
├── TcpSession.hpp          # 【Phase6 保留】单个客户端会话，管理状态、缓冲区、统计信息
├── TcpSession.cpp          # 会话实现，数据包解析与状态流转
└── README.md               # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│            StandaloneServer / IntegratedServer              │
│              (应用层，持有 ServerNetwork / TcpServer)        │
└─────────────────────────────────────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
┌─────────────────────────┐   ┌─────────────────────────────┐
│      ServerNetwork      │   │      TcpServer (Phase6)     │
│  (新 IR 门面，accept +   │   │  (监听端口、管理会话、广播)  │
│   管理 ServerClientConn) │   └────────────┬────────────────┘
└────────────┬────────────┘                │
              │                             ▼
              ▼                    ┌─────────────────────┐
┌─────────────────────────┐        │      TcpSession     │
│ ServerClientConnection  │        │  (会话状态、数据缓冲) │
│  (per-connection 包装)  │        └─────────────────────┘
└────────────┬────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  common/network/transport/          │
│   LocalTransport (同进程零拷贝 IR)   │
│   TcpTransport   (asio + VarInt21)  │
└─────────────────────────────────────┘
```

**数据流**：
- **入站（新层）**：客户端 → transport 收字节 → `pipeline::Connection` 解帧/解压/解密 → `ir::IrPacket` → `ServerPlayRouter::handle()`（Play 阶段）按分支调 `MinecraftServer` 既有处理逻辑
- **入站（旧 TcpServer，Phase6）**：客户端 → TcpServer.poll() → TcpSession.handleReceivedData() → 解析完整包 → onPacket 回调
- **出站**：上层 → `ConnectionManager`（IR 门面）→ `pipeline::Connection::send(ir::IrPacket)` → transport 发送

## 使用场景

### 1. StandaloneServer（独立服务器）
`StandaloneServer` 在 `initialize()` 中创建网络层并监听 `serverPort`。所有玩家均通过 TCP 连接。当前走旧 `TcpServer`/`TcpSession` 基建（Phase6 迁移到 `ServerNetwork` + `TcpTransport` 进行中）。

### 2. IntegratedServer（集成服务器局域网发布）
`IntegratedServer` 默认使用 `LocalTransport` 与本地客户端同进程零拷贝直传 IR 包。当执行 `/publish [port] [allowCheats]` 命令时，`publishToLan()` 启动 TCP 监听器（旧 `TcpServer`/`TcpSession` 基建）接受远程玩家。

**双路径架构**：发布后，`IntegratedServer` 同时服务本地客户端（`sessionId == 0`，走 `LocalTransport`）与远程 TCP 玩家（`sessionId != 0`，走旧 `TcpServer`/`TcpSession`，Phase6 迁移后改为 `TcpTransport`）。详见 `src/server/application/README.md` 第 11 节。

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
| 旧数据包（Phase6） | `src/common/network/packet/Packet.hpp` | 旧 Packet 基类（TcpSession 帧解析仍用） |
| 日志 | `spdlog` (vcpkg) | 日志输出 |
| 系统网络 | Winsock2 (Windows) / POSIX Socket (Linux) | 底层 socket API |

### 依赖本模块的外部模块

| 模块 | 路径 | 用途 |
|------|------|------|
| StandaloneServer | `src/server/application/StandaloneServer.hpp` | 使用网络层接受远程玩家 |
| IntegratedServer | `src/server/application/IntegratedServer.hpp` | `publishToLan()` 后使用 TcpServer 接受远程玩家 |
| ConnectionManager | `src/server/core/ConnectionManager.hpp` | 服务端 IR 发送门面，经 ServerClientConnection 发包 |
| ServerPlayer | `src/server/player/ServerPlayer.hpp` | 通过 ServerClientConnection 发送数据 |
| MinecraftServer | `src/server/application/MinecraftServer.hpp` | `routeInboundPlayPacket` 委托 ServerPlayRouter 分发 |

## 容易踩的坑

### 1. Socket 句柄未存储到 TcpSession（Phase6 旧基建）

**问题**：当前实现中 `handleSessionData()` 和 `sendSessionData()` 标记为 TODO，TcpSession 没有存储 socket 句柄，导致无法实际收发数据。

**解决方案**：需要扩展 TcpSession 存储 socket 句柄（Phase6 迁移时一并处理，或直接由 `TcpTransport` 取代）。

### 2. 非阻塞模式下的部分接收（Phase6 旧基建）

**问题**：非阻塞 socket 可能只收到部分数据。

**当前处理**：旧 `TcpSession` 实现了缓冲区和期望大小机制，会等待完整包才处理。新层 `TcpTransport` 走 VarInt21 长度前缀帧化。

### 3. 线程安全

**问题**：`poll()` 在主线程调用，但回调可能修改共享状态。

**建议**：
- 回调中避免阻塞操作
- 使用 `std::mutex` 保护共享数据
- 考虑使用消息队列解耦

### 4. 包大小限制（Phase6 旧基建）

旧 `TcpSession` 最大包大小 64KB。新层 `TcpTransport`/`pipeline` 走 VarInt21 帧化 + 压缩，限制不同。迁移期间两套限制并存。

### 5. Winsock 初始化

**问题**：Windows 需要初始化 Winsock。

**当前处理**：使用全局 `WinsockInitializer` 自动管理初始化/清理，无需手动处理。

### 6. 会话状态检查

**问题**：在断开连接后继续使用会话会导致错误。

**建议**：使用前检查状态：
```cpp
if (session->state() != SessionState::Playing) {
    return;  // 会话不可用
}
```

### 7. 数据包序列化失败静默处理（Phase6 旧基建）

**问题**：旧 `sendPacket()` 中序列化失败时只记录日志，不通知调用者。新层 `pipeline::Connection::send` 返回 `Result<void>`。

### 8. 本地与远程路径区分

旧 `TcpConnection`（已删除）与 `LocalConnection`（已删除）的双实现接口 `IServerConnection`（已删除）均已不复存在。当前：
- 本地客户端走 `transport/LocalTransport`（同进程零拷贝 IR 包）
- 远程 TCP 玩家走旧 `TcpServer`/`TcpSession`（Phase6 迁移到 `transport/TcpTransport`）

上层代码通过 `ServerClientConnection`（定义于 `ServerNetwork.hpp`）统一操作，不要再引用已删除的 `IServerConnection`/`TcpConnection`/`LocalConnection` 类型。
