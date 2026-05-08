# Connection 模块

本模块提供网络连接抽象层，支持 TCP 远程连接和本地进程内通信。

## 目录结构

```
src/common/network/connection/
├── Connection.hpp              # 统一头文件
├── IServerConnection.hpp       # 服务端连接抽象接口
├── LocalConnection.hpp         # 本地连接端点声明
├── LocalConnection.cpp         # 本地连接端点实现
├── LocalServerConnection.hpp   # 本地服务端连接适配器声明
└── LocalServerConnection.cpp   # 本地服务端连接适配器实现
```

## 文件详解

### Connection.hpp

**职责**：统一头文件，方便外部引入所有连接相关的类和接口。

**内容**：
```cpp
#include "IServerConnection.hpp"
#include "LocalConnection.hpp"
#include "LocalServerConnection.hpp"
```

---

### IServerConnection.hpp

**职责**：定义服务端连接的抽象接口，使上层代码（如 `ServerWorld`、`EntityTracker`）能够与具体网络实现解耦。

**主要内容**：

```cpp
// 连接类型枚举
enum class ConnectionType : u8 {
    Tcp,    // TCP 远程连接
    Local   // 本地进程内连接
};

// 服务端连接接口
class IServerConnection {
public:
    virtual ~IServerConnection() = default;
    
    // 发送数据到对端
    virtual void send(const u8* data, size_t size) = 0;
    
    // 断开连接
    virtual void disconnect(const std::string& reason = "") = 0;
    
    // 检查连接状态
    [[nodiscard]] virtual bool isConnected() const = 0;
    
    // 获取连接标识符（用于日志和调试）
    [[nodiscard]] virtual std::string identifier() const = 0;
    
    // 获取连接类型
    [[nodiscard]] virtual ConnectionType type() const = 0;
};

// 类型别名
using ConnectionPtr = std::shared_ptr<IServerConnection>;
using ConnectionWeakPtr = std::weak_ptr<IServerConnection>;
```

**设计意图**：
- 支持多种连接类型（TCP、本地）
- 统一接口便于测试和模拟
- 允许 `IntegratedServer` 和 `StandaloneServer` 使用相同的上层逻辑

---

### LocalConnection.hpp / LocalConnection.cpp

**职责**：实现进程内通信机制，类似 Minecraft Java 版的 `LocalChannel`，用于集成服务器（单人游戏）场景。

**主要类**：

#### LocalEndpoint

线程安全的消息队列端点，支持双向通信。

```cpp
class LocalEndpoint {
public:
    // 发送数据到对端
    void send(const u8* data, size_t size);
    
    // 非阻塞接收
    bool receive(std::vector<u8>& outData);
    
    // 阻塞接收（带超时）
    bool receiveWait(std::vector<u8>& outData, u32 timeoutMs = 0);
    
    // 查询方法
    bool hasData() const;
    size_t pendingCount() const;
    bool isConnected() const;
    
    // 连接管理
    void connectTo(LocalEndpoint* remote);
    void disconnect();
};
```

**实现细节**：
- 使用 `std::queue` 存储消息
- 使用 `std::mutex` 和 `std::condition_variable` 实现线程安全
- 支持阻塞等待和非阻塞查询两种接收模式

#### LocalConnectionPair

管理一对相互连接的端点。

```cpp
class LocalConnectionPair {
public:
    void connect();     // 建立双向连接
    void disconnect();  // 断开双向连接
    
    LocalEndpoint& clientEndpoint();  // 客户端端点
    LocalEndpoint& serverEndpoint();  // 服务端端点
};
```

**通信模型**：
```
+------------------+          +------------------+
|  ClientEndpoint  | <------> |  ServerEndpoint  |
+------------------+          +------------------+
       |                             |
   主线程使用                    服务端线程使用
```

---

### LocalServerConnection.hpp / LocalServerConnection.cpp

**职责**：将 `LocalEndpoint` 适配到 `IServerConnection` 接口，用于 `IntegratedServer` 的进程内通信。

