# 核心渲染器

本目录包含实体渲染系统的核心组件。

## 目录结构

```
core/
├── IEntityRenderer.hpp       # 实体渲染器接口，将渲染器与模型类型解耦
├── EntityRenderer.hpp/cpp    # 实体渲染器基类，定义渲染接口（阴影、名称标签等）
├── LivingRenderer.hpp        # 生物渲染器模板类，支持层渲染器系统和动画参数计算
├── EntityRendererManager.hpp/cpp  # 渲染器管理器，管理所有实体渲染器和网格缓存
├── AnimatedMeshCache.hpp/cpp # 动画网格缓存，按状态变化节流更新 GPU 网格
├── AnimationContext.hpp/cpp  # 动画上下文，存储实体的动画状态用于传递给渲染器
└── RendererFactory.hpp/cpp   # 渲染器工厂，统一管理渲染器创建（注册表模式）
```

## 内部模块关系

```
IEntityRenderer ◄────────── LivingRenderer ◄─────── 具体渲染器（在 renderer/ 目录）
       │                            │
       │                            │
       ▼                            ▼
  EntityRenderer              LayerRenderer（在 layer/ 目录）
       │
       ▼
EntityRendererManager ─────► NameTagRenderer ─────► WorldTextRenderer
        │
        ├──► AnimatedMeshCache（动画网格缓存）
        └──► RendererFactory（渲染器创建）
```

## 上下游外部依赖关系

**上游依赖（本目录依赖）**：
- `model/core/EntityModel` - 模型基类
- `model/core/AgeableModel` - 可成长模型基类（在 model/core/ 目录）
- `layer/core/LayerRenderer` - 层渲染器基类
- `pipeline/EntityPipeline` - Vulkan 渲染管线
- `pipeline/EntityTextureAtlas` - 实体纹理图集
- `util/NameTagRenderer` - 名称标签渲染器
- `util/ShadowRenderer` - 阴影渲染器
- `common/entity/core/LivingEntity` - 生物实体基类
- `common/entity/core/AgeableEntity` - 可成长实体基类

**下游依赖（依赖本目录）**：
- `renderer/` 下所有具体渲染器（animal、monster、player、projectile、vehicle 等）
- `effect/` 下的特效系统（glow、fire、hurt）
- `layer/` 下的层渲染器

## 容易踩的坑

### 阴影渲染条件

`shouldRenderShadow()` 方法会检测实体是否处于隐身状态，隐身实体不渲染阴影。调试阴影问题时首先检查：
1. `EntityFlags::Invisible` 标志是否设置
2. `m_shadowSize > 0` 且 `m_shadowAlpha > 0`

### 动画网格缓存节流策略

AnimatedMeshCache 有三种更新频率：
- 姿态切换（坐下/蹲伏/游泳/骑乘/幼体）立即更新
- 活跃动画按 2 帧节流更新
- 非活跃动画按 6 帧节流更新
- 最多 12 帧强制刷新一次，防止状态漂移

如果动画看起来卡顿或不同步，检查 `STATE_CHANGE_THRESHOLD = 0.08` 是否适合当前动画。

### 状态哈希比较

AnimationContext 使用状态哈希快速比较动画状态是否变化。如果动画状态更新但网格没有更新，检查 `computeHash()` 是否正确计算了所有相关状态字段。

### 相机信息必须每帧设置

EntityRendererManager::setCameraInfo() 必须在每帧渲染实体前调用，用于名称标签的视锥剔除和背面剔除。如果名称标签渲染异常，首先检查相机信息是否正确设置。

### CPU 网格单位

CPU 网格保持 MC 模型单位，实体着色器统一用 push constant scale 应用 1/16 缩放。不要在 CPU 端预先缩放网格数据。

### EntityRendererFactory 已弃用

EntityRendererFactory（在 EntityRenderer.hpp 中）已弃用，应使用 RendererFactory（在 RendererFactory.hpp 中）。RendererFactory 使用注册表模式替代巨型 if-else 链，与 ModelFactory 设计保持一致。

### 幼体动画速度

LivingRenderer::getLimbSwing() 中，幼体的动画速度会自动乘以 3.0。如果动画看起来太快或太慢，检查 `isChild()` 状态是否正确。

### PipelineMeshProvider 用于特殊几何体

