# 客户端实体模块

本目录包含客户端实体相关的功能实现。

## 目录结构

```
entity/
├── ClientEntity.hpp           # 客户端实体类定义
├── ClientEntity.cpp           # 客户端实体类实现
├── ClientEntityManager.hpp    # 客户端实体管理器定义
├── ClientEntityManager.cpp    # 客户端实体管理器实现
└── README.md                  # 本文档
```

## 文件介绍

### ClientEntity.hpp/cpp

客户端实体代理类。存储客户端实体的渲染相关信息，包括位置插值、动画状态等。

#### 核心功能

1. **平滑位置插值**：实体位置通过目标位置平滑插值，避免瞬移
2. **平滑旋转插值**：Yaw、Pitch、HeadYaw 都支持平滑插值，角度环绕正确处理
3. **动画状态跟踪**：LimbSwing 用于行走动画

#### 平滑插值机制

客户端实体的位置和旋转支持平滑插值，避免网络更新造成的瞬移感：

```cpp
// 设置目标位置（触发平滑插值）
entity->setTargetPosition(100.0f, 64.0f, 200.0f);
entity->setTargetRotation(90.0f, 0.0f);
entity->setTargetHeadRotation(45.0f);

// 每 tick 调用，执行插值计算
entity->tickPosition();  // 位置向目标移动一定比例
entity->tickRotation();  // 旋转向目标移动一定比例

// 渲染时使用插值位置
Vector3 renderPos = entity->getInterpolatedPosition(partialTick);
f32 renderYaw = entity->getInterpolatedYaw(partialTick);
```

#### 插值速度

- `setInterpolationSpeed(speed)`：设置插值速度（0.01-1.0）
  - 默认值：0.3（每 tick 移动30%的距离）
  - 1.0：瞬移（禁用插值）
  - 0.01：非常缓慢的插值
- `setSmoothInterpolation(enabled)`：启用/禁用平滑插值

#### 网络回调处理

| 网络包 | 处理方式 |
|--------|----------|
| `onSpawnEntity` / `onSpawnMob` | `setPosition()` - 立即设置，不插值 |
| `onEntityMove` | `setTargetPosition()` - 触发插值 |
| `onEntityTeleport` | `setPosition()` - 传送，立即设置 |
| `onEntityHeadLook` | `setTargetHeadRotation()` - 触发插值 |
| `onPlayerMove`（远程玩家） | `setTargetPosition()` / `setTargetRotation()` |

### ClientEntityManager.hpp/cpp

客户端实体管理器。管理所有客户端实体的创建、更新、销毁。

#### 固定 Tick 系统（20 TPS）

客户端实体管理器实现了固定 tick 累加器，确保实体逻辑以 20 TPS（每秒 20 次）的固定频率运行，与服务器保持同步：

```cpp
class ClientEntityManager {
public:
    // 主循环调用，传入帧时间
    void update(f32 deltaTime);
    
private:
    f32 m_tickAccumulator = 0.0f;
    static constexpr f32 TICK_INTERVAL = 1.0f / 20.0f;  // 0.05秒
    
    void tick();  // 固定频率 tick
};
```

**固定 Tick 流程**：

```
update(deltaTime)
    └─ m_tickAccumulator += deltaTime
    └─ while (m_tickAccumulator >= TICK_INTERVAL)
        └─ tick()
        └─ m_tickAccumulator -= TICK_INTERVAL
```

**好处**：
- 客户端实体逻辑与服务器同步
- 物理计算稳定
- 动画 tick 更新一致

#### 动画包处理

客户端实体管理器处理来自服务器的动画触发包：

| 方法 | 触发源 | 描述 |
|------|--------|------|
| `triggerSwingAnimation(entityId)` | SwingPacket | 触发实体挥手动画 |
| `triggerHurtAnimation(entityId)` | EntityHurtPacket | 触发实体受伤动画 |

```cpp
// 网络包处理
void ClientEntityManager::handleSwingPacket(i32 entityId) {
    ClientEntity* entity = getEntity(entityId);
    if (entity) {
        triggerSwingAnimation(*entity);
    }
}

void ClientEntityManager::triggerSwingAnimation(ClientEntity& entity) {
    // 重置挥手动计时器
    entity.m_swingProgress = 0.0f;
    entity.m_isSwinging = true;
}
```

#### 河豚膨胀状态同步

