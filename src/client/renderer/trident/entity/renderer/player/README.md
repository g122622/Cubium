# 玩家渲染器

本目录包含玩家实体的渲染器实现。

## 目录结构

```
player/
├── PlayerRenderer.hpp           # 玩家渲染器头文件
├── PlayerRenderer.cpp           # 玩家渲染器实现
├── PlayerArmPoseResolver.hpp    # 手臂姿态解析器（纯逻辑，可独立测试）
├── PlayerArmPoseResolver.cpp    # 手臂姿态解析器实现
└── README.md                    # 本文档
```

## 内部模块关系

```
PlayerRenderer
├── 继承体系
│   ├── EntityRenderer (基类：阴影渲染、基础实体渲染)
│   └── IEntityRenderer<Player, PlayerModel> (接口：模型访问、纹理获取)
├── 核心组件
│   ├── PlayerModel m_model (玩家模型，支持标准/纤细手臂)
│   ├── LayerRenderer[] m_layers (层渲染器列表)
│   └── PlayerArmPoseResolver (手臂姿态解析器，纯逻辑工具类)
└── 层渲染器（按顺序添加）
    ├── HeldItemLayer (手持物品层)
    ├── HeadLayer (头部物品层：头盔、南瓜等)
    ├── CapeLayer (披风层)
    └── ElytraLayer (鞘翅层)
```

**渲染流程：**
1. `render()` → 设置模型可见性 → 计算动画参数 → 渲染模型 → 渲染阴影
2. `renderLayersPipeline()` → 遍历层渲染器 → 传递纹理（披风、鞘翅）→ 渲染各层
3. `renderRightArm()` / `renderLeftArm()` → 第一人称手臂渲染

## 上下游外部依赖关系

**依赖了谁（上游）：**

| 模块 | 用途 |
|------|------|
| `entity/core/EntityRenderer` | 实体渲染器基类 |
| `entity/core/IEntityRenderer` | 实体渲染器接口 |
| `entity/model/player/PlayerModel` | 玩家模型 |
| `entity/layer/equipment/HeldItemLayer` | 手持物品层 |
| `entity/layer/equipment/HeadLayer` | 头部物品层 |
| `entity/layer/cosmetic/CapeLayer` | 披风层 |
| `entity/layer/cosmetic/ElytraLayer` | 鞘翅层 |
| `common/entity/entities/player/Player` | 玩家实体类 |
| `common/resource/ResourceLocation` | 纹理资源路径 |

**被谁依赖（下游）：**

