# network/play - Play 阶段入站处理

处理客户端进入 Play 阶段后发来的所有 C→S 包。上游是 `session/ClientSession::handleInbound`，
它做完握手消费与 phase / playerId 双重守卫后，把未被消费的 Play 包交给本层。

按**包族**拆分为 6 个处理器，由 `ServerPlayHandler` 聚合门面统一分发。每个处理器是
`play/` 子树内的独立对象，持 `MinecraftServer&`（见 `base/PlayHandlerBase`），只认领自己那族包。

## 目录结构

```
src/server/network/play/
├── README.md
├── base/
│   ├── README.md
│   └── PlayHandlerBase.hpp        # 处理器基座：只提供 MinecraftServer&
├── ServerPlayHandler.hpp/cpp      # 聚合门面：24 路 std::visit 分发表 + 对外 updateEntityTrackingForPlayer
├── MovementHandler.hpp/cpp        # 移动 / 载具移动 / 客户端输入 / 传送确认
├── BlockActionHandler.hpp/cpp     # 挖掘 / 物品动作 / 使用物品 / 放置 / 告示牌
├── EntityActionHandler.hpp/cpp    # 实体交互（INTERACT / ATTACK / INTERACT_AT）
├── ChatHandler.hpp/cpp            # 聊天与命令执行
├── PlayerStateHandler.hpp/cpp     # PlayerCommand / 难度 / 配方书 / 进度界面
└── SessionSignalHandler.hpp/cpp   # 心跳 / ping / 阶段重协商确认 / 区块批次反馈
```

## 内部模块关系

```
ClientSession::handleInbound（session/）
        │ 握手已消费则返回；否则 phase / playerId 守卫
        ▼
ServerPlayHandler::route          ← 唯一的 std::holds_alternative 分发表
        ├─ 22 个分支 ──▶ 对应包族处理器（m_movement / m_blockAction / …）
        └─  4 个分支 ──▶ MinecraftServer 纯虚（handleHotbarSelect / handleContainerClick
                          / handleCloseContainer / SetCreativeModeSlot）
```

守卫之所以在 `ClientSession` 而不在本层：phase 守卫需要连接状态、playerId 守卫需要会话状态，
二者都只有会话层持有。

## 上下游外部依赖关系

### 本目录依赖

| 依赖 | 用途 |
|---|---|
| `common/network/ir/packets/play/*` | C→S 包定义（`std::visit` 的变体） |
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
6 个处理器，"某变体无人认领" 也无法集中发现——现有 else 分支的 `unhandled C->S play variant`
告警会被静默化）。漏加分支不会编译报错，只在运行期打一条日志并丢弃。新增 C→S 包时务必同步：
①在 `route()` 加分支；②在对应处理器加实现体。

### 2. 分发表不能改用 `packet.index()`

`std::variant` 的 index 会随 `PlayPackets.hpp` 增删变体而整体错位，用 index 建静态分发表会导致
静默串包。`holds_alternative` 链虽然啰嗦但是类型安全的。

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
