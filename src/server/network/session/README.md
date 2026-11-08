# 连接接入、断开与会话生命周期

## 目录结构树

```text
session/
├── keepalive/  # 会话心跳与超时判定
├── ClientSession.cpp  # 模块接口与实现
├── ClientSession.hpp  # 单个客户端的会话簿记：协议状态 + 入站派发
├── ClientSessionManager.cpp  # 模块接口与实现
├── ClientSessionManager.hpp  # 远程 TCP 会话管理门面（批2c 下沉自 StandaloneServer / IntegratedServer）
├── PlayerConnectionManager.cpp  # 模块接口与实现
├── PlayerConnectionManager.hpp  # 连接关闭入口；玩家记录与资源释放委托 PlayerManager。
├── ServerNetwork.cpp  # 模块接口与实现
└── ServerNetwork.hpp  # 服务端网络门面：管理所有 ServerClientConnection、TCP accept、协议表
```

## 内部模块关系

ServerNetwork 管理连接集合；ClientSession/ClientSessionManager 维护协议会话；PlayerConnectionManager 发起关闭并委托 PlayerManager 移除玩家。被动断开同样进入统一玩家生命周期。

## 上下游外部依赖关系

连接实现来自 network/base，入站委派 handshake/play；玩家资源释放由应用装配到 PlayerManager。

## 容易踩的坑

- PacketSender 只投递包，PlayerConnectionManager 关闭连接，PlayerManager 统一移除记录。
- 玩家移除前释放菜单、实体和区块资源，移除后通知剩余玩家；重复移除不得重复通知。
- 玩家移动与区块订阅分开，订阅统一使用 ChunkSyncManager 持有的 tracker。

- `startAccept(0, ...)` 由系统分配空闲端口，监听日志必须记录绑定后的实际端口。

### 1. 销毁顺序：会话必须先于连接

`ClientSession` 持非拥有 `ServerClientConnection*`，连接所有权在 `ServerNetwork::m_connections`。
销毁顺序必须是**先会话、后连接**：

- 远程会话：`MinecraftServer::_shutdownRemoteSessions()` 先 `m_clientSessionManager.reset()`，
  再 `m_serverNetwork.reset()`。
- 本地会话（`sessionId == 0`）：`IntegratedServer::stop()` 先 `m_clientSession.reset()`，
  再调 `_shutdownRemoteSessions()`。

顺序颠倒会留下悬垂引用。这是本模块唯一的硬性生命周期约束。

### 2. 本地会话由 `IntegratedServer` 自持，不经 `ClientSessionManager`

这是**有意为之**，不是遗留重复：`ClientSessionManager` 由 `_setupRemoteSessions()` 在
LAN 发布（`/publish`）时才创建，单机启动时它根本不存在；而本地会话在单机启动就必须存在。
把本地会话并进该 manager 需要把它改成"始终存在 + 按连接传参（离线模式/压缩阈值/就绪回调/
入站队列开关）"，收益（会话所有权单一化）不足以抵消对单机启动主路径的改动风险。

两条路径的**派发链已经统一**在 `ClientSession::handleInbound` 内（握手状态机 → phase/playerId
守卫 → Play 分发），本地侧只剩构造 + 就绪回调 + 入站监听器装配三段短代码，不存在同一逻辑的
两份实现。改动这两条路径前请先确认收益大于上述风险。

### 3. 构造顺序：ClientSession 必须在 playHandler 之后构造

`ClientSession` 构造时取 `MinecraftServer::playHandler()` 的引用。若早于 `initializeCoreManagers()`，
拿到的是空悬引用，表现为**首个 `AcceptTeleportation` 包触发 ACCESS_VIOLATION**（已踩过一次）。

### 4. 不可移动，只能 unique_ptr

`ClientSession` 内含 `ServerHandshakeStateMachine`，后者有引用成员（不可重绑），因此
`ClientSession` 的拷贝与移动全部 `= delete`。必须经 `unique_ptr` 存入容器，不要"顺手"补移动构造。

### 5. `ServerNetwork::tick()` 里禁止重入

入站 handler 内递归调用 `tick()` / `pumpLocal()` 会造成入站队列的重入死循环。

### 6. accept 线程关闭（Linux 关服卡死）

`_beginAccept` 必须用 `async_accept` 回调链 + `io_context::run()`，不能在专用线程里写同步阻塞
`accept()`——Linux 上 `close()` listen socket 的 fd 不会中断阻塞中的 `::accept(fd)`，导致 join 永久
阻塞、关服卡死（Windows 正常，故极易漏测）。析构时 `m_ioContext->stop()` 是唯一可靠的唤醒手段。

### 7. Wire 入站队列是 Wire-only 的

Local 连接不走 `enqueueInbound`/`drainInbound`（`pumpLocal()` 已在主线程直驱）。给 Local 连接也接队列
只会引入一次多余的 tick 延迟。

### 8. 延迟断开队列

`_notifyDisconnect` 在**网络执行器**被调用，只允许 push `sessionId`，不得触碰连接或 session map。
`tick()` 在主线程 swap 出来后回调 `m_onDisconnect`，连接从 `m_connections` 的移除也在此完成。

客户端偏好必须在建号和重配置完成后应用到已有玩家，调用 MinecraftServer::updateClientInformation；只保存在握手状态机会使 Play 阶段实体与区块追踪继续使用旧设置。

### 网络执行与共享发送

TCP 编码、压缩、加密和异步读写在共享 NetworkIO 上按连接串行执行，与存档 IO 池独立。
accept 回调只投递待接管 socket，会话创建、玩家筛选与游戏处理仍由主线程完成。
Wire 入站队列最多 4096 个包，每连接每 tick 最多派发 512 个；超限关闭连接。
广播通过 sendShared 复用 OutboundPacket 快照和加密前帧，任务不持有玩家或世界引用。
