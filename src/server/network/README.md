# Server Network Module

本目录包含服务端 TCP 网络通信模块，负责独立服务器（StandaloneServer）的远程客户端连接管理。

## 目录结构

```
src/server/network/
├── TcpServer.hpp       # TCP 服务器核心，监听端口、接受连接、管理会话
├── TcpServer.cpp       # TCP 服务器实现（跨平台：Windows Winsock2 / Linux POSIX）
├── TcpSession.hpp      # 单个客户端会话，管理状态、缓冲区、统计信息
├── TcpSession.cpp      # 会话实现，数据包解析与状态流转
├── TcpConnection.hpp   # 将 TcpSession 适配到 IServerConnection 接口
├── TcpConnection.cpp   # 连接适配器实现，供上层游戏逻辑使用
└── README.md           # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                    StandaloneServer                          │
│                      (应用层)                                 │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      TcpServer                               │
│              (监听端口、管理会话、广播)                         │
└─────────────────────────────────────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
┌─────────────────────────┐   ┌─────────────────────────────┐
│      TcpSession         │   │      TcpConnection          │
│  (会话状态、数据缓冲)     │   │  (适配到 IServerConnection)  │
└─────────────────────────┘   └─────────────────────────────┘
              │                           │
              ▼                           ▼
┌─────────────────────────┐   ┌─────────────────────────────┐
│   PacketDeserializer    │   │    IServerConnection        │
│    (数据包反序列化)       │   │      (统一连接接口)           │
└─────────────────────────┘   └─────────────────────────────┘
```

**数据流**：
- **入站**：客户端 → TcpServer.poll() → TcpSession.handleReceivedData() → 解析完整包 → onPacket 回调
- **出站**：上层调用 → TcpConnection.send() → TcpSession.send() → 发送队列 → socket

**会话状态流转**：Connecting → Connected → Authenticating → Playing → Disconnecting → Disconnected

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖项 | 路径 | 用途 |
|--------|------|------|
| 基础类型 | `src/common/core/Types.hpp` | u8, u16, u32, u64, std::string 等 |
| 错误处理 | `src/common/core/Result.hpp` | Result<T>, Error, ErrorCode |
| 数据包基类 | `src/common/network/packet/Packet.hpp` | Packet、PacketType 枚举 |
| 序列化器 | `src/common/network/packet/PacketSerializer.hpp` | 数据包序列化 |
| 连接接口 | `src/common/network/connection/IServerConnection.hpp` | 服务端连接抽象接口 |
| 日志 | `spdlog` (vcpkg) | 日志输出 |
| 系统网络 | Winsock2 (Windows) / POSIX Socket (Linux) | 底层 socket API |

### 依赖本模块的外部模块

| 模块 | 路径 | 用途 |
|------|------|------|
| StandaloneServer | `src/server/core/StandaloneServer.hpp` | 使用 TcpServer 作为网络层 |
| ConnectionManager | `src/server/core/ConnectionManager.hpp` | 管理 TcpConnection 适配器 |
| ServerPlayer | `src/server/player/ServerPlayer.hpp` | 通过 IServerConnection 发送数据 |

## 容易踩的坑

### 1. Socket 句柄未存储到 TcpSession

**问题**：当前实现中 `handleSessionData()` 和 `sendSessionData()` 标记为 TODO，TcpSession 没有存储 socket 句柄，导致无法实际收发数据。

**解决方案**：需要扩展 TcpSession 存储 socket 句柄：
```cpp
// TcpSession.hpp 中添加
#ifdef _WIN32
    uintptr_t m_socket = INVALID_SOCKET;
#else
    int m_socket = -1;
#endif

// TcpServer::acceptNewConnection() 中设置
session->setSocket(clientSocket);
```

### 2. 非阻塞模式下的部分接收

**问题**：非阻塞 socket 可能只收到部分数据。

**当前处理**：正确实现了缓冲区和期望大小机制，会等待完整包才处理。

### 3. 线程安全

**问题**：`poll()` 在主线程调用，但回调可能修改共享状态。

**建议**：
- 回调中避免阻塞操作
- 使用 `std::mutex` 保护共享数据
- 考虑使用消息队列解耦

### 4. 包大小限制

**当前限制**：最大包大小 64KB。如果发送大包（如区块数据），需要确认不超过此限制。限制在 `TcpSession.cpp` 中：
```cpp
if (m_expectedSize > 65536) { // 64KB最大包大小
    disconnect("Packet too large");
}
```

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

### 7. 数据包序列化失败静默处理

**问题**：`sendPacket()` 中序列化失败时只记录日志，不通知调用者，调用者无法得知发送是否成功。

**建议**：返回 `Result<void>` 让调用者处理错误。

### 8. TcpConnection 与 LocalConnection 的区别

- `TcpConnection`：用于 StandaloneServer（多人服务器），包装远程 TcpSession
- `LocalConnection`：用于 IntegratedServer（单机模式），本地 IPC

上层代码应通过 `IServerConnection` 接口操作，不要直接依赖具体实现类型。
