# 玩家模型

本目录包含玩家实体的模型实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `PlayerModel.hpp/cpp` | 玩家模型 |

## 模型详解

### PlayerModel（玩家模型）

继承自 `BipedModel`，支持标准手臂和纤细手臂两种模式。

**特点**：
- 纹理尺寸：64x64（包含外层）
- 支持标准手臂（4x12x4）和纤细手臂（3x12x4）
- 包含外层皮肤渲染

**部件**：

#### 基础部件（继承自 BipedModel）

| 部件 | 尺寸 | 纹理位置 | 旋转点 |
|------|------|----------|--------|
| 头部 | 8x8x8 | (0, 0) | (0, 0, 0) |
| 头套 | 8x8x8 | (32, 0) | (0, 0, 0) |
| 身体 | 8x12x4 | (16, 16) | (0, 0, 0) |
| 右臂 | 4x12x4 或 3x12x4 | (40, 16) | (-5, 2, 0) |
| 左臂 | 4x12x4 或 3x12x4 | (32, 48) | (5, 2, 0) |
| 右腿 | 4x12x4 | (0, 16) | (-1.9, 12, 0) |
| 左腿 | 4x12x4 | (16, 48) | (1.9, 12, 0) |

#### 外观层部件

| 部件 | 尺寸 | 纹理位置 | 膨胀 |
|------|------|----------|------|
| 右臂外层 | 同主部件 | (40, 32) | +0.25 |
| 左臂外层 | 同主部件 | (48, 48) | +0.25 |
| 右腿外层 | 4x12x4 | (0, 32) | +0.25 |
| 左腿外层 | 4x12x4 | (0, 48) | +0.25 |
| 身体外层 | 8x12x4 | (16, 32) | +0.25 |

#### 特殊部件

| 部件 | 尺寸 | 纹理位置 | 说明 |
|------|------|----------|------|
| 斗篷 | 10x16x1 | (0, 0) | 斗篷渲染，纹理尺寸 64x32 |
| 耳朵 | 6x6x1 | (24, 0) | Deadmau5 皮肤专用 |

**手臂姿态**：

```cpp
enum class ArmPose {
    Empty,          // 空手
    Item,           // 持有物品
    Block,          // 格挡
    BowAndArrow,    // 拉弓
    ThrowSpear,     // 投掷三叉戟
    CrossbowCharge, // 装填弩
    CrossbowHold    // 持有弩
};
```

**特殊状态**：
- `crouching` - 蹲伏状态，影响斗篷位置
- `swimming` - 游泳状态
- `sprinting` - 疾跑状态

**斗篷位置调整**：

```cpp
// 无胸甲蹲伏
cape.rotationPointZ = 1.4F;
cape.rotationPointY = 1.85F;

// 无胸甲站立
cape.rotationPointZ = 0.0F;
cape.rotationPointY = 0.0F;

// 穿胸甲蹲伏
cape.rotationPointZ = 0.3F;
cape.rotationPointY = 0.8F;

// 穿胸甲站立
cape.rotationPointZ = -1.1F;
cape.rotationPointY = -0.85F;
```

**参考**：MC 1.16.5 PlayerModel

## 手臂单独渲染

### renderRightArm / renderLeftArm

用于第三人称视角的手臂渲染（如玩家皮肤预览、纸娃娃等）。

```cpp
// 渲染右手臂（仅手臂和袖子）
playerModel->renderRightArm(1.0 / 16.0);

// 渲染左手臂（仅手臂和袖子）
playerModel->renderLeftArm(1.0 / 16.0);
```

**实现逻辑**（参考 MC 1.16.5 PlayerRenderer.renderItem）：

1. 保存当前可见性状态
2. 隐藏所有部件 (`setVisible(false)`)
3. 仅显示目标手臂和袖子
4. 重置手臂 X 轴旋转角度 (`rotateAngleX = 0.0F`)
5. 渲染内层皮肤（手臂）
6. 渲染外层皮肤（袖子）
7. 恢复原始可见性状态

**特点**：
- 渲染时手臂水平伸出（X 轴旋转归零）
- 同时渲染内层和外层皮肤
- 自动恢复可见性状态，不影响后续渲染

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::player {
    class PlayerModel;
    enum class ArmPose;
}
```

## 使用示例

```cpp
// 创建标准手臂玩家模型
auto playerModel = std::make_shared<PlayerModel>(0.0f, false);

// 创建纤细手臂玩家模型
auto slimModel = std::make_shared<PlayerModel>(0.0f, true);

// 设置手臂姿态
playerModel->setArmPose(ArmPose::Empty, ArmPose::BowAndArrow);

// 渲染
playerModel->setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
playerModel->render(scale);

// 渲染斗篷
playerModel->renderCape(scale);

// 渲染耳朵（Deadmau5 皮肤）
playerModel->renderEars(scale);
```

## 依赖关系

```
PlayerModel.hpp
├── base/BipedModel.hpp
└── ModelRenderer.hpp

PlayerModel.cpp
├── PlayerModel.hpp
└── <cmath>
```
