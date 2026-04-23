# 服务端玩家模块

本目录包含服务端玩家实体管理相关的功能实现。

## 目录结构

```
player/
├── ServerPlayerEntityManager.hpp   # 服务端玩家实体管理器定义
├── ServerPlayerEntityManager.cpp   # 服务端玩家实体管理器实现
└── README.md                       # 本文档
```

## 文件介绍

### ServerPlayerEntityManager.hpp/cpp

服务端玩家实体管理器。负责创建和管理服务端玩家实体，整合：
- **实体创建**: 创建 Player 对象
- **世界实体池管理**: 将玩家加入 EntityManager
- **实体追踪**: 将玩家加入 EntityTracker
- **PlayerId ↔ EntityId 双向映射**: 维护两种标识符的关联

#### 设计原则

- **PlayerId**: 网络会话标识，由 `PlayerManager` 分配
- **EntityId**: 世界实体标识，由 `EntityManager` 分配
- 玩家实体被纳入世界实体池，与其他实体统一管理
- `EntityTracker` 追踪玩家实体，自动同步给其他玩家

#### 关键方法

| 方法 | 说明 |
|------|------|
| `createPlayerEntity(playerId, username, world, x, y, z)` | 创建玩家实体并加入世界 |
| `removePlayerEntity(playerId, world)` | 移除玩家实体 |
| `clearAll(world)` | 清除所有玩家实体（服务器关闭时调用） |
| `getPlayerEntityId(playerId)` | 获取玩家的 EntityId |
| `getPlayerIdByEntityId(entityId)` | 通过 EntityId 获取 PlayerId |
| `getPlayerEntity(playerId, world)` | 获取玩家的实体指针 |
| `hasPlayer(playerId)` | 检查玩家是否存在 |
| `playerCount()` | 获取玩家数量 |

#### 线程安全

所有公共方法都是线程安全的，使用内部 `std::mutex` 保护。

## 模块关系

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
│  │       ├─ 创建 Player 对象                                     │
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

## 登录流程

```
1. 玩家连接 → PlayerManager 分配 PlayerId
2. 创建 ServerPlayerData（会话信息）
3. ServerPlayerEntityManager.createPlayerEntity()
   ├─ 创建 Player 对象（EntityId 暂时为 0）
   ├─ 设置 PlayerId
   ├─ 设置初始位置
   ├─ world.spawnEntity() → EntityManager 分配 EntityId
   ├─ world.entityTracker().trackEntity() → 开始同步
   └─ 建立映射
4. 发送 LoginResponsePacket（包含 playerId 和 entityId）
```

## 相关模块

- **PlayerManager** (`server/core/`): 网络会话管理，分配 PlayerId
- **EntityManager** (`common/world/entity/`): 世界实体池管理
- **EntityTracker** (`server/world/entity/`): 实体同步系统
- **LocalPlayerIdentity** (`client/world/player/`): 客户端对应模块

## 注意事项

1. 玩家实体必须通过 `createPlayerEntity()` 创建，不能直接创建
2. 移除玩家时必须调用 `removePlayerEntity()`，确保清理追踪器和映射
3. 服务端关闭时必须调用 `clearAll()` 清理所有玩家实体
4. `getPlayerEntity()` 返回的指针可能在下次实体操作后失效，调用者应立即使用
