# 玩家渲染器

本目录包含玩家实体的渲染器实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `PlayerRenderer.hpp` | 玩家渲染器头文件 |
| `PlayerRenderer.cpp` | 玩家渲染器实现 |

## 渲染器详解

### PlayerRenderer（玩家渲染器）

继承自 `LivingRenderer<PlayerEntity, PlayerModel>`。

**特点**：
- 阴影大小：0.5
- 支持标准手臂和纤细手臂两种模式
- 包含外层皮肤渲染
- 支持斗篷和特殊皮肤（如 Deadmau5）

**层渲染器**：
- `BipedArmorLayer` - 盔甲层
- `HeldItemLayer` - 手持物品层
- `ArrowLayer` - 箭矢附着层
- `Deadmau5HeadLayer` - Deadmau5 头部层
- `CapeLayer` - 斗篷层
- `HeadLayer` - 头部物品层
- `ElytraLayer` - 鞘翅层
- `ParrotVariantLayer` - 鹦鹉层
- `SpinAttackEffectLayer` - 旋转攻击效果层
- `BeeStingerLayer` - 蜜蜂刺层

**模型可见性设置**：
```cpp
void setModelVisibilities(PlayerEntity& player) {
    // 根据玩家设置显示/隐藏各部件
    // - 头套：PlayerModelPart.HAT
    // - 外套：PlayerModelPart.JACKET
    // - 左裤腿：PlayerModelPart.LEFT_PANTS_LEG
    // - 右裤腿：PlayerModelPart.RIGHT_PANTS_LEG
    // - 左袖：PlayerModelPart.LEFT_SLEEVE
    // - 右袖：PlayerModelPart.RIGHT_SLEEVE
}
```

**手臂姿态判定**：
```cpp
ArmPose determineArmPose(PlayerEntity& player, bool mainHand) {
    // 根据手持物品和使用状态确定姿态
    // - UseAction.BLOCK -> ArmPose::Block
    // - UseAction.BOW -> ArmPose::BowAndArrow
    // - UseAction.SPEAR -> ArmPose::ThrowSpear
    // - UseAction.CROSSBOW (正在使用) -> ArmPose::CrossbowCharge
    // - UseAction.CROSSBOW (已装填) -> ArmPose::CrossbowHold
    // - 其他非空物品 -> ArmPose::Item
}
```

**参考**：MC 1.16.5 PlayerRenderer

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer::player {
    class PlayerRenderer;
}
```

## 使用示例

```cpp
// 创建标准手臂玩家渲染器
auto playerRenderer = std::make_unique<PlayerRenderer>(false);

// 创建纤细手臂玩家渲染器
auto slimRenderer = std::make_unique<PlayerRenderer>(true);

// 渲染玩家
playerRenderer->render(playerEntity, partialTicks);

// 获取纹理
auto texture = playerRenderer->getEntityTexture(playerEntity);

// 渲染手臂（第一人称）
playerRenderer->renderRightArm(player, partialTicks);
playerRenderer->renderLeftArm(player, partialTicks);
```

## 注册渲染器

```cpp
#include "PlayerRenderer.hpp"

// 注册玩家渲染器
mc::client::renderer::entity::renderer::player::registerPlayerRenderers();
```

## 依赖关系

```
PlayerRenderer.hpp
├── core/LivingRenderer.hpp
└── model/player/PlayerModel.hpp

PlayerRenderer.cpp
├── PlayerRenderer.hpp
└── core/EntityRendererManager.hpp
```

## 第一人称渲染

玩家渲染器还提供第一人称手臂渲染：

```cpp
// 渲染右手臂
playerRenderer->renderRightArm(player, partialTicks);

// 渲染左手臂
playerRenderer->renderLeftArm(player, partialTicks);
```

这用于：
- 第一人称视角手持物品渲染
- 攻击动画
- 物品使用动画

### renderRightArm / renderLeftArm 实现

这两个方法内部调用 `PlayerModel::renderRightArm()` 和 `PlayerModel::renderLeftArm()`：

```cpp
void PlayerRenderer::renderRightArm(::mc::Player& player, f64 partialTicks)
{
    // 设置模型可见性
    setModelVisibilities(player);

    // 重置动画状态（参考 MC 1.16.5 PlayerRenderer.renderItem）
    m_model.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);
    m_model.setSwingProgress(0.0f);
    m_model.setCrouching(false);
    m_model.setSwimming(false);
    m_model.setSwimAnimation(0.0f);

    // 渲染右手臂和右袖
    m_model.renderRightArm(1.0 / 16.0);
}
```

**参考**：MC 1.16.5 PlayerRenderer.renderRightArm/renderLeftArm

## 注意事项

- 玩家模型需要在渲染前设置正确的可见性
- 手臂姿态需要根据手持物品动态更新
- 斗篷位置受蹲伏和胸甲状态影响
- 第一人称渲染只渲染手臂，不渲染全身
