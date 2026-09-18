# Server Network 模块

服务端网络子系统：承载单个客户端的 Handshake → Status → Login → Configuration → Play 全流程，
并把世界数据下推给客户端。对外门面只有 `session/ServerNetwork`，其余类型不对外暴露
（`base/` 的两种连接类型除外——`server/core` 与 `ServerPlayer` 需要它们）。

目录内按**连接生命周期阶段**分层（`base` → `session` → `handshake` → `play`），另加两条正交轴：
`outbound/`（服务端主动发起的出站构造）与 `sync/`（世界数据下推）。层名与 `common/network/` 的
`buffer`/`codec`/`ir`/`pipeline`/`transport` 刻意不重名——那些层是本模块**依赖**的实现，不在服务端。

## 目录结构

```
src/server/network/
├── README.md
├── base/                              # 依赖链末梢：连接抽象与连接实例
│   ├── IServerClientConnection.hpp    # 业务侧最小连接视图（send/close/isConnected/peerAddress/disconnect）
│   ├── ServerClientConnection.hpp     # 单连接实现（Local/Wire 双模）+ HandshakeState 枚举
│   └── ServerClientConnection.cpp
├── session/                           # 连接与会话生命周期
│   ├── ServerNetwork.hpp/cpp          # 门面：TCP accept + 连接集合 + tick 泵
│   ├── ClientSession.hpp/cpp          # 单客户端会话：协议状态 + 入站派发链（本地与远程共用）
│   └── ClientSessionManager.hpp/cpp   # 远程会话登记/清理（connect → ready → disconnect）
├── handshake/                         # 连接建立全过程
│   ├── ServerHandshake.hpp/cpp        # Handshake/Status/Login/Configuration 四阶段状态机
│   ├── RegistryDataBuilder.hpp/cpp    # Configuration 阶段 registry/tags/knownPacks 载荷
│   ├── EnchantmentNbtBuilder.hpp/cpp  # datapack 附魔 JSON → 内联 NBT RegistryEntry
│   └── LoginFlow.hpp/cpp              # 进入 Play 的入场序列（建号 + 初始状态推送整簇）
├── play/                              # Play 阶段入站处理（按包族拆分）
│   ├── base/PlayHandlerBase.hpp       # 处理器基座（只提供 MinecraftServer&）
│   ├── ServerPlayHandler.hpp/cpp      # 聚合门面：24 路 std::visit 分发表
│   ├── MovementHandler.hpp/cpp        # 移动 / 载具输入 / 传送确认
│   ├── BlockActionHandler.hpp/cpp     # 挖掘 / 物品动作 / 放置 / 使用物品 / 告示牌
│   ├── EntityActionHandler.hpp/cpp    # 实体交互（INTERACT / ATTACK / INTERACT_AT）
│   ├── ChatHandler.hpp/cpp            # 聊天与命令执行
│   ├── PlayerStateHandler.hpp/cpp     # PlayerCommand / 难度 / 配方书 / 进度界面
│   └── SessionSignalHandler.hpp/cpp   # 心跳 / ping / 配置确认 / 区块批次反馈
├── outbound/                          # 出站：IR 构造、广播、复合下发序列
│   ├── PacketBuilders.hpp/cpp         # 纯自由函数 IR 构造（零 MinecraftServer 依赖）
│   ├── CommandTreeEncoder.hpp         # 命令树 → ClientboundCommandsPacket 包体
│   ├── PlayerBroadcaster.hpp/cpp      # 距离过滤 + 多态广播
│   └── MapPacketBuilder.hpp/cpp       # 脏地图周期推送
└── sync/                              # 世界数据下推（客户端可见状态的同步服务）
    ├── ChunkSendManager.hpp/cpp       # 区块发送 / 卸载通知
    ├── BlockUpdateSyncManager.hpp/cpp # 方块变化同 tick 去重后统一 flush
    ├── WeatherSyncService.hpp/cpp     # 天气影子状态比对 + 广播
    └── chunk/                         # 区块推送记账
        ├── ChunkView.hpp              # 正方形视距范围
        ├── PlayerChunkTracker.hpp/cpp # 单玩家已下发区块
        └── ChunkSyncManager.hpp/cpp   # 玩家 ↔ 区块双向索引
```

## 内部模块关系

