# 客户端实体模块

本目录包含客户端实体相关的功能实现。

## 目录结构

```
entity/
├── ClientEntity.hpp           # 客户端实体类，存储位置插值、动画状态等渲染相关信息
├── ClientEntity.cpp           # 客户端实体类实现
├── ClientEntityManager.hpp    # 客户端实体管理器，管理所有客户端实体的创建、更新、销毁
├── ClientEntityManager.cpp    # 客户端实体管理器实现
└── README.md                  # 本文档
```

## 内部模块关系

```
ClientEntityManager
├── 管理 m_entities: map<EntityId, unique_ptr<ClientEntity>>
├── 特殊处理 m_localPlayerEntityId（本地玩家实体）
└── 调用 ClientEntity 的方法：
    ├── spawnEntity() → new ClientEntity()
    ├── tick() → entity.tick()
    ├── updateInterpolation() → entity.updateInterpolation()
    └── updateAnimations() → entity.updateAnimation()

ClientEntity
├── 位置插值系统：position, targetPosition, interpolationSpeed
├── 旋转插值系统：yaw, pitch, headYaw 及其 target 值
├── 动画状态：limbSwing, swingProgress, hurtTime
├── 实体状态：onGround, sneaking, swimming, riding, sleeping
├── 元数据缓存：EntityDataManager, metadata bytes
└── 特殊实体数据：puffState(河豚), axolotlVariant(美西螈), xpValue(经验球), itemStack(物品实体)
```

## 上下游外部依赖关系

**被谁依赖（上游）：**
- `ClientWorld` - 持有 ClientEntityManager 实例，在 tick 和渲染时调用
- `NetworkClient` - 接收网络包后调用 spawnEntity/setTargetPosition 等方法
- `EntityRenderer` 系列 - 渲染时读取 ClientEntity 的插值位置和动画状态
- `ClientPlayerPredictor` - 与本地玩家实体的预测系统配合

**依赖了谁（下游）：**
- `mc::core::Types` - EntityId, PlayerId 等基础类型
- `mc::entity::EntityDataManager` - 实体元数据管理
- `mc::item::ItemStack` - 物品堆（装备、物品实体）
- `mc::math::Vector3` - 三维向量
- `mc::world::BlockPos` - 方块位置（睡眠位置）

## 容易踩的坑

1. **位置更新必须区分"立即设置"和"目标设置"**：
   - `setPosition()` - 立即设置位置，不插值。用于：实体出生（onSpawnEntity/onSpawnMob）、传送（onEntityTeleport）
   - `setTargetPosition()` - 设置目标位置，触发平滑插值。用于：实体移动（onEntityMove）
   - 用错会导致实体瞬移或漂移

2. **本地玩家由预测器管理**：
   - 本地玩家的位置不应该从网络包直接更新
   - 通过 `isLocalPlayer(entityId)` 判断，本地玩家使用 `ClientPlayerPredictor`
   - `ClientEntityManager` 会跳过本地玩家的网络位置更新

3. **固定 Tick 累加器防止螺旋死亡**：
   - `fixedTick()` 有 `MAX_TICKS_PER_FRAME = 5` 限制
   - 如果帧率过低，会丢弃部分 tick，而不是卡死

4. **渲染时必须使用插值位置**：
   - 不要直接用 `position()`，必须用 `getInterpolatedPosition(partialTick)`
   - `partialTick` 从 `tickAccumulator() / TICK_INTERVAL` 计算

5. **角度环绕处理**：
   - Yaw 角度在 -180 到 180 之间，插值时要选择最短路径
   - 代码中已处理，但如果手动修改 yaw 需要注意

6. **不能移除本地玩家实体**：
   - `removeEntity()` 对本地玩家返回 false
   - 必须先调用 `clearLocalPlayer()` 才能移除

7. **元数据同步**：
   - 接收到 `EntityMetadataPacket` 后，调用 `setMetadata()` 设置原始字节
   - 然后调用 `syncMetadataFromDataManager()` 更新本地状态（如 puffState, axolotlVariant）
