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
├── 特殊实体数据：puffState(河豚), axolotlVariant(美西螈), xpValue(经验球), ironGolemAttackTimer/ironGolemArmsRaised/ironGolemHoldingRose(铁傀儡), itemStack(物品实体), fuseTimer(TNT矿车), eatAnimationTimer(羊等吃草动画), witherSideHeadYaw/Pitch[2]+m_witherHeadTargetId[3](凋灵侧头朝向), fishingHookedEntityId/fishingBiting(钓鱼浮标)
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
     - `minecraft:skeleton` / `minecraft:stray` / `minecraft:bogged` → 拉弓状态（`isChargingBow`，由 `AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM` 同步，驱动 `SkeletonModel` 的 `BowAndArrow` 姿态）
     - `minecraft:wither` / `wither` → 凋灵侧头目标实体 ID（`m_witherHeadTargetId[3]`，由 `WitherEntity::HEAD_TARGET_1/2/3` 同步，驱动 `tickWitherSideHeads` 客户端镜像计算）
     - `minecraft:fishing_bobber` / `fishing_bobber` → 被钩住实体 ID（`m_fishingHookedEntityId`，由 `FishingBobberEntity::DATA_HOOKED_ENTITY_PARAM` 同步，+1 偏移 0=无）+ 咬钩状态（`m_fishingBiting`，由 `DATA_BITING_PARAM` 同步），驱动 `FishingBobberRenderer` 浮标下沉与钓线绷紧

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
    - 客户端在 `onEntityStatus` 回调中根据 `entityType() == VanillaEntityTypeKeys::TNT_MINECART` 区分：TNT矿车调用 `setFuseTimer(80)`，羊调用 `setEatAnimationTimer(40)`
    - `ClientEntity::tick()` 中递减 `m_fuseTimer`
    - 与铁傀儡状态同步模式一致：服务端 `broadcastEntityStatus()` → 网络包 → 客户端回调设置字段

12. **眼高计算依赖注册表和姿态**：
    - `ClientEntity::eyeHeight()` 返回实体的眼睛高度，用于旁观者相机定位等场景
    - 实体创建时从 `EntityRegistry` 查找 `EntitySize` 初始化 `width`/`height`/`eyeHeight`
    - 玩家实体根据姿态动态调整：蹲伏=1.27，游泳/鞘翅飞行=0.4，睡眠=0.2，站立=1.62
    - 非玩家幼年个体眼高为站立眼高的一半（与 MC Java 的 `getAgeScale() = 0.5` 一致）
    - 姿态变化（`setSneaking`/`setSwimming`/`setSleeping`/`setChild`）自动触发 `refreshEyeHeight()`

13. **兔子跳跃动画客户端状态**（参考 MC 1.21.11 `Rabbit.handleEntityEvent(byte 1)`）：
    - **字段**：`m_rabbitJumpTicks`（当前 tick）、`m_rabbitJumpDuration`（总 tick，0=未在跳跃中）。
    - **启动**：`ClientApplicationNetwork::onEntityStatus` 收到 `RabbitJump(1)` 状态码时调用 `setRabbitJumpStart()`，设置 `m_rabbitJumpDuration=10`、`m_rabbitJumpTicks=0`。
    - **推进**：`tick()` 末尾调用 `tickRabbitJump()`，对应 MC `Rabbit.aiStep()` 的跳跃推进逻辑——`m_rabbitJumpTicks != m_rabbitJumpDuration` 时递增，相等且 `m_rabbitJumpDuration != 0` 时归零。
    - **读取**：`rabbitJumpCompletion(partialTick)` 供 `EntityRendererManager` 计算 `jumpRotation = sin(completion * PI)`，最终通过 `RabbitModel::setJumpRotation` 影响 thigh/foot/arm 旋转角度。
    - **数据流**：服务端 `RabbitEntity::startJumping()` 广播 `RabbitJump(1)` → 网络包 → `onEntityStatus` 回调 → `setRabbitJumpStart()` → `tick()` 推进 → `rabbitJumpCompletion()` → 渲染器 → `RabbitModel`。与狼甩水动画（`ShakeOffWater(8)`）使用相同的"状态包触发 → 客户端镜像状态 → tick 推进 → 渲染器读取"模式。

