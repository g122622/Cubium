# 玩家模型

本目录包含玩家实体的模型实现。

## 目录结构

```
player/
├── PlayerModel.hpp     # 玩家模型类定义
├── PlayerModel.cpp     # 玩家模型实现
└── README.md           # 本文档
```

## 内部模块关系

```
PlayerModel
├── 继承自 BipedModel（双足模型基类）
│   ├── 头部、头套、身体、手臂、腿部等基础部件
│   └── 基础动画（走路、游泳、蹲伏等）
├── 外观层部件（外层皮肤）
│   ├── m_leftArmwear / m_rightArmwear（袖子）
│   ├── m_leftLegwear / m_rightLegwear（裤腿）
│   └── m_bodywear（外套）
├── 特殊部件
│   ├── m_cape（斗篷）
│   └── m_ears（耳朵，Deadmau5 皮肤专用）
└── 手臂姿态控制
    └── ArmPose 枚举（空手、持物品、拉弓、弩等）
```

## 上下游外部依赖关系

**本目录依赖：**
- `model/base/BipedModel.hpp` - 双足模型基类
- `model/core/ModelRenderer.hpp` - 模型渲染器（通过基类间接依赖）
- `common/entity/entities/player/PlayerModelPart.hpp` - 玩家皮肤部件枚举

**被依赖：**
- 玩家渲染器层（PlayerRenderer / PlayerLayer）使用 PlayerModel
- 皮肤预览界面、纸娃娃渲染等 UI 场景

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::player {
    class PlayerModel;
    // ArmPose 与 HandSide 复用基类 BipedModel 的枚举，通过 using 引入：
    //   using ArmPose  = mc::client::renderer::entity::model::ArmPose;
    //   using HandSide = mc::client::renderer::entity::model::HandSide;
}
```

## 容易踩的坑

### 纤细手臂模型差异

纤细手臂（Slim Arms）与标准手臂的尺寸和纹理坐标不同：
- 标准手臂：4x12x4，旋转点 Y=2.0
- 纤细手臂：3x12x4，旋转点 Y=2.5

创建 PlayerModel 时必须正确传入 `slimArms` 参数，否则手臂渲染位置会偏移。

### 外观层部件角度同步

调用 `setAngles()` 后必须调用 `copyAnglesToWear()` 将主部件角度复制到外观层，否则外层皮肤不会跟随动画。当前实现已在 `setAngles()` 中自动调用。

### 斗篷可见性控制

`setModelVisibilitiesFromFlags()` 不会设置斗篷（Cape）的可见性，斗篷由 `CapeLayer` 单独处理。如需控制斗篷可见性，需直接调用 `setPartVisible(PlayerModelPart::Cape, ...)`。

### 手臂单独渲染

`renderRightArm()` / `renderLeftArm()` 会临时修改可见性状态，渲染后自动恢复。但它们会重置手臂 X 轴旋转为 0（水平伸出），适用于皮肤预览等场景。

### ArmPose 枚举复用基类

`PlayerModel` 不再自定义 `ArmPose` 与 `HandSide` 枚举，而是通过 `using` 声明复用基类 `BipedModel` 的同名枚举。`setArmPose`/`setLeftArmPose`/`setRightArmPose` 直接写入基类字段 `m_leftArmPose`/`m_rightArmPose`，由 `BipedModel::setAngles → handleRightArmPose/handleLeftArmPose` 消费。这避免了字段隐藏导致姿态永远为 `Empty` 的问题。

### translateHand 多态与纤细手臂偏移

`PlayerModel::translateHand(HandSide, std::array<f64,16>&) const override` 重写基类 `BipedModel::translateHand`（基类方法已声明为 `virtual`），以支持纤细手臂偏移：

- **纤细手臂**：手臂宽度由 4 缩减为 3，手臂中心需向身体中线方向偏移 0.5 个模型单位（右手 X+0.5、左手 X-0.5），保持手持物品视觉上仍位于手臂中心。
- **无副作用实现**：临时修改手臂 `rotationPointX` 获取变换矩阵后立即恢复原值，参考 MC 1.21.11 `PlayerModel.translateToHand`。
- **多态调用**：`HeldItemLayer` 通过 `IEntityRenderer::getModel()` 获取模型并调用 `translateHand`，需要 `PlayerRenderer` 在注册 `HeldItemLayer` 时传入 `*this` 并显式指定 `TModel = PlayerModel`，多态分派才会生效。

### 手臂姿态动画进度

基类 `BipedModel::handleRightArmPose/handleLeftArmPose` 已对 BowAndArrow/CrossbowCharge/CrossbowHold 等姿态做了基础处理（固定角度）。若后续需要更精细的动画进度（如弩装填进度 0~1、弓拉弦进度），可在 `PlayerModel::_animateArms` 中扩展实现，并通过 override `handleRightArmPose/handleLeftArmPose` 或新增钩子集成。