Arrow、Boat、Minecart、FishingBobber 等实体的自定义几何体通过 PipelineMeshProvider 接口提供，不是通过 ModelFactory。如果实体不支持 ModelFactory 动画模型，需要实现此接口并重写 `getPipelineMeshProvider()` 返回 this。

### 实体特定动画状态注入

`_createModelForEntity` 中按 `normalizedId` 分发，为特定实体模型注入 AnimationContext 之外的实体特定状态：

| 实体 | 分支 | 读取字段 | 调用方法 | 用途 |
|------|------|----------|----------|------|
| 狼 (`wolf`) | wolf | `wolfShakeAnim`/`wolfInterestedAngle`/`wolfIsWet`/`isSitting` | `WolfModel::setAnimState` + `setLivingAnimations` + `setTint` | 甩水动画、乞求食物、湿润着色、坐下姿态 |
| 兔子 (`rabbit`) | rabbit | `rabbitJumpCompletion(partialTick)` | `RabbitModel::setJumpRotation(sin(completion * PI))` + `setLivingAnimations` | 跳跃动画（thigh/foot/arm 旋转） |
| 羊 (`sheep`) | sheep | `eatAnimationTimer` | `SheepModel::setEatAnimationTimer` + `setLivingAnimations` | 吃草低头动画 |
| 北极熊 (`polar_bear`) | polar_bear | `standingProgress` | `PolarBearModel::setStandingProgress` | 后腿站立动画 |
| 疣猪兽 (`hoglin`/`zoglin`) | boar | `attackAnimationTicks` | `BoarModel::setAttackAnimationTicks` | 甩头攻击动画 |
| 河豚 (`pufferfish`) | pufferfish | `puffState` | 替换为对应大小的模型 | 膨胀状态 |
| 僵尸系 (`zombie`/`husk`/`drowned`/`zombie_villager`/`giant`) | zombie | `isAggressive()` | `_applyZombieState(model, entity)` → `ZombieModel::setAggressive` | 激怒状态驱动 `animateZombieArms` 攻击抬臂动画 |

**僵尸激怒状态分支**（参考 MC 1.21.11 `ZombieRenderer` + `AnimationUtils.animateZombieArms`）：
1. 服务端 `MobEntity::setAggressive(true)` → `DATA_MOB_FLAGS_PARAM` 位 2 置位
2. 客户端 `ClientEntity::syncMetadataFromDataManager` 读取位 2 → `m_isAggressive=true`
3. `_createModelForEntity` 僵尸分支调用 `_applyZombieState(model, entity)`
4. `_applyZombieState` 读取 `entity.isAggressive()`，通过 `dynamic_cast<ZombieModel*>` 安全转换后调用 `model->setAggressive(aggressive)`
5. `ZombieModel::setAngles` 中按 `f1 = -PI/(m_isAggressive ? 1.5 : 2.25)` 应用 `animateZombieArms` 攻击抬臂动画

`HuskModel`/`DrownedModel`/`ZombieVillagerModel`/`GiantModel` 均继承 `ZombieModel`，因此 `dynamic_cast<ZombieModel*>` 对这些变体模型同样命中，无需为每个变体单独编写分支。

**兔子跳跃分支数据流**（参考 MC 1.21.11 `Rabbit.getJumpCompletion` + `RabbitModel.setupAnim`）：
1. 服务端 `RabbitEntity::startJumping()` 广播 `mc::network::EntityStatus::RabbitJump(1)`（经 `ir::play::EntityEvent`；旧 `EntityStatusPacket` 已删除）
2. 客户端 `onEntityStatus` 调用 `ClientEntity::setRabbitJumpStart()`（`m_rabbitJumpDuration=10`）
3. `ClientEntity::tick()` 中 `tickRabbitJump()` 推进 `m_rabbitJumpTicks`
4. 此处读取 `entity.rabbitJumpCompletion(partialTick)` 计算 `jumpRotation = sin(completion * PI)`
5. 调用 `RabbitModel::setJumpRotation(jumpRotation)`，`setAngles` 中据此计算 thigh/foot/arm 旋转角度

新增实体动画时，优先复用此"状态包触发 → 客户端镜像字段 → tick 推进 → 渲染器读取"模式，与狼甩水、兔子跳跃保持一致。激怒状态走的是"元数据包（`ir::play::SetEntityData`）触发 → `syncMetadataFromDataManager` 镜像 → 渲染器读取"模式，与骷髅拉弓同步模式一致。