14. **凋灵侧头独立朝向客户端镜像**（参考 MC 1.21.11 `WitherBoss.aiStep()` 中 `j=0..1` 循环）：
    - **背景**：`WitherEntity::aiStep()` 不在客户端运行（`ClientEntity` 是独立代理类，不继承 `Entity`/`WitherEntity`），但凋灵两侧头需要独立追踪 `HEAD_TARGET_2`/`HEAD_TARGET_3` 目标实体。因此客户端必须独立镜像 MC 的侧头朝向计算。
    - **字段**：
      - `m_witherHeadTargetId[3]`：三个头的目标实体 ID（index 0=主头 `HEAD_TARGET_1`，1=左头 `HEAD_TARGET_2`，2=右头 `HEAD_TARGET_3`）。
      - `m_witherSideHeadYaw[2]` / `m_witherSideHeadPitch[2]`：侧头当前 tick 的偏航/俯仰角（度，不包装到 `[-180,180)`），index 0=左头、1=右头。
      - `m_prevWitherSideHeadYaw[2]` / `m_prevWitherSideHeadPitch[2]`：上一 tick 的值，供渲染插值。
    - **元数据同步**：`syncMetadataFromDataManager()` 中 `typeId == "minecraft:wither" || "wither"` 分支读取 `WitherEntity::getHeadTarget1/2/3ParamId()` 对应的 `DataParameter<i32>`，写入 `m_witherHeadTargetId[0/1/2]`。侧头角度本身**不**网络同步——只有目标实体 ID 同步。
    - **推进**：`ClientEntityManager::tick()` 对 `typeId == "minecraft:wither" || "wither"` 的实体调用 `tickWitherSideHeads(entityLookup)`，其中 `entityLookup` 回调为 `[this](EntityId id) -> const ClientEntity* { return this->getEntity(id); }`。方法内部：
      1. 备份 prev 值（对应 MC `aiStep()` 中 `super.aiStep()` 之后的 `yRotOHeads[i]=yRotHeads[i]`）
      2. `j=0..1` 循环：读 `m_witherHeadTargetId[j+1]`，通过回调查找目标 `ClientEntity`
         - 有目标：计算头部位置（`bodyRot + 180*j` 角度偏移，1.3 格水平偏移，2.2 格 Y 偏移），`dx/dy/dz` → `atan2` → `targetYaw/Pitch`，`math::clampedRotate` 逼近（yaw 限速 10°/tick，pitch 限速 40°/tick）
         - 无目标（`targetId<=0` 或回调返回 `nullptr` 或回调为空）：yaw 朝 `bodyRot`（`m_yaw`）逼近 10°/tick，pitch 不变
    - **读取**：`getInterpolatedWitherSideHeadYaw/Pitch(index, partialTick)` 供 `EntityRendererManager::_createModelForEntity` 调用。渲染器通过 `math::wrapDegrees(absoluteYaw - bodyYaw)` 将绝对 yaw 转为 body 相对值（对齐 MC `WitherBossModel.setupHeadRotation` 中 `yHeadRots[index] - bodyRot`），再调用 `WitherModel::setSideHeadRotations(yaw0, pitch0, yaw1, pitch1)` 注入。
    - **数据流**：服务端 `WitherEntity::aiStep` → `_updateSideHeadRotations`（服务端权威计算）→ `HEAD_TARGET_1/2/3` 通过 `EntityMetadataPacket` 同步 → 客户端 `ClientEntity::syncMetadataFromDataManager` 读取 → `m_witherHeadTargetId[3]` → `ClientEntityManager::tick` 调用 `tickWitherSideHeads` → 客户端独立镜像 MC 计算 → `getInterpolatedWitherSideHeadYaw/Pitch` → `EntityRendererManager` → `WitherModel::setSideHeadRotations` → `WitherModel::setAngles` 应用到 `m_heads[1]`/`m_heads[2]`。
    - **与狼甩水/兔子跳跃模式的差异**：狼甩水/兔子跳跃通过 `EntityStatusPacket` 触发状态镜像 + `tick()` 推进；凋灵侧头通过 `EntityMetadataPacket` 同步目标 ID + `tickWitherSideHeads` 独立计算朝向。共同点是"服务端权威 → 客户端镜像状态 → tick 推进 → 渲染器读取"。

