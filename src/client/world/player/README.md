# 玩家模块

本目录包含客户端玩家相关的功能实现。

## 目录结构

```
player/
├── LocalPlayerIdentity.hpp   # 本地玩家身份类定义
├── LocalPlayerIdentity.cpp   # 本地玩家身份类实现
└── README.md                 # 本文档
```

## 文件介绍

### LocalPlayerIdentity.hpp/cpp

本地玩家身份信息管理类。维护 `playerId` 和 `entityId` 的映射关系，用于网络回调正确路由玩家相关包。

#### 设计原则

- **PlayerId**: 网络会话标识，用于认证、权限、网络路由。由服务端的 `PlayerManager` 分配。
- **EntityId**: 世界实体标识，由 `EntityManager` 分配。用于实体系统内部。

这两个标识符是**独立的**，不能互换或强转。

#### 使用场景

1. **登录成功后**，设置本地玩家身份：
   ```cpp
   m_localIdentity.setIdentity(playerId, entityId);
   ```

2. **网络回调中**，判断是否是本地玩家：
   ```cpp
   if (m_localIdentity.isLocalPlayerEntity(entityId)) {
       // 本地玩家，交给预测系统处理
   }
   ```

3. **登出时**，清除身份：
   ```cpp
   m_localIdentity.clear();
   ```

#### 关键方法

| 方法 | 说明 |
|------|------|
| `setIdentity(PlayerId, EntityId)` | 设置本地玩家身份 |
| `clear()` | 清除身份信息 |
| `hasIdentity()` | 检查是否已设置身份 |
| `playerId()` | 获取玩家ID |
| `entityId()` | 获取实体ID |
| `isLocalPlayerEntity(EntityId)` | 检查是否是本地玩家的实体ID |
| `isLocalPlayer(PlayerId)` | 检查是否是本地玩家的玩家ID |

## 模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      客户端架构                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ClientApplication                                              │
│  ├─ m_localIdentity: LocalPlayerIdentity                        │
│  │   └─ 维护 playerId ↔ entityId 映射                           │
│  │                                                              │
│  ├─ NetworkClient                                               │
│  │   └─ onLoginSuccess(playerId, entityId, username)            │
│  │       └─ 设置 m_localIdentity                                │
│  │                                                              │
│  └─ ClientWorld                                                 │
│      └─ ClientEntityManager                                     │
│          └─ spawnLocalPlayer(entityId, playerId, username)      │
│              └─ 创建本地玩家实体                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 线程安全

`LocalPlayerIdentity` 类**不是线程安全的**。调用者需要确保在正确的线程访问。在网络回调（通常是网络线程）和游戏主循环之间共享时，需要适当的同步机制。

## 相关模块

- **ClientEntityManager**: 客户端实体管理，包含本地玩家实体管理
- **NetworkClient**: 网络客户端，接收登录响应并设置身份
- **ServerPlayerEntityManager**: 服务端玩家实体管理（对应模块）

## 注意事项

1. **永远不要将 EntityId 强转为 PlayerId**，这是导致相机绑定到错误实体的根本原因
2. 登录时必须同时设置 `playerId` 和 `entityId`
3. 登出时必须调用 `clear()` 清除身份
4. 判断本地玩家时，使用 `isLocalPlayerEntity(entityId)` 而不是比较 `entityId == playerId`
