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
├── 实体尺寸：width, height, eyeHeight（根据实体类型和姿态计算）
├── 元数据缓存：EntityDataManager, metadata bytes
└── 特殊实体数据：puffState(河豚), axolotlVariant(美西螈), xpValue(经验球), ironGolemAttackTimer/ironGolemArmsRaised/ironGolemHoldingRose(铁傀儡), itemStack(物品实体), fuseTimer(TNT矿车), eatAnimationTimer(羊等吃草动画)
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
   - 各实体类型的同步分支在 `syncMetadataFromDataManager()` 中按 `typeId` 分发：
     - `minecraft:item` → 物品数量
     - `minecraft:polar_bear` → 站立状态
     - `minecraft:pufferfish` → 膨胀状态
     - `minecraft:ocelot` → 信任状态（`isTrusting`）
     - `minecraft:cat` → 躺下/放松状态
     - `minecraft:wolf` → 兴趣状态（`wolfIsInterested`，由 `BegGoal` 驱动）、驯服状态（`wolfTamed`，由 `TameableEntity::DATA_TAMED_PARAM` 同步）、颈圈颜色（`wolfCollarColor`，由 `WolfEntity::DATA_COLLAR_COLOR_PARAM` 同步，默认红色）

8. **狼兴趣状态（乞求食物）动画**：
   - 服务端 `WolfEntity::setInterested` 写入 `DATA_INTERESTED_PARAM`
   - `syncMetadataFromDataManager` 读取后调用 `setWolfIsInterested`
   - `ClientEntity::tick` 推进 `m_wolfInterestedAngle` 向 1.0/0.0 插值（系数 0.4）
   - 渲染时由 `EntityRendererManager` 写入 `AnimationContext::wolfInterestedAngle`
   - 对应 MC 1.21.11 `Wolf.tick()` 第 318-323 行的 `interestedAngle` 插值逻辑

9. **狼驯服状态与颈圈颜色镜像字段**：
   - `wolfTamed()` ← `TameableEntity::DATA_TAMED_PARAM`（bool）：服务端 `setTamed()` 写入，客户端 `syncMetadataFromDataManager` 读取并调用 `setWolfTamed`。`WolfCollarLayer::shouldRender` 据此判断是否渲染项圈，`WolfRenderer::getEntityTexture` 据此选择驯服纹理。
   - `wolfCollarColor()` ← `WolfEntity::DATA_COLLAR_COLOR_PARAM`（i32，`DyeColor` 枚举序数 0-15）：服务端 `setCollarColor()` 写入，客户端读取并调用 `setWolfCollarColor`。默认红色（`DyeColor::Red` = 14）。`WolfCollarLayer::_getCollarColor` 据此选择项圈色调。
   - 这两个字段使 `WolfCollarLayer` 能在 GPU 管线路径下仅通过 `ClientEntity` 完成项圈渲染，无需访问服务端 `WolfEntity`。

10. **铁傀儡状态不走元数据同步**：
    - 铁傀儡的攻击动画和持花状态通过 `EntityStatusPacket` 触发，**不经过** `EntityMetadataPacket` / `syncMetadataFromDataManager()`
    - 客户端在 `onEntityStatus` 回调中直接设置 `ClientEntity` 的 `ironGolemAttackTimer` / `ironGolemArmsRaised` / `ironGolemHoldingRose`
    - `ClientEntity::tick()` 中递减 `ironGolemAttackTimer`
    - 新增铁傀儡动画状态时不要误走 metadata 路径

11. **TNT矿车引信计时器不走元数据同步**：
    - TNT矿车的 `fuseTimer` 通过 `EntityStatusPacket::Status::EatBlock` (status code 10) 触发，**不经过** `EntityMetadataPacket` / `syncMetadataFromDataManager()`
    - 客户端在 `onEntityStatus` 回调中根据 `typeId() == TNT_MINECART` 区分：TNT矿车调用 `setFuseTimer(80)`，羊调用 `setEatAnimationTimer(40)`
    - `ClientEntity::tick()` 中递减 `m_fuseTimer`
    - 与铁傀儡状态同步模式一致：服务端 `broadcastEntityStatus()` → 网络包 → 客户端回调设置字段

12. **眼高计算依赖注册表和姿态**：
    - `ClientEntity::eyeHeight()` 返回实体的眼睛高度，用于旁观者相机定位等场景
    - 实体创建时从 `EntityRegistry` 查找 `EntitySize` 初始化 `width`/`height`/`eyeHeight`
    - 玩家实体根据姿态动态调整：蹲伏=1.27，游泳/鞘翅飞行=0.4，睡眠=0.2，站立=1.62
    - 非玩家幼年个体眼高为站立眼高的一半（与 MC Java 的 `getAgeScale() = 0.5` 一致）
    - 姿态变化（`setSneaking`/`setSwimming`/`setSleeping`/`setChild`）自动触发 `refreshEyeHeight()`
