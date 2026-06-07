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
    enum class ArmPose;
    enum class HandSide;
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

### translateHand 签名问题

当前 `translateHand(i32 side)` 的签名与基类 `BipedModel::translateHand(HandSide, array<f64,16>&)` 不一致，需要后续统一。

### 未完成的动画实现

以下私有方法已定义但尚未被调用：
- `_animateBow()` - 弓箭姿态动画
- `_animateCrossbowCharge()` - 弩装填动画
- `_animateCrossbowHold()` - 弩持有动画
- `_updateCapePosition()` - 斗篷位置动态调整

这些功能需要等待手臂姿态动画系统和斗篷动画系统集成后才能启用。
