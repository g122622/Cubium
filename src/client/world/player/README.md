# 玩家模块

本目录包含客户端玩家相关的功能实现。

## 目录结构

```
player/
├── LocalPlayerIdentity.hpp    # 本地玩家身份类定义
├── LocalPlayerIdentity.cpp    # 本地玩家身份类实现
├── ClientPlayerPredictor.hpp  # 客户端玩家预测器定义
├── ClientPlayerPredictor.cpp  # 客户端玩家预测器实现
└── README.md                  # 本文档
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

### ClientPlayerPredictor.hpp/cpp

客户端玩家预测器。处理本地玩家的客户端预测：
- **移动预测**: 立即响应用户输入
- **位置校正**: 服务端确认后校正偏差
- **平滑插值**: 避免校正时的跳变

#### 设计原则

客户端预测允许玩家操作获得即时反馈，同时保持与服务端的状态同步。

核心流程：
1. 玩家输入 → 本地预测移动 → 发送输入给服务端
2. 服务端处理 → 返回确认位置
3. 客户端收到确认 → 与预测位置比较 → 校正偏差

#### 使用场景

```cpp
// 处理玩家输入
predictor.handleMovementInput(forward, strafe, jumping, sneaking);

// 每帧更新预测位置
predictor.tick(deltaTime);

// 接收服务端确认
predictor.receiveServerPosition(position, yaw, pitch);

// 获取预测位置用于渲染
Vector3 renderPos = predictor.predictedPosition();
```

#### 关键方法

| 方法 | 说明 |
|------|------|
| `handleMovementInput(forward, strafe, jumping, sneaking)` | 处理移动输入 |
| `handleRotationInput(deltaYaw, deltaPitch)` | 处理旋转输入 |
| `receiveServerPosition(position, yaw, pitch)` | 接收服务端位置确认 |
| `tick(deltaTime)` | 每帧更新 |
| `predictedPosition()` | 获取预测位置 |
| `predictedRotation()` | 获取预测旋转 |
| `reset(position, yaw, pitch)` | 重置预测器（传送/重生） |
| `setMovementSpeed(speed)` | 设置移动速度 |
| `setCorrectionThreshold(threshold)` | 设置校正阈值 |

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
│  ├─ m_predictor: ClientPlayerPredictor                          │
│  │   └─ 客户端预测，平滑校正                                     │
│  │                                                              │
│  ├─ NetworkClient                                               │
│  │   ├─ onLoginSuccess(playerId, entityId, username)            │
│  │   │   └─ 设置 m_localIdentity，初始化 m_predictor            │
│  │   │                                                          │
│  │   └─ onEntityTeleport(entityId, x, y, z, yaw, pitch)         │
│  │       └─ 本地玩家 → m_predictor.receiveServerPosition()       │
│  │                                                              │
│  └─ ClientWorld                                                 │
│      └─ ClientEntityManager                                     │
│          └─ spawnLocalPlayer(entityId, playerId, username)      │
│              └─ 创建本地玩家实体                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 线程安全

- `LocalPlayerIdentity` 类**不是线程安全的**。调用者需要确保在正确的线程访问。
- `ClientPlayerPredictor` 类**不是线程安全的**。调用者需要确保在主线程访问。

## 相关模块

- **ClientEntityManager**: 客户端实体管理，包含本地玩家实体管理
- **NetworkClient**: 网络客户端，接收登录响应并设置身份
- **ServerPlayerEntityManager**: 服务端玩家实体管理（对应模块）

## 注意事项

1. **永远不要将 EntityId 强转为 PlayerId**，这是导致相机绑定到错误实体的根本原因
2. 登录时必须同时设置 `playerId` 和 `entityId`
3. 登出时必须调用 `clear()` 清除身份
4. 判断本地玩家时，使用 `isLocalPlayerEntity(entityId)` 而不是比较 `entityId == playerId`
5. `ClientPlayerPredictor` 需要在登录成功后初始化，在断开连接时重置
6. 传送时应调用 `predictor.reset()` 清除预测状态
