# network/play - Play 阶段入站处理

处理客户端进入 Play 阶段后发来的所有 C→S 包。上游是 `session/ClientSession::handleInbound`，
它做完握手消费与 phase / playerId 双重守卫后，把未被消费的 Play 包交给本层。

按**包族**拆分为 8 个处理器，由 `ServerPlayHandler` 聚合门面统一分发。每个处理器是
`play/` 子树内的独立对象，持 `MinecraftServer&`（见 `base/PlayHandlerBase`），只认领自己那族包。

`ConfigurationAcknowledged` 是例外：它是重配置请求的响应，需访问该连接的握手状态机，故在
`ClientSession::handleInbound` 进入 Play 路由**之前**被就地拦截（见 ClientSession），不会到达本层。

**空闲踢出时间戳**：`route()` 在分发前对「玩家主动动作」包集合（`isPlayerActionPacket`）调
`MinecraftServer::recordPlayerAction`，对齐 vanilla 各 C→S 处理器里的 `player.resetLastActionTime()`。
集合**不含**自动回包（`keep_alive`/`pong`/`ping_request`）——否则挂机客户端会被心跳续命、永不超时；
移动包另由 `MovementHandler` 按「位移显著才推进」处理（对齐 vanilla `handlePlayerKnownMovement`）。

## 目录结构

```
src/server/network/play/
├── README.md
├── base/
│   ├── README.md
│   └── PlayHandlerBase.hpp        # 处理器基座：只提供 MinecraftServer&
├── ServerPlayHandler.hpp/cpp      # 聚合门面：按类型的 switch 分发表 + 对外 updateEntityTrackingForPlayer
├── MovementHandler.hpp/cpp        # 移动 / 载具移动 / 客户端输入 / 传送确认 / 客户端 tick 结束
├── BlockActionHandler.hpp/cpp     # 挖掘 / 物品动作 / 使用物品 / 放置 / 告示牌
├── EntityActionHandler.hpp/cpp    # 实体交互（INTERACT / ATTACK / INTERACT_AT）
├── ContainerActionHandler.hpp/cpp # 物品编辑与菜单专用操作（有效菜单、真实玩家校验）
├── ChatHandler.hpp/cpp            # 聊天与命令执行
├── PlayerStateHandler.hpp/cpp     # PlayerCommand / 游戏模式 / 难度 / 配方书 / 进度界面
└── SessionSignalHandler.hpp/cpp   # 心跳 / ping / 阶段重协商确认 / 区块批次反馈
```

## 内部模块关系

```
ClientSession::handleInbound（session/）
        │ 握手已消费则返回；否则 phase / playerId 守卫
        ▼
ServerPlayHandler::route          ← 唯一的 switch 分发表
        ├─ 23 个分支 ──▶ 对应包族处理器（m_movement / m_blockAction / …）
        └─  4 个分支 ──▶ MinecraftServer 纯虚（handleHotbarSelect / handleContainerClick
                          / handleCloseContainer / SetCreativeModeSlot）
```

守卫之所以在 `ClientSession` 而不在本层：phase 守卫需要连接状态、playerId 守卫需要会话状态，
二者都只有会话层持有。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/ir/packets/play/*` | C→S 包定义（类型驱动 switch 分发） |
| `common/entity/*`、`common/item/*`、`common/world/*` | 各处理体的实体/物品/世界操作 |
| `common/advancement/*` | `ChatHandler` 之外的成就触发（方块使用 / 实体交互） |
| `server/application/MinecraftServer.hpp` | 各处理体的业务入口与 4 个纯虚 |
| `server/core/*`、`server/interaction/*`、`server/player/ServerPlayer.hpp` | 玩家/挖掘/方块交互 |
| `server/command/*` | `ChatHandler` 的命令执行 |
| `server/network/outbound/*` | 部分处理体经其构造回包 |

### 依赖本目录

| 模块 | 用途 |
|---|---|
| `session/ClientSession` | 引门面并在守卫后调用 `route` |
| `server/application/MinecraftServer` | 持 `ServerPlayHandler` 门面；登录序列与维度切换调 `updateEntityTrackingForPlayer` |

## 容易踩的坑

### 1. 新增 C→S 包必须同时改两处

`ServerPlayHandler::route()` 是**唯一**的分发表，刻意不下沉到各处理器（否则每个包要依次问遍
各个处理器，"某变体无人认领" 也无法集中发现——现有 else 分支的 `unhandled C->S play variant`
告警会被静默化）。漏加分支不会编译报错，只在运行期打一条日志并丢弃。新增 C→S 包时务必同步：
①在 `route()` 加分支；②在对应处理器加实现体。

### 2. switch 分支标签必须由包类型推导

route 使用 PlayPacket.index() 作为 switch 判别值；每个 case 均通过
_playPacketIndex<包类型>() 在编译期取得下标，禁止手写数字。
新增或调整变体顺序后标签自动跟随类型；类型缺失或重复会触发编译错误。
同族移动包共用一个处理分支，其他包各自显式 break，避免落入下一分支。

### 3. 纯虚分发不能下沉

`handleHotbarSelect` / `handleContainerClick` / `handleCloseContainer` /
`handleOpenPlayerInventoryPacket` 由 `MinecraftServer` 声明、`IntegratedServer`/
`StandaloneServer` override，且**不在** `IServer` 上。因此 `route()` 里这四个分支必须保持
`m_server.handleXxxPacket(...)` 的虚分发写法，不能改成调用某个处理器——改了会直接编译失败。

### 4. 包族的归属由调用关系决定，不要按语义硬分

两个容易分错的点：`handlePlayerItemAction`（丢弃/副手交换/释放使用）虽属"物品"语义，但它由
`handleBlockInteractionPacket` 的 PlayerAction 分支调用，故与方块动作同族；`_triggerAnyBlockUse`
由方块放置/使用触发，同理留在 `BlockActionHandler`。`updateEntityTrackingForPlayer` 由移动与
传送确认调用，实现落在 `MovementHandler`，门面只做转发。拆分/新增处理器前先核对调用图。

客户端设置经 MinecraftServer::updateClientInformation 应用；配置阶段与 Play 阶段共用相同负载校验和连接状态。请求视距与服务器上限取较小值，区块追踪变化负责发送与卸载。

移动速度阈值默认生效。`--stress_skip_movement_speed_check=true` 仅供显式压测运行跳过玩家的
`moved-too-quickly` 阈值，其他移动检查不变；压测报告通过选项和 `-unchecked` 用例名标明豁免状态。
