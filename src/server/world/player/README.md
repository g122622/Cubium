# 服务端玩家模块

本目录包含服务端玩家实体管理相关的功能实现。

## 目录结构

```
player/
├── ServerPlayerEntityManager.hpp   # 服务端玩家实体管理器定义
├── ServerPlayerEntityManager.cpp   # 服务端玩家实体管理器实现
└── README.md                       # 本文档
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      服务端架构                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  MinecraftServer / IntegratedServer                             │
│  ├─ m_playerEntityManager: ServerPlayerEntityManager             │
│  │                                                              │
│  ├─ handleLoginRequest()                                        │
│  │   ├─ m_playerManager.addPlayer(playerId, ...) // 会话管理     │
│  │   └─ m_playerEntityManager.createPlayerEntity(...)           │
│  │       ├─ 创建 ServerPlayer 对象（携带 PlayerAdvancements、   │
│  │       │  末影箱回调等服务端特有状态）                          │
│  │       ├─ 注入服务端上下文：setServer/setWorld/setConnection  │
│  │       │  必须在 spawnEntity 之前完成，确保实体一进入          │
│  │       │  EntityManager 就是完整初始化状态                     │
│  │       ├─ world.spawnEntity() → EntityManager 分配 EntityId   │
│  │       ├─ world.entityTracker().trackEntity()                 │
│  │       └─ 建立 playerId ↔ entityId 映射                        │
│  │                                                              │
│  └─ stop()                                                      │
│      └─ m_playerEntityManager.clearAll()                        │
│                                                                 │
│  ServerWorld                                                    │
│  ├─ EntityManager: 世界实体池（包含玩家实体）                     │
│  └─ EntityTracker: 实体同步系统（追踪玩家实体）                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖的）

| 模块 | 用途 |
|------|------|
| `common/core/Types.hpp` | PlayerId、EntityId 类型定义 |
| `server/player/ServerPlayer.hpp` | ServerPlayer 实体类（携带服务端特有状态） |
| `common/world/entity/EntityManager.hpp` | 实体管理器（分配 EntityId） |
| `server/application/IServer.hpp` | 服务器接口（注入到 ServerPlayer） |
| `common/network/connection/IServerConnection.hpp` | 网络连接（注入到 ServerPlayer） |
| `server/world/ServerWorld.hpp` | 服务端世界（spawnEntity、removeEntity） |
| `server/world/entity/EntityTracker.hpp` | 实体追踪器（trackEntity、untrackEntity） |

### 下游依赖（依赖本模块的）

| 模块 | 用途 |
|------|------|
| `server/application/MinecraftServer` | 持有 ServerPlayerEntityManager |
| `server/application/IntegratedServer` | 继承并实现 playerEntityManager() |
| `server/application/StandaloneServer` | 继承并实现 playerEntityManager() |
| `server/core/PacketHandler` | 通过 IServer 接口访问玩家实体 |
| `server/command/commands/*` | 多个命令通过 IServer 访问玩家实体 |
| `server/advancement/AdvancementEventHandler` | 通过 IServer 访问玩家实体 |
| `server/interaction/BlockInteractionManager` | 通过 IServer 访问玩家实体 |
| `client/world/player/LocalPlayerIdentity` | 客户端对应模块 |

## 容易踩的坑

### 1. 玩家实体创建流程

玩家实体必须通过 `createPlayerEntity()` 创建，不能直接 `new ServerPlayer`。因为创建流程包含：
1. 创建 ServerPlayer 对象（携带 PlayerAdvancements、末影箱回调等服务端特有状态）
2. 设置 PlayerId
3. 设置初始位置
4. 注入服务端上下文（setServer/setWorld/setConnection），必须在 spawnEntity 之前完成，
   使成就触发、网络发包、末影箱自动保存等路径在实体进入 EntityManager 时立即可用
5. 加入世界实体池（EntityManager 分配 EntityId）
6. 加入实体追踪器（EntityTracker 开始同步）
7. 建立双向映射

### 2. 玩家移除必须使用正确的方法

移除玩家时必须调用 `removePlayerEntity()`，确保：
- 从 EntityTracker 移除追踪
- 从 EntityManager 移除实体
- 清除 PlayerId ↔ EntityId 映射

如果直接调用 `world.removeEntity()` 或 `entityManager.removeEntity()`，会导致追踪器和映射残留。

### 3. 实体指针生命周期

`getPlayerEntity()` 返回的指针可能在下次实体操作后失效，调用者应立即使用，不要长期持有。

### 4. PlayerId 与 EntityId 的区别

- **PlayerId**：网络会话标识，由 `PlayerManager` 分配，用于玩家会话管理
- **EntityId**：世界实体标识，由 `EntityManager` 分配，用于实体池管理

两者需要通过 `ServerPlayerEntityManager` 进行双向映射查询。

### 5. 服务端关闭时的清理

服务端关闭时必须调用 `clearAll()` 清理所有玩家实体，否则 EntityTracker 和 EntityManager 中会残留数据。
