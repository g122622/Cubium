# 基础模型

本目录包含实体模型的基础模板类，为具体实体模型提供通用骨架结构。

## 目录结构

```
base/
├── BipedModel.hpp           # 双足模型基类（玩家、僵尸、骷髅等）
├── BipedModel.cpp           # 双足模型实现
├── ElytraSpeedValue.hpp     # 鞘翅飞行速度因子纯逻辑函数（computeSpeedValue）
├── ElytraSpeedValue.cpp     # 鞘翅飞行速度因子实现
├── QuadrupedModel.hpp       # 四足模型基类（猪、牛、羊等）
└── QuadrupedModel.cpp       # 四足模型实现
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                         base/                               │
│  ┌─────────────────┐      ┌───────────────────┐            │
│  │   BipedModel    │      │  QuadrupedModel   │            │
│  │ (双足模型基类)   │      │  (四足模型基类)    │            │
│  └────────┬────────┘      └────────┬──────────┘            │
│           │                        │                        │
│           └──────────┬─────────────┘                        │
│                      │                                      │
│                      ▼                                      │
│           ┌─────────────────────┐                           │
│           │    AgeableModel     │  (core/ 可成长模型基类)    │
│           │  (支持幼体/成年体)   │                           │
│           └──────────┬──────────┘                           │
│                      │                                      │
│                      ▼                                      │
│           ┌─────────────────────┐                           │
│           │    EntityModel      │  (core/ 实体模型基类)      │
│           │  (模型渲染接口)      │                           │
│           └─────────────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本目录依赖的外部模块

| 模块 | 用途 |
|------|------|
| `core/AgeableModel` | 父类，提供幼体/成年体缩放支持 |
| `core/ModelRenderer` | 模型部件渲染器 |
| `common/util/math/Vector3` | 数学向量类型 |

### 依赖本目录的外部模块

| 模块 | 用途 |
|------|------|
| `model/player/PlayerModel` | 玩家模型，继承 BipedModel |
| `model/monster/ZombieModel` | 僵尸模型，继承 BipedModel |
| `model/monster/SkeletonModel` | 骷髅模型，继承 BipedModel |
| `model/monster/EndermanModel` | 末影人模型，继承 BipedModel |
| `model/animal/PigModel` / `CowModel` / `SheepModel` | 猪、牛、羊模型，继承 QuadrupedModel |
| `model/aquatic/AquaticModels` | 水生生物模型 |
| `layer/equipment/ArmorLayer` | 盔甲渲染层，使用 BipedModel 部件可见性 |
| `layer/equipment/HeldItemLayer` | 手持物品渲染层，使用 translateHand 方法 |

## 容易踩的坑

### BipedModel 部件命名别名

BipedModel 有两套部件命名：`m_bipedXxx`（原名）和 `m_xxx`（别名引用）。子类应使用 `m_head`、`m_body` 等简短别名，避免直接访问 `m_bipedHead` 等原名称。

### translateHand 矩阵变换顺序

`translateHand()` 的变换顺序为：平移到旋转点 → Z轴旋转 → Y轴旋转 → X轴旋转。HeldItemLayer 组合物品变换时需遵循此顺序。

### 幼体模型缩放

AgeableModel 父类通过 `m_isChild` 控制幼体渲染，幼体会对头部和身体应用不同的缩放矩阵。子类构造时需正确传入 `childHeadScale`、`childBodyScale` 等参数，否则幼体渲染会错位。

### 盔甲槽位可见性

BipedModel 的盔甲槽位可见性设置需参考 MC 1.16.5 `BipedArmorLayer.setModelSlotVisible`：
- Head 槽位：显示头部 + 帽子层
- Chest 槽位：显示身体 + 双臂
- Legs 槽位：显示身体 + 双腿
- Feet 槽位：显示双腿

### 动画参数来源

`setAngles()` 的参数由 LivingRenderer 计算：
- `limbSwing`：步态周期（插值后的 prevLimbSwing → limbSwing）
- `netHeadYaw`：头部偏航角（rotationYawHead - renderYawOffset）
- 不要在模型内部重新计算这些值，应直接使用传入参数

### 弩装填/持握动画

BipedModel 通过两个 protected 辅助方法实现弩的双手协调姿态，对应 MC 1.21
`AnimationUtils.animateCrossbowCharge` / `animateCrossbowHold`：

- `handleCrossbowCharge(isRightHanded)`：由 `handleRightArmPose` /
  `handleLeftArmPose` 在 `ArmPose::CrossbowCharge` 分支调用。主手固定向前下方
  （Y=±0.8, X=-0.97079635），副手 X 从 -0.97079635 lerp 到 -PI/2、Y 从 ±0.4
  lerp 到 ±0.85，进度由 `m_crossbowChargeTicks / m_maxCrossbowChargeDuration`
  归一化（含 clamp 防越界、除零保护）。
- `handleCrossbowHold(isRightHanded)`：由 `handleRightArmPose` /
  `handleLeftArmPose` 在 `ArmPose::CrossbowHold` 分支调用。主副手 Y/X 角度
  跟随头部 yaw/pitch，呈双手持弩瞄准姿态。

调用约定（避免双手协调姿态被重复设置）：
- 右手分支：`CrossbowCharge`/`CrossbowHold` 直接调用 handler
- 左手分支：先判断右手姿态是否已为同一姿态，是则跳过（双手已在右手分支设置）

状态字段 setter（由渲染器在 `setAngles` 前调用）：
- `setCrossbowChargeTicks(f32)`：已使用 ticks（含 partialTick 插值），
  对应 MC `HumanoidRenderState.ticksUsingItem`
- `setMaxCrossbowChargeDuration(f32)`：弩最大装填时长（ticks），
  对应 MC `HumanoidRenderState.maxCrossbowChargeDuration`，
  由渲染器调用 `CrossbowItem::getChargeTime(stack)` 计算

### 鞘翅飞行状态与速度因子

BipedModel 通过 `setFallFlying(bool)` 与 `setSpeedValue(f32)` 接收鞘翅飞行状态，
对应 MC 1.21.11 `HumanoidRenderState.isFallFlying` 与 `HumanoidRenderState.speedValue`：

- `setFallFlying(bool)`：直接布尔，对应 `Entity::isElytraFlying()` /
  `ClientEntity::isFallFlying()`。控制头部角度（飞行时强制 -π/4）。
- `setSpeedValue(f32)`：速度因子，由渲染器调用 `elytra::computeSpeedValue`
  （`ElytraSpeedValue.hpp`）按 MC 公式计算：
  - 默认 1.0
  - 鞘翅飞行时 `speedValue = (velocity.lengthSquared() / 0.2)^3`
  - 钳制到 `[1.0, +∞)`
  - 用作手臂/腿部摆动振幅的除数（值越大摆动越慢，模拟风阻）
- `setElytraFlyingTicks(i32)`：历史遗留字段，MC 1.21.11
  `HumanoidRenderState` 已移除 `fallFlyTicks`，`HumanoidModel.setupAnim`
  仅检查 `isFallFlying` 布尔。Cubium 中 `PlayerRenderer::setModelVisibilities`
  会将服务端 `LivingEntity::fallFlyTicks()` 推送给 BipedModel 以保持状态机一致性，
  但 `setAngles` 不读取此字段（与 MC 1.21.11 行为一致）。

`elytra::computeSpeedValue(bool, f32)` 抽取为自由函数（`ElytraSpeedValue.hpp/cpp`），
便于在 GPU 管线路径（`EntityRendererManager::_applyBipedElytraState`）与 CPU
路径（`PlayerRenderer::setModelVisibilities`）共用，并在单元测试中直接验证公式分支
（`tests/client/renderer/entity/ElytraSpeedValueTest.cpp`）。

推送位置：
- GPU 管线路径：`EntityRendererManager::_createModelForEntity` 通用 BipedModel 分支
  调用 `_applyBipedElytraState`，覆盖所有 BipedModel 派生模型（玩家+所有怪物）。
  通过 `dynamic_cast<BipedModel*>` 守卫，非 BipedModel 派生模型（蜘蛛/苦力怕/猪等）
  返回 nullptr 跳过此步（见 `tests/client/renderer/entity/BipedModelDynamicCastTest.cpp`）。
- CPU 路径：`PlayerRenderer::setModelVisibilities` 中按 `player.isElytraFlying()` 计算
- 第一人称手臂渲染：`renderRightArm/renderLeftArm` 中重置为 `setFallFlying(false)`
  与 `setSpeedValue(1.0f)`，避免鞘翅状态影响第一人称手臂姿态

**鞘翅滑翔状态机已完整实现**（对应 MC 1.21.11 LivingEntity 中的 elytra 逻辑）：
- 服务端 `LivingEntity::tick` 末尾递增/重置 `fallFlyTicks`
- `LivingEntity::aiStep` 在 `travel()` 前调用 `updateFallFlying()`
  维护可滑翔条件、周期性触发 `ELYTRA_GLIDE` 游戏事件与装备损坏
- `LivingEntity::travel` 开头根据 `isElytraFlying()` 分发到 `travelFallFlying()`
  处理滑翔物理（视线方向驱动、重力抵消、撞墙伤害）
- `Player` 重写 `canGlide()`/`tryToStartFallFlying()` 排除创造飞行模式
- `ServerPlayRouter` 的 EntityAction 分支处理 `StartFallFlying`
  （客户端按下空格触发，服务端校验后设置 FallFlying 标志；旧 `PacketHandler::handleEntityAction` 已删除，逻辑迁入 ServerPlayRouter）

### 命名空间

所有模型类位于 `mc::client::renderer::entity::model` 命名空间（注意：不是 `model::base` 子命名空间）。
