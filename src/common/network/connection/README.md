# Connection 模块

本模块提供网络连接抽象层，支持 TCP 远程连接和本地进程内通信。

## 目录结构

```
src/common/network/connection/
├── Connection.hpp              # 统一头文件
├── IServerConnection.hpp       # 服务端连接抽象接口
├── LocalConnection.hpp/cpp     # 本地连接端点（线程安全消息队列）
├── LocalServerConnection.hpp/cpp  # 本地连接适配器（将 LocalEndpoint 适配到 IServerConnection）
└── README.md
```

## 内部模块关系

```
┌─────────────────────────┐
│   IServerConnection     │  (抽象接口)
│─────────────────────────│
│ + send()                │
│ + disconnect()          │
│ + isConnected()         │
│ + identifier()          │
│ + type()                │
│ + getAddress()          │
└───────────┬─────────────┘
            │ 实现
            ▼
┌─────────────────────────┐
│  LocalServerConnection  │  (适配器)
│─────────────────────────│
│ - m_endpoint: *Local    │
│ - m_id: u64             │
└───────────┬─────────────┘
            │ 持有指针
            ▼
┌─────────────────────────┐
│     LocalEndpoint       │  (底层通信)
│─────────────────────────│
│ - m_queue: queue        │
│ - m_mutex: mutex        │
│ - m_cv: cond_var        │
│ - m_remote: *Local      │
└─────────────────────────┘
            ▲
            │ 相互引用
            │
┌─────────────────────────┐
│   LocalConnectionPair   │  (连接管理)
│─────────────────────────│
│ - m_clientEndpoint      │
│ - m_serverEndpoint      │
└─────────────────────────┘
```

- **IServerConnection**：抽象接口，定义统一的连接操作
- **LocalServerConnection**：将 `LocalEndpoint` 适配到 `IServerConnection` 接口
- **LocalEndpoint**：线程安全的消息队列，支持双向通信
- **LocalConnectionPair**：管理一对相互连接的端点

## 上下游依赖关系

### 上游依赖（本模块依赖的）

| 模块/库 | 用途 |
|---------|------|
| `common/core/Types.hpp` | 基础类型定义（u8, u64, std::string 等） |
| `<mutex>` | 线程同步 |
| `<condition_variable>` | 条件变量 |
| `<queue>` | 消息队列 |
| `<vector>` | 数据缓冲区 |
| `<memory>` | 智能指针 |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `server/player/ServerPlayer.hpp` | 服务端玩家实体持有连接引用 |
| `server/network/TcpConnection.hpp` | TCP 连接实现 `IServerConnection` 接口 |
| `server/core/PlayerManager.hpp` | 玩家管理器管理连接生命周期 |
| `server/core/PacketHandler.hpp` | 数据包处理器通过连接发送数据 |
| `server/application/IntegratedServer.hpp/cpp` | 集成服务器使用 `LocalConnectionPair` |
| `client/network/NetworkClient.hpp` | 客户端网络层使用本地端点 |

## 容易踩的坑

### 1. LocalServerConnection 不持有端点所有权

`LocalServerConnection` 使用裸指针持有 `LocalEndpoint`，不管理其生命周期。

```cpp
// 错误示例：端点被销毁后继续使用
auto endpoint = &pair.serverEndpoint();
auto conn = std::make_shared<LocalServerConnection>(endpoint);
pair.disconnect();  // 端点可能无效
conn->send(data, size);  // 危险！悬空指针
```

**解决方案**：确保 `LocalConnectionPair` 的生命周期长于 `LocalServerConnection`。

### 2. 阻塞接收可能导致死锁

`receiveWait()` 默认无限等待（timeoutMs = 0），可能导致线程永久阻塞。

```cpp
// 潜在问题：如果对端永不发送数据，将永久阻塞
std::vector<u8> data;
endpoint.receiveWait(data);  // 危险！
```

**解决方案**：使用超时参数或确保有其他机制通知对端发送数据。

```cpp
// 推荐做法：设置超时
if (endpoint.receiveWait(data, 1000)) {  // 1秒超时
    // 处理数据
}
```

### 3. 忘记调用 connect()

创建 `LocalConnectionPair` 后必须显式调用 `connect()`。

```cpp
// 错误示例
auto pair = std::make_unique<LocalConnectionPair>();
// 忘记调用 pair->connect()
pair->serverEndpoint().send(data, size);  // 数据不会被发送
```

### 4. 线程安全问题

虽然 `LocalEndpoint` 是线程安全的，但调用者仍需注意：

```cpp
// 问题：多线程同时调用 disconnect 和 send
// 线程1
conn->disconnect("reason");
// 线程2
conn->send(data, size);  // 可能发送到已断开的端点
```

**解决方案**：使用更高层的同步机制或确保单线程操作。

### 5. 消息积压

如果接收方处理速度慢于发送方，消息队列会无限增长。

```cpp
// 潜在问题：大量发送，少量接收
for (int i = 0; i < 1000000; i++) {
    endpoint.send(data, size);  // 队列可能变得很大
}
```

**解决方案**：监控 `pendingCount()` 或实现背压机制。