```
┌───────────────────────────────────────────────────────────────────┐
│  StandaloneServer / IntegratedServer / MinecraftServer            │
└───────────────┬───────────────────────────────────────────────────┘
                │ 持有
                ▼
┌───────────────────────────────────────────────────────────────────┐
│ session/ServerNetwork       门面：accept + 连接集合 + tick 泵       │
│   owns ──▶ base/ServerClientConnection（每连接，Local/Wire 双模）   │
└───────────────┬───────────────────────────────────────────────────┘
                │ onConnect / onDisconnect 回调
                ▼
┌───────────────────────────────────────────────────────────────────┐
│ session/ClientSessionManager   远程 TCP 会话簿记                    │
│   owns ──▶ session/ClientSession                                   │
│              ├─ ServerHandshakeStateMachine（handshake/）          │
│              └─ ServerPlayHandler&（play/，单例门面）               │
│   handleInbound：握手状态机 → phase/playerId 守卫 → Play 分发        │
└───────────────┬───────────────────────────────────────────────────┘
                ▼
┌───────────────────────────────┐   ┌───────────────────────────────┐
│ handshake/ServerHandshake     │   │ play/ServerPlayHandler        │
│  Handshake→Status→Login→Conf  │──▶│   24 路 std::visit 分发        │
│  Configuration 载荷来自 ↓      │   │   + 各 handle*Packet 处理体    │
│  handshake/RegistryDataBuilder│   └───────────────────────────────┘
│   └─ EnchantmentNbtBuilder    │
│  Play 入场交 ↓                 │
│  handshake/LoginFlow          │
└───────────────────────────────┘
                │ 出站统一经 ↓
                ▼
┌───────────────────────────────────────────────────────────────────┐
│ outbound/  PacketBuilders（纯构造）/ PlayerBroadcaster（过滤广播）  │
│            MapPacketBuilder / CommandTreeEncoder                  │
└───────────────────────────────────────────────────────────────────┘
┌───────────────────────────────────────────────────────────────────┐
│ sync/  ChunkSendManager / BlockUpdateSyncManager / WeatherSyncService
│          └─ chunk/  ChunkSyncManager ─ PlayerChunkTracker ─ ChunkView
└───────────────────────────────────────────────────────────────────┘
    （sync/ 由 ServerDimension 持有，经回调把数据交给 outbound/ 下发）
```

**数据流**

- **入站（Local）**：客户端 `Connection::send(ir)` → `LocalTransport` 直传 → `ServerClientConnection::onPacket`
  → `ServerNetwork::tick()` 内 `pumpLocal()` 主线程派发 → `ClientSession::handleInbound`
- **入站（Wire）**：客户端字节 → `TcpTransport`（接收线程）→ `pipeline::Connection` 解帧/解压/解密 →
  `enqueueInbound`（接收线程入队）→ `drainInbound`（主线程）→ 同上
- **出站**：上层 → `ServerClientConnection::send(ir::IrPacket)` → `pipeline::Connection::send`
  → Local 直传 / Wire 编码+压缩+加密 → transport 发送

## 上下游外部依赖关系

### 本模块依赖的外部模块

| 依赖 | 用途 |
|---|---|
| `common/network/{ir,pipeline,transport,buffer,crypto,protocol}` | IR 包、连接管线、Local/TCP 传输、加密与协议元数据 |
| `common/network/sync/ChunkSerializer.hpp` | 区块二进制序列化（双向共用，故留在 common） |
| `common/network/backend/java/*` | Java 协议表、wire codec、`JavaLoginHandshaker` |
| `common/command/*` | 命令树快照（`CommandTreeEncoder` 的输入） |
| `server/application/MinecraftServer.hpp` | `LoginFlow`/`PlayerBroadcaster`/`ServerPlayHandler` 的处理体入口 |
| `server/core/*` | `PlayerManager`、`ConnectionManager`、区块推送记账的宿主 |
| `server/world/ServerWorld.hpp` | 区块加载事件、方块变化事件、地图脏数据 |

### 依赖本模块的外部模块

| 模块 | 用途 |
|---|---|
| `server/application/{StandaloneServer,IntegratedServer}` | 建 `ServerNetwork`、`startAccept` / `publishToLan` |
| `server/application/MinecraftServer` | 持 `ServerNetwork`、`ClientSessionManager`、`ServerPlayHandler` 门面 |
| `server/core/{PlayerManager,ServerPlayerData}` | 经 `IServerClientConnection*` 发包；持 `ChunkSyncManager`/`PlayerChunkTracker` |
| `server/dimension/ServerDimension` | 持 `sync/` 各管理器 |
| `server/command/commands/*` | 经 `outbound/PacketBuilders` 构造命令回包 |
| `server/player/ServerPlayer` | 持 `ServerClientConnection*` |

## 容易踩的坑

### 1. Wire 入站必须走队列 + 主线程 drain

`TcpTransport::_receiveLoop` 在**接收线程**同步触发 `onPacket` 监听器。若直接在监听器里跑游戏逻辑，
会在接收线程触碰非线程安全的 `MinecraftServer` 世界状态。Wire 连接的监听器只允许 `enqueueInbound`
（锁内 push），由 `ServerNetwork::tick()` 在主线程 `drainInbound` 派发。Local 模式不经队列，
`pumpLocal()` 直接主线程派发——队列是 Wire-only 的关注点。

### 2. 断开检测跨线程