15. **钓鱼浮标网络同步镜像**（参考 MC 1.21.11 `FishingHook.onSyncedDataUpdated()`）：
    - **字段**：
      - `m_fishingHookedEntityId`（i32）：被钩住实体 ID（+1 偏移，0=无）。由 `FishingBobberEntity::DATA_HOOKED_ENTITY_PARAM` 同步。
      - `m_fishingBiting`（bool）：是否咬钩。由 `FishingBobberEntity::DATA_BITING_PARAM` 同步。
    - **元数据同步**：`syncMetadataFromDataManager()` 中 `typeId == "minecraft:fishing_bobber" || "fishing_bobber"` 分支读取 `FishingBobberEntity::getHookedEntityParamId()` / `getBitingParamId()` 对应参数，写入镜像字段。对应 MC 1.21.11 `FishingHook.onSyncedDataUpdated(DATA_HOOKED_ENTITY/DATA_BITING)`。
    - **访问器**：`fishingHookedEntityId()` / `fishingBiting()` 供渲染器读取。
    - **读取**：`FishingBobberRenderer::generateMesh()` 读取 `fishingBiting()` 驱动浮标 Y 偏移下沉（咬钩时 0.15 替代 0.25，模拟 MC 中 `DATA_BITING` 触发的 `-0.4*random[0.6,1.0]` 向下速度视觉效果）；读取 `fishingHookedEntityId()` 判断是否有被钩实体，有则钓线绷紧（下垂量减半）。
    - **数据流**：服务端 `FishingBobberEntity::_syncCaughtEntityId()` / `_catchingFish()` / `tick()` 写入 `DATA_HOOKED_ENTITY_PARAM` / `DATA_BITING_PARAM` → `EntityMetadataPacket` 同步 → 客户端 `syncMetadataFromDataManager` 读取 → `m_fishingHookedEntityId` / `m_fishingBiting` → `FishingBobberRenderer::generateMesh` 消费。
    - **+1 偏移**：`DATA_HOOKED_ENTITY` 存储 `entityId+1`，0 专门表示"无被钩住实体"，避免与合法 entityId=0 冲突。渲染器通过 `fishingHookedEntityId() > 0` 判断是否存在被钩实体。

16. **Mob 激怒状态镜像**（参考 MC 1.21.11 `Mob.MOB_FLAG_AGGRESSIVE`）：
    - **字段**：`m_isAggressive`（bool）— Mob 激怒状态镜像，驱动僵尸系模型 `ZombieModel::setAngles` 的 `animateZombieArms` 攻击抬臂动画。
    - **元数据同步**：`syncMetadataFromDataManager()` 通过 `m_dataManager.hasParam(MobEntity::getMobFlagsParamId())` 检测参数是否存在（**所有** MobEntity 子类都注册了 `DATA_MOB_FLAGS_PARAM`，因此不按 typeId 分发）。读取 `DataParameter<i8>` 后位与 `MobEntity::getAggressiveFlagMask()`（0x04），写入 `m_isAggressive`。对应 MC 1.21.11 `Mob.onSyncedDataUpdated(DATA_MOB_FLAGS_ID)` 的位 2 解码。
    - **访问器**：`isAggressive()` / `setIsAggressive(bool)` 供渲染器读取与外部设置。
    - **数据流**：服务端 `MobEntity::setAggressive(true)` → `DATA_MOB_FLAGS_PARAM` 位 2 置位 → `EntityMetadataPacket` 广播 → 客户端 `syncMetadataFromDataManager` 读取位 2 → `m_isAggressive=true` → `EntityRendererManager::_applyZombieState` 推送到 `ZombieModel::setAggressive` → `ZombieModel::setAngles` 按 `f1 = -PI/(aggressive?1.5:2.25)` 计算手臂前伸基础角度。
    - **位隔离**：`DATA_MOB_FLAGS_PARAM` 还携带 NO_AI（位 0）/ LEFTHANDED（位 1）等标志，客户端只读取位 2，不影响其他位解码。
    - **覆盖范围**：所有 MobEntity 子类（zombie/husk/drowned/zombie_villager/skeleton/creeper/enderman/...）共享同一同步路径，无需为每个 typeId 编写分支。
    - **与骷髅拉弓同步的差异**：骷髅拉弓状态（`AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM`）按 typeId 分发，因为是骷髅特有的独立参数；激怒状态通过 `DATA_MOB_FLAGS_PARAM` 通用同步，因为所有 MobEntity 子类都注册了该参数。