**主要类**：

```cpp
class LocalServerConnection : public IServerConnection {
public:
    explicit LocalServerConnection(LocalEndpoint* endpoint);
    
    // IServerConnection 接口实现
    void send(const u8* data, size_t) override;
    void disconnect(const std::string& reason = "") override;
    bool isConnected() const override;
    std::string identifier() const override;
    ConnectionType type() const override;
    
    // 本地连接特有方法
    LocalEndpoint* endpoint() const;
};
```

**设计特点**：
- 不持有 `LocalEndpoint` 的所有权（裸指针）
- 使用静态计数器生成唯一标识符
- 类型固定返回 `ConnectionType::Local`

---

## 类关系图

```
                    ┌─────────────────────────┐
                    │   IServerConnection     │  (抽象接口)
                    │─────────────────────────│
                    │ + send()                │
                    │ + disconnect()          │
                    │ + isConnected()         │
                    │ + identifier()          │
                    │ + type()                │
                    └───────────┬─────────────┘
                                │
                                │ 实现
                                │
                    ┌───────────┴─────────────┐
                    │  LocalServerConnection  │  (适配器)
                    │─────────────────────────│
                    │ - m_endpoint: *Local    │
                    │ - m_id: u64             │
                    └───────────┬─────────────┘
                                │
                                │ 持有指针
                                │
                    ┌───────────┴─────────────┐
                    │     LocalEndpoint       │  (底层通信)
                    │─────────────────────────│
                    │ - m_queue: queue        │
                    │ - m_mutex: mutex        │
                    │ - m_cv: cond_var        │
                    │ - m_remote: *Local      │
                    └─────────────────────────┘
                                │
                                │ 相互引用
                                │
                    ┌───────────┴─────────────┐
                    │   LocalConnectionPair   │  (连接管理)
                    │─────────────────────────│
                    │ - m_clientEndpoint      │
                    │ - m_serverEndpoint      │
                    └─────────────────────────┘
```

---

## 模块职责

### 整体职责

1. **抽象网络通信**：提供统一的连接接口，隔离上层代码与底层网络实现
2. **支持多种连接类型**：TCP 远程连接、本地进程内连接
3. **线程安全通信**：为集成服务器提供高效的进程内通信机制
4. **解耦服务器实现**：使 `IntegratedServer` 和 `StandaloneServer` 能共享核心逻辑

### 输入

| 来源 | 输入内容 |
|------|----------|
| 上层调用者 | 二进制数据包（待发送） |
| 对端端点 | 二进制数据包（待接收） |
| 连接管理 | 连接/断开命令 |

### 输出

| 目标 | 输出内容 |
|------|----------|
| 对端端点 | 二进制数据包 |
| 上层调用者 | 接收状态、连接状态、标识符 |

---

## 依赖项

### 内部依赖

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | 基础类型定义（u8, std::string 等） |

### 外部依赖

| 库 | 用途 |
|----|------|
| `<mutex>` | 线程同步 |
| `<condition_variable>` | 条件变量 |
| `<queue>` | 消息队列 |
| `<vector>` | 数据缓冲区 |
| `<memory>` | 智能指针 |
| `spdlog` | 日志输出（LocalServerConnection） |

---

## 使用方法

### 基本使用流程

```cpp
#include "common/network/connection/Connection.hpp"

using namespace mc::network;

// 1. 创建本地连接对
auto connectionPair = std::make_unique<LocalConnectionPair>();
connectionPair->connect();

// 2. 创建服务端连接适配器
ConnectionPtr serverConn = std::make_shared<LocalServerConnection>(
    &connectionPair->serverEndpoint()
);

// 3. 发送数据
u8 data[] = {1, 2, 3, 4, 5};
serverConn->send(data, sizeof(data));

// 4. 客户端接收
std::vector<u8> received;
connectionPair->clientEndpoint().receive(received);

// 5. 断开连接
serverConn->disconnect("Shutdown");
connectionPair->disconnect();
```

### 在 IntegratedServer 中的使用