`TcpTransport::onDisconnect` 在接收线程触发。跨线程直接改 session map 会与主线程 tick 竞争。
`ServerNetwork::_notifyDisconnect` 在接收线程只把 `sessionId` 推入延迟队列；`tick()` 末尾在主线程
swap 出来逐 sid 回调 `m_onDisconnect`。所有 session map 变动与玩家清理都必须在主线程。

### 3. 销毁顺序：会话必须先于连接

`ClientSession` 持**非拥有**的 `ServerClientConnection*`，其所有权归 `ServerNetwork::m_connections`。
子类 `stop()` 中必须先清空 `ClientSessionManager`，再 `m_serverNetwork.reset()`，否则悬垂。
`ClientSession` 因 `ServerHandshakeStateMachine` 含引用成员而删除了移动语义，只能经 `unique_ptr`
存入容器——不要为了"更优雅"给它加移动构造。

### 4. 构造顺序：ClientSession 必须在 playHandler 之后构造

`ClientSession` 构造时要取 `MinecraftServer::playHandler()` 的引用。若在 `initializeCoreManagers()`
之前构造，会拿到空悬引用，表现为首个 `AcceptTeleportation` 包触发 ACCESS_VIOLATION（已踩过一次）。

### 5. accept 线程关闭（Linux 关服卡死）

不能用同步阻塞 `accept()`：Linux 上 `close()` 一个 listen socket 的 fd **不会**中断正阻塞在
`::accept(fd)` 的线程，会导致 join 永久阻塞。`_beginAccept` 用 `async_accept` 回调链 + `io_context::run()`
驱动，析构时 `m_ioContext->stop()` 可靠唤醒。新增连接的 accept 必须沿用异步链。

### 6. `TcpTransport.hpp` 是全项目唯一拖入 `<asio.hpp>` 的传输头

`base/ServerClientConnection.hpp` 因此只**前置声明** `TcpTransport`（构造签名里出现
`unique_ptr<TcpTransport>` 无需完整类型），真正需要完整类型的 `base/ServerClientConnection.cpp`
与 `session/ServerNetwork.cpp` 自行 include。改回 `#include` 会让 `LoginFlow.hpp`/`ServerHandshake.hpp`/
`ServerPlayer.hpp` 等仅需连接类型的头全部被迫拉入 asio。

### 7. 包大小限制

`TcpTransport`/`pipeline` 走 VarInt21 帧化 + zlib 压缩（threshold=256）。解压后有最大包大小上限，
超过判非法断开。

### 8. `Disconnect` 的 reason 必须是纯文本

codec 会把 reason 编码为 NBT `StringTag`（vanilla `Component.literal(text)` 的纯文本折叠路径）。
误传 JSON 字符串走 `writeString`，客户端按 NBT 解码时会把首字节当 tag id，报 `Invalid tag id`。

### 9. RegistryData 的 NBT 是刻意留白

`RegistryDataBuilder` 目前对所有条目发 `RegistryEntry{id, data=nullopt}`（声明"客户端已知"），
仅在我方互通双方均硬编码 vanilla registry 且 `SelectKnownPacks{minecraft:core}` 命中时合法。
真 Java 互通时双方各自用本地 registry，NBT 消费路径在 core 命中前提下永不触发。卡在 Configuration
挂死时先查此处的 `SelectKnownPacks` 是否命中 core。

### 10. 方块更新不要直接发包

`sync/BlockUpdateSyncManager` 负责同坐标去重与 tick 末统一 flush。在 `ServerWorld`、`IntegratedServer`
或 `StandaloneServer` 中直接发 `ir::play::BlockUpdate` 会绕过去重，产生重复包与竞态。
只让 `ServerWorld::setOnBlockChanged()` 产出事件。

### 11. `ChunkView::getChunksInView()` 返回正方形

视距 n 表示以中心为原点、半径 n 的**正方形**区域，区块数量为 `(2n+1)²`，不是圆形。

### 12. `sync/chunk/` 只管记账，不含序列化

区块推送记账（谁已收到哪个区块）在本目录；区块**二进制序列化**在
`common/network/sync/ChunkSerializer`——它被客户端 `ClientWorld` 的 `deserializeChunk` 双向使用，
所以留在 common 而非迁入 server。

### 13. Windows 需链接 `ws2_32`

asio 在 `io_context` 运行时自动管理 Winsock 初始化，无需手动 `WSAStartup`；但 `ws2_32` 链接库仍需在
`src/server/CMakeLists.txt` 保留。

### 14. CMake 登记要三处同步

`src/server/CMakeLists.txt`、`src/client/CMakeLists.txt`（客户端 target 直接编译约 170 个 server 源文件）、
`tests/CMakeLists.txt` 各自列举同一批源文件。新增/移动 `.cpp` 必须三处同步；漏登记在 client 上的表现是
**链接期 undefined symbol**，而非 configure 失败，容易被误判为"代码写错了"。