河豚的膨胀状态通过实体元数据同步：

```cpp
// 在 ClientEntityManager::tick() 中
void ClientEntityManager::syncEntityMetadata(ClientEntity& entity) {
    if (entity.getType() == EntityType::Pufferfish) {
        i32 puffState = entity.getMetadata(DataParameters::PUFF_STATE);
        entity.setPuffState(puffState);
    }
}
```

**元数据参数**：
- `PUFF_STATE` (int): 0 = 未膨胀, 1 = 中等膨胀, 2 = 完全膨胀

#### 本地玩家支持

本地玩家实体也被纳入此管理器，通过 `isLocalPlayer()` 判断：

```cpp
// 创建本地玩家实体
ClientEntity* player = entityManager.spawnLocalPlayer(entityId, playerId, username);

// 判断是否是本地玩家
if (entityManager.isLocalPlayer(entityId)) {
    // 本地玩家使用预测系统
}
```

#### 实体遍历

```cpp
// 遍历所有实体（包括本地玩家）
entityManager.forEachEntity([](ClientEntity& entity) {
    // 处理实体
});

// 只遍历远程实体（不包括本地玩家）
entityManager.forEachRemoteEntity([](ClientEntity& entity) {
    // 处理远程实体
});
```

## 模块关系

```
┌─────────────────────────────────────────────────────────────────┐
│                      客户端架构                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ClientApplication                                              │
│  ├─ m_world: ClientWorld                                        │
│  │   └─ m_entityManager: ClientEntityManager                    │
│  │       ├─ 本地玩家实体（spawnLocalPlayer 创建）               │
│  │       └─ 远程实体列表（spawnEntity 创建）                    │
│  │                                                              │
│  ├─ m_localIdentity: LocalPlayerIdentity                        │
│  │   └─ 维护 playerId ↔ entityId 映射                           │
│  │                                                              │
│  ├─ m_predictor: ClientPlayerPredictor                          │
│  │   └─ 本地玩家预测，平滑校正                                   │
│  │                                                              │
│  └─ NetworkClient                                               │
│      ├─ onSpawnEntity → entityManager.spawnEntity()             │
│      ├─ onEntityMove → entity.setTargetPosition()               │
│      └─ onEntityTeleport → entity.setPosition()                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 平滑插值原理

### 位置插值

每个 tick，实体的当前位置向目标位置移动一定比例：

```cpp
position += (targetPosition - position) * interpolationSpeed;
```

这种方法会产生平滑的减速效果：开始时移动快，接近目标时移动慢。

### 角度环绕处理

Yaw 角度需要特殊处理环绕问题：

```cpp
// 计算差值，选择最短路径
float yawDiff = targetYaw - currentYaw;
// 规范化到 [-180, 180]
while (yawDiff > 180) yawDiff -= 360;
while (yawDiff < -180) yawDiff += 360;
// 应用插值
currentYaw += yawDiff * interpolationSpeed;
// 规范化结果
while (currentYaw > 180) currentYaw -= 360;
while (currentYaw < -180) currentYaw += 360;
```

### 渲染时插值

渲染时使用 `getInterpolatedPosition(partialTick)` 进行帧间插值：

```cpp
Vector3 renderPos = prevPosition + (position - prevPosition) * partialTick;
```

## 线程安全

- `ClientEntity` 类**不是线程安全的**。调用者需要确保在主线程访问。
- `ClientEntityManager` 类**不是线程安全的**。调用者需要确保在主线程访问。

## 相关模块

- **LocalPlayerIdentity**：本地玩家身份管理
- **ClientPlayerPredictor**：本地玩家预测
- **ClientWorld**：客户端世界管理
- **NetworkClient**：网络客户端

## 测试用例

- `tests/client/world/entity/ClientEntityTest.cpp`：ClientEntity 单元测试

## 注意事项

1. **出生时使用立即设置**：`onSpawnEntity`/`onSpawnMob` 使用 `setPosition()`，不插值
2. **移动时使用目标设置**：`onEntityMove` 使用 `setTargetPosition()`，触发平滑插值
3. **传送时使用立即设置**：`onEntityTeleport` 使用 `setPosition()`，不插值
4. **本地玩家使用预测器**：本地玩家的位置由 `ClientPlayerPredictor` 管理
5. **插值速度可调**：通过 `setInterpolationSpeed()` 调整平滑程度