17. **游泳状态镜像与 swimAmount 渐变**（参考 MC 1.21.11 `LivingEntity.isVisuallySwimming` / `HumanoidMobRenderer.extractHumanoidRenderState`）：
    - **字段**：`m_swimAmount`（f32，当前 tick）/ `m_swimAmountO`（f32，上一 tick）— 游泳动画渐变量，驱动 `DrownedModel::setAngles` 的溺尸专属游泳手臂/腿部覆盖动画。
    - **元数据同步**：`syncMetadataFromDataManager()` 中 Mob flags 分支之后新增 Swimming 位读取：从 `DATA_FLAGS_PARAM`（slot 0，i8）位 4（`EntityFlags::Swimming`）解码，调用 `setSwimming(bool)`。对应 MC 1.21.11 `Entity.onSyncedDataUpdated(DATA_SHARED_FLAGS_ID)` 的位 4 解码。服务端 `DrownedEntity::updateSwimming` 按 `areEyesInWater && isInWater && wantsToSwim && !isRiding` 设置该位。
    - **推进**：`tick()` 末尾调用 `updateSwimAmount()`（对应 MC `LivingEntity::updateSwimAmount`）：`m_swimAmountO = m_swimAmount`，`isVisuallySwimming()` 为真时 `m_swimAmount = min(1, m_swimAmount + 0.09)`，否则 `m_swimAmount = max(0, m_swimAmount - 0.09)`。
    - **isVisuallySwimming**：`ClientEntity::isVisuallySwimming()` 返回 `isSwimming() && !isRiding()`（与 `DrownedEntity::isVisuallySwimming` 语义一致；普通实体回退到 `Entity::isVisuallySwimming` 基于 Pose 的判定）。
    - **访问器**：`swimAmount()` / `swimAmountO()` / `getInterpolatedSwimAmount(partialTicks)` 供渲染器读取。插值使用 `math::lerp(m_swimAmountO, m_swimAmount, partialTicks)`（注意本项目 `math::lerp(a, b, t)` 与 MC `Mth.lerp(t, a, b)` 参数顺序相反）。
    - **数据流**：服务端 `DrownedEntity::updateSwimming` → `EntityFlags::Swimming` 位 4 → `EntityMetadataPacket` → 客户端 `syncMetadataFromDataManager` 读取位 4 → `setSwimming` → `tick()` 中 `updateSwimAmount` 渐变 → `getInterpolatedSwimAmount` → `EntityRendererManager` 写入 `AnimationContext::swimAmount` → `_applyZombieState` 推送 `setSwimAnimation` → `DrownedModel::setAngles` 执行游泳覆盖。
    - **未实现项**：MC 1.21.11 `DrownedRenderer.setupRotations` 的游泳身体倾斜（渲染器层整体 X 轴旋转）暂留 TODO，见 `MonsterVariantRenderers.hpp` 中 DrownedRenderer 类注释。

