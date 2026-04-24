# 怪物模型

本目录包含怪物实体的模型实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `ZombieModel.hpp/cpp` | 僵尸模型 |
| `SkeletonModel.hpp/cpp` | 骷髅模型 |
| `CreeperModel.hpp/cpp` | 苦力怕模型 |
| `SpiderModel.hpp/cpp` | 蜘蛛模型 |
| `EndermanModel.hpp/cpp` | 末影人模型 |

## 模型详解

### ZombieModel（僵尸模型）

继承自 `BipedModel`，僵尸是双足生物，手臂向前伸。

**特点**：
- 纹理尺寸：64x64
- 手臂向前伸 90 度
- 攻击时有摆动动画

**参考**：MC 1.16.5 ZombieModel

### SkeletonModel（骷髅模型）

继承自 `BipedModel`，骷髅是双足生物，手臂和腿更细。

**特点**：
- 纹理尺寸：64x32
- 手臂尺寸：2x12x2（玩家是 4x12x4）
- 腿部尺寸：2x12x2（玩家是 4x12x4）
- 支持弓箭姿态

**手臂姿态**：
```cpp
enum class ArmPose {
    Empty,          // 空手
    BowAndArrow,    // 拉弓
    ThrowSpear,     // 投掷三叉戟
    CrossbowCharge, // 装填弩
    CrossbowHold    // 持有弩
};
```

**参考**：MC 1.16.5 SkeletonModel

### CreeperModel（苦力怕模型）

继承自 `EntityModel`，苦力怕有独特的四足结构。

**部件**：
- 头部：8x8x8，位置 (0, 6, 0)
- 身体：8x12x4，位置 (0, 6, 0)
- 四条腿：4x6x4，各位置不同

**动画**：
- 腿部交替摆动：`cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount`
- 对角腿同步

**参考**：MC 1.16.5 CreeperModel

### SpiderModel（蜘蛛模型）

继承自 `EntityModel`，蜘蛛有 8 条腿。

**部件**：
- 头部：8x8x8，位置 (0, 15, -3)
- 颈部：6x6x6，位置 (0, 15, 0)
- 身体：10x8x12，位置 (0, 15, 9)
- 8 条腿：16x2x2

**腿部动画**：
- Z 轴旋转：基础角度 + 摆动
- Y 轴旋转：基础角度 + 摆动

**参考**：MC 1.16.5 SpiderModel

### EndermanModel（末影人模型）

继承自 `BipedModel`，末影人身材高大，手臂和腿很长。

**特点**：
- 纹理尺寸：64x32
- Y 偏移：-14（比普通生物高）
- 手臂长度：30（玩家是 12）
- 腿部长度：30（玩家是 12）
- 手臂限制：±0.4 弧度

**部件**：
- 头部：8x8x8，纹理 (0, 0)，旋转点 (0, -14, 0)
- 头套：8x8x8，纹理 (0, 16)，旋转点 (0, -14, 0)
- 身体：8x12x4，纹理 (32, 16)，旋转点 (0, -14, 0)
- 右臂：2x30x2，纹理 (56, 0)，旋转点 (-3, -12, 0)
- 左臂：2x30x2，纹理 (56, 0)，旋转点 (5, -12, 0)，镜像
- 右腿：2x30x2，纹理 (56, 0)，旋转点 (-2, -2, 0)
- 左腿：2x30x2，纹理 (56, 0)，旋转点 (2, -2, 0)，镜像

**特殊状态**：
- `isCarrying`：携带方块时手臂姿态
- `isAttacking`：攻击/尖叫时头部下移

**动画**：
- 手臂和腿角度限制在 ±0.4 弧度
- 携带方块：手臂 X=-0.5, Z=±0.05
- 尖叫：头部 Y 位置 -5

**参考**：MC 1.16.5 EndermanModel

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::monster {
    class ZombieModel;
    class SkeletonModel;
    class CreeperModel;
    class SpiderModel;
    class EndermanModel;
}
```