| 模块 | 用途 |
|------|------|
| `renderer/RendererRegistration.cpp` | 注册玩家渲染器到工厂 |
| `client/world/ClientWorld` | 通过 EntityRendererManager 渲染玩家 |
| `client/skin/ClientSkinManager` | 设置玩家皮肤纹理 |

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer::player {
    class PlayerRenderer;
    class PlayerArmPoseResolver;
}
```

## 容易踩的坑

### 1. 纤细手臂模型差异

`PlayerRenderer(bool slimArms)` 构造参数决定手臂类型。纤细手臂的模型尺寸和纹理坐标与标准手臂不同，创建时必须传入正确参数。

### 2. 层渲染器纹理传递

披风和鞘翅纹理通过 `dynamic_cast` 在 `renderLayersPipeline()` 中传递给对应的层渲染器。如果层渲染器类型不匹配，纹理不会被设置。

### 3. 模型可见性设置顺序

`setModelVisibilities(player, partialTicks)` 必须在渲染前调用，它会：
1. 设置所有部件可见
2. 通过 `PlayerArmPoseResolver::resolveArmPoses()` 解析双手姿态并映射到模型左右臂
3. 设置 `mainHand`/`swingingHand` 供 `BipedModel::setAngles` 双臂协调逻辑使用
4. 根据 `playerModelParts()` 设置外层皮肤可见性
5. 设置蹲伏/游泳状态
6. 计算弩装填参数并写入模型：
   - `maxChargeDuration = CrossbowItem::getChargeTime(stack)`
   - `ticksUsingItem = useDuration - getItemInUseCount() + partialTicks`
     （Cubium 中 `getItemInUseCount` 返回剩余 ticks，故用 `useDuration` 反推
     已使用 ticks，对应 MC `HumanoidMobRenderer.extractHumanoidRenderState` 填充逻辑）
   - 通过 `m_model.setMaxCrossbowChargeDuration` / `setCrossbowChargeTicks` 写入

**注意**：披风可见性由 `CapeLayer` 单独控制，不在 `setModelVisibilitiesFromFlags` 中。

### 3.1 第三人称 GPU 管线路径的弩参数填充

第三人称玩家走 GPU 管线路径（`EntityRendererManager::_createModelForEntity`），
不经过 `PlayerRenderer::setModelVisibilities`。为避免弩动画参数缺失导致
`handleCrossbowCharge` 中 progress 恒为 0，`_createModelForEntity` 玩家分支
会调用 `_applyPlayerCrossbowState`，从本地 `Player` 对象读取 use-item 状态
并填充弩参数与 ArmPose。

本地玩家访问通过 `EntityRendererManager::setLocalPlayerAccessor` 注入，
由 `ClientApplicationSession` 在实体渲染回调中每帧更新。**远程玩家缺
use-item 状态的网络同步，弩动画在远程玩家上暂时不生效**（见代码中
TODO 注释）。

### 4. 第一人称手臂渲染

`renderRightArm()` / `renderLeftArm()` 会重置动画状态（setAngles 参数全为 0、swingProgress 为 0、crouching/swimming 为 false），仅渲染手臂部件，用于第一人称视角。

### 5. determineArmPose 实现说明

`determineArmPose(player, hand)` 对应 MC 1.21.11 `AvatarRenderer.getArmPose` 实现，根据手持物品与使用状态返回对应 `ArmPose`：

1. 空手 → `Empty`
2. 已装填的弩（且未挥动）→ `CrossbowHold`
3. 正在使用物品且使用的手就是当前判断的手：
   - `Block`（盾牌）→ `Block`
   - `Bow`（弓）→ `BowAndArrow`
   - `Spear`/`Trident`（三叉戟/长矛）→ `ThrowSpear`
   - `Crossbow`（弩装填）→ `CrossbowCharge`
   - `Spyglass`（望远镜）→ `Spyglass`
   - `Brush`（刷子）→ `Brush`
   - `Bundle`（收纳袋）→ 暂降级为 `Item`（TODO：第三人称 ArmPose 无 EatOrDrink 枚举，未来扩展后应改为返回该姿态）
4. 长矛类物品（`ItemTags::SPEARS`）→ `ThrowSpear`
5. 默认持有物品 → `Item`

`setModelVisibilities()` 中还会执行双手姿态协调：若主手姿态为 `BowAndArrow`/`CrossbowCharge`/`CrossbowHold`，副手姿态降级为 `Empty`（副手空）或 `Item`（副手非空）。随后根据 `player.isRightHanded()` 将主/副手姿态映射到模型右臂/左臂。

`Spyglass`/`Brush` 姿态为单手动作（`isTwoHanded=false`），不参与双手协调，由 `BipedModel::handleRightArmPose`/`handleLeftArmPose` 中的对应 case 分支按 MC 1.21.11 `HumanoidModel.poseRightArm`/`poseLeftArm` 角度公式设置：
- `Spyglass`：手臂 X 跟随头部 pitch（带 clamp 与蹲伏偏移），Y 为头部 yaw ± π/12
- `Brush`：手臂 X 半折后偏移 -π/5，Y 归零

### 6. PlayerArmPoseResolver 工具类

`PlayerArmPoseResolver` 是纯逻辑工具类，不依赖任何渲染层（Vulkan/管线/层渲染器），仅依赖 `Player`/`ItemStack`/`Item`/`Items`/`CrossbowItem`/`ItemTags`/`UseAction`。`PlayerRenderer::determineArmPose` 与 `setModelVisibilities` 中的姿态解析逻辑均委托给该类，便于在单元测试中直接验证各分支逻辑与双手协调映射，无需构造完整 `PlayerRenderer`（其构造会拉起 `HeldItemLayer`/`HeadLayer`/`CapeLayer`/`ElytraLayer` 等渲染层依赖）。对应测试见 `tests/client/renderer/entity/PlayerArmPoseResolverTest.cpp`。

### 7. ArmPose 枚举复用基类

`PlayerModel` 不再自定义 `ArmPose` 枚举，而是通过 `using ArmPose = mc::client::renderer::entity::model::ArmPose;` 复用基类 `BipedModel` 的枚举。`setArmPose`/`setLeftArmPose`/`setRightArmPose` 直接写入基类字段 `m_leftArmPose`/`m_rightArmPose`，由 `BipedModel::setAngles → handleRightArmPose/handleLeftArmPose` 消费。这避免了字段隐藏导致姿态永远为 `Empty` 的问题。
