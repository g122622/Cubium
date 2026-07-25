# 玩家模块

本目录包含客户端玩家相关的功能实现。

## 目录结构树

```text
player/
├── LocalPlayerIdentity.hpp     # 本地玩家身份类（playerId ↔ entityId 映射）
├── LocalPlayerIdentity.cpp     # 本地玩家身份类实现
├── ClientPlayerPredictor.hpp   # 客户端玩家预测器（移动预测、位置校正）
├── ClientPlayerPredictor.cpp   # 客户端玩家预测器实现
└── README.md                   # 本文档
```

## 内部模块关系

```
LocalPlayerIdentity ←→ ClientPlayerPredictor
        ↓                      ↓
   网络回调路由            渲染位置预测
```

- **LocalPlayerIdentity**：维护 `playerId` 和 `entityId` 的映射关系，用于网络回调正确路由玩家相关包。
- **ClientPlayerPredictor**：处理本地玩家的客户端预测，实现即时反馈与服务端同步。

## 上下游外部依赖关系

**上游依赖（本模块使用的）**：
- `mc::common::core::Types` - PlayerId、EntityId 等基础类型
- `mc::common::util::math::Vector3` - 向量类型

**下游依赖（使用本模块的）**：
- `ClientApplication` - 持有 `LocalPlayerIdentity` 和 `ClientPlayerPredictor` 实例
- `ClientNetwork` - 登录成功后设置身份，接收服务端位置确认时调用预测器
- `ClientWorld` / `ClientEntityManager` - 通过身份判断本地玩家实体

**数据流向**：
```
ClientNetwork.onLoginSuccess(playerId, entityId)
    → LocalPlayerIdentity.setIdentity()
    → ClientPlayerPredictor.reset()

ClientNetwork.onEntityTeleport()/onPlayerPosition()
    → LocalPlayerIdentity.isLocalPlayerEntity() 判断
    → ClientPlayerPredictor.receiveServerPosition()（仅本地玩家）

ClientApplication.render()
    → ClientPlayerPredictor.predictedPosition() 获取渲染位置
```

## 容易踩的坑

1. **永远不要将 EntityId 强转为 PlayerId**，这是导致相机绑定到错误实体的根本原因
2. 登录时必须同时设置 `playerId` 和 `entityId`
3. 登出时必须调用 `clear()` 清除身份
4. 判断本地玩家时，使用 `isLocalPlayerEntity(entityId)` 而不是比较 `entityId == playerId`
5. `ClientPlayerPredictor` 需要在登录成功后初始化，在断开连接时重置
6. 传送时应调用 `predictor.reset()` 清除预测状态
7. 这两个类都不是线程安全的，调用者需要确保在主线程访问