```cpp
// IntegratedServer.cpp 中的典型用法

// 创建连接对
m_connectionPair = std::make_unique<LocalConnectionPair>();
m_connectionPair->connect();

// 服务端端点
LocalEndpoint* serverEndpoint = &m_connectionPair->serverEndpoint();

// 创建连接适配器并注册玩家
auto conn = std::make_shared<LocalServerConnection>(serverEndpoint);
m_playerManager->addPlayer(playerId, username, conn);

// 发送数据包
void sendGamePacket(LocalEndpoint* endpoint, PacketType type, const Packet& packet) {
    PacketSerializer payload;
    packet.serialize(payload);
    auto fullPacket = ConnectionManager::encapsulatePacket(type, payload.buffer());
    endpoint->send(fullPacket.data(), fullPacket.size());
}
```

### 在客户端 NetworkClient 中的使用

```cpp
// NetworkClient.cpp 中的用法

// 设置本地端点（用于集成服务器）
void NetworkClient::setLocalEndpoint(LocalEndpoint* endpoint) {
    m_localEndpoint = endpoint;
}

// 从本地端点接收数据
void NetworkClient::pollLocalEndpoint() {
    if (m_localEndpoint && m_localEndpoint->isConnected()) {
        std::vector<u8> data;
        while (m_localEndpoint->receive(data)) {
            processPacket(data);
        }
    }
}
```

---

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

`receiveWait()` 默认无限等待，可能导致线程阻塞。

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

---

## 测试用例

### LocalServerConnectionTest.cpp

| 测试名称 | 测试内容 |
|----------|----------|
| `BasicSendReceive` | 基本发送和接收功能 |
| `Disconnect` | 断开连接功能 |
| `Identifier` | 标识符格式验证 |
| `SendWhenDisconnected` | 断开后发送不崩溃 |
| `NullEndpoint` | 空端点处理 |
| `UseThroughInterface` | 通过接口使用 |

### ConnectionManagerTest.cpp

| 测试名称 | 测试内容 |
|----------|----------|
| `SendToPlayer` | 发送数据给指定玩家 |
| `SendPacketToPlayer` | 发送数据包给指定玩家 |
| `Broadcast` | 广播给所有玩家 |
| `BroadcastExcept` | 排除指定玩家广播 |
| `BroadcastPacket` | 广播数据包 |
| `DisconnectPlayer` | 断开指定玩家连接 |
| `DisconnectAll` | 断开所有连接 |
| `CleanupDisconnectedPlayers` | 清理断开连接的玩家 |
| `EncapsulatePacket` | 数据包封装 |

### 相关测试文件

- `tests/server/core/PlayerManagerTest.cpp` - 玩家管理测试
- `tests/server/ServerWorldTest.cpp` - 服务器世界测试
- `tests/server/core/TeleportManagerTest.cpp` - 传送管理测试
- `tests/server/core/KeepAliveManagerTest.cpp` - 心跳管理测试
- `tests/server/core/PositionTrackerTest.cpp` - 位置跟踪测试

---

## 扩展指南

### 添加新的连接类型

1. 继承 `IServerConnection` 接口：

```cpp
class TcpServerConnection : public IServerConnection {
public:
    void send(const u8* data, size_t size) override;
    void disconnect(const std::string& reason = "") override;
    bool isConnected() const override;
    std::string identifier() const override;
    ConnectionType type() const override { return ConnectionType::Tcp; }
    
private:
    std::unique_ptr<asio::ip::tcp::socket> m_socket;
};
```

2. 在 `Connection.hpp` 中添加包含：

```cpp
#include "TcpServerConnection.hpp"
```

### 添加新功能

1. **连接统计**：在 `IServerConnection` 中添加 `bytesSent()`、`bytesReceived()` 等方法
2. **连接事件**：添加 `onDisconnect` 回调
3. **流量控制**：实现背压机制

---

## 版本历史

| 版本 | 变更 |
|------|------|
| 初始版本 | 实现基本连接抽象和本地连接支持 |
