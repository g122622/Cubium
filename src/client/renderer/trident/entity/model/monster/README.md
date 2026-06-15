# 怪物模型

本目录包含怪物实体的模型实现。

## 目录结构

```
monster/
├── ZombieModel.hpp                  # 僵尸模型（继承 BipedModel）
├── ZombieModel.cpp
├── SkeletonModel.hpp                # 骷髅模型（继承 BipedModel，支持弓箭姿态）
├── SkeletonModel.cpp
├── CreeperModel.hpp                 # 苦力怕模型（四足结构，含充能盔甲层）
├── CreeperModel.cpp
├── SpiderModel.hpp                  # 蜘蛛模型（8条腿）
├── SpiderModel.cpp
├── EndermanModel.hpp                # 末影人模型（继承 BipedModel，高瘦身材）
├── EndermanModel.cpp
├── BlazeModel.hpp                   # 烈焰人模型（漂浮头部 + 12根烟雾棒）
├── BlazeModel.cpp
├── MonsterVariantModels.hpp         # 怪物变体模型（僵尸村民、溺尸、流浪者、尸壳、洞穴蜘蛛、巨人）
├── MonsterVariantModels.cpp
├── MoreMonsterModels.hpp            # 更多怪物模型（灾厄村民、恼鬼、铁傀儡、雪傀儡、蜜蜂、狐狸、熊猫、鹦鹉、幻翼、劫掠兽）
├── MoreMonsterModels.cpp
├── SpecialMonsterModels.hpp         # 特殊怪物模型（凋灵、史莱姆、守卫者、远古守卫者、潜影贝、蠹虫、末影螨）
├── SpecialMonsterModels.cpp
├── WitchModel.hpp                   # 女巫模型（继承 VillagerModel，含帽子、鼻子动画）
├── WitchModel.cpp
└── README.md
```

## 内部模块关系

### 继承体系

```
EntityModel（基类，定义于 model/core/）
├── BipedModel（双足模型，定义于 model/base/）
│   ├── ZombieModel        → 僵尸，手臂前伸
│   ├── SkeletonModel      → 骷髅，细臂细腿，支持弓箭姿态
│   ├── EndermanModel      → 末影人，高瘦身材，长臂长腿
│   ├── ZombieVillagerModel → 僵尸村民（带鼻子部件）
│   ├── DrownedModel       → 溺尸
│   ├── StrayModel         → 流浪者（与骷髅结构相同）
│   ├── HuskModel          → 尸壳（与僵尸结构相同）
│   ├── GiantModel         → 巨人（与僵尸结构相同，缩放更大）
│   ├── VexModel           → 恼鬼（带翅膀）
│   └── IllagerModel       → 灾厄村民基类
├── SpiderModel
│   └── CaveSpiderModel    → 洞穴蜘蛛（复用蜘蛛模型，缩放0.7倍）
├── CreeperModel           → 苦力怕（独立实现，四足）
├── BlazeModel             → 烈焰人（独立实现，漂浮结构）
├── IronGolemModel         → 铁傀儡（独立实现）
├── SnowGolemModel         → 雪傀儡（独立实现）
├── BeeModel               → 蜜蜂（独立实现）
├── FoxModel               → 狐狸（独立实现）
├── PandaModel             → 熊猫（独立实现）
├── ParrotModel            → 鹦鹉（独立实现）
├── PhantomModel           → 幻翼（独立实现）
├── RavagerModel           → 劫掠兽（独立实现）
├── WitherModel            → 凋灵（独立实现，3个头）
├── SlimeModel             → 史莱姆（独立实现）
├── GuardianModel          → 守卫者（独立实现）
│   └── ElderGuardianModel → 远古守卫者（复用守卫者模型）
├── ShulkerModel           → 潜影贝（独立实现）
├── SilverfishModel        → 蠹虫（独立实现）
├── EndermiteModel         → 末影螨（独立实现）
└── WitchModel             → 女巫（继承 VillagerModel，分层帽子+鼻子动画）
```

### 模型分类

| 分类 | 模型 | 特点 |
|------|------|------|
| 双足类 | Zombie, Skeleton, Enderman 等 | 继承 BipedModel，复用双足动画 |
| 四足类 | Creeper | 独立实现四足动画 |
| 节肢类 | Spider, Silverfish, Endermite | 多腿/分节身体 |
| 特殊类 | Blaze, Guardian, Shulker 等 | 独特结构，需单独实现动画 |

## 上下游外部依赖关系

### 上游依赖（本目录依赖的模块）

```
model/core/
├── EntityModel.hpp      # 模型基类
└── ModelRenderer.hpp    # 模型部件渲染

model/base/
└── BipedModel.hpp       # 双足模型基类
```

### 下游依赖（依赖本目录的模块）

```
renderer/monster/
├── MonsterRenderers.hpp       # 基础怪物渲染器（Zombie, Skeleton, Creeper, Spider, Enderman, Blaze）
├── MonsterVariantRenderers.hpp # 变体渲染器（僵尸村民、溺尸、流浪者、尸壳、洞穴蜘蛛）
├── SpecialMonsterRenderers.hpp # 特殊怪物渲染器（凋灵、史莱姆、守卫者等）
└── MoreMonsterRenderers.hpp   # 更多怪物渲染器（灾厄村民、铁傀儡等）

layer/
├── effect/EyesLayer.cpp       # 发光眼睛层（蜘蛛、末影人）
├── effect/EnergyGlintLayer.cpp # 附魔光效层（充能苦力怕）
└── entity/HeldBlockLayer.cpp  # 手持方块层（末影人）
```

## 容易踩的坑

### 1. 纹理尺寸差异

不同怪物使用不同的纹理尺寸，设置错误会导致纹理错乱：
- **64x64**：普通僵尸、巨人
- **64x32**：骷髅、末影人、尸壳、溺尸
- 苦力怕、蜘蛛等有各自独特的纹理布局

### 2. 模型状态同步

渲染前必须正确设置模型状态，否则动画不正确：
- `ZombieModel.setAggressive()` - 攻击状态（影响手臂动画）
- `SkeletonModel.setRightArmPose()/setLeftArmPose()` - 手臂姿态（拉弓、持弩等）
- `EndermanModel.setCarrying()/setAttacking()` - 携带方块/尖叫状态
- `CreeperModel` 的充能状态通过 `renderArmor()` 单独渲染
- `GuardianModel.setSpikeAnimation()/setTailAnimation()` - 尖刺和尾巴动画

### 3. 变体模型复用

部分变体模型直接继承基础模型，仅改变纹理或缩放：
- `CaveSpiderModel` 继承 `SpiderModel`，渲染时缩放 0.7 倍
- `StrayModel`/`HuskModel`/`GiantModel` 继承 `BipedModel`，结构与骷髅/僵尸相同
- `ElderGuardianModel` 继承 `GuardianModel`，仅纹理不同

### 4. BipedModel 基类接口

继承 `BipedModel` 的模型会自动获得：
- `m_head`, `m_body`, `m_rightArm`, `m_leftArm`, `m_rightLeg`, `m_leftLeg` 部件引用
- `setSwingProgress()` - 挥动动画进度
- `setSneaking()/setSitting()` - 蹲伏/坐下状态
- `translateHand()` - 手持物品渲染时获取手臂变换矩阵

### 5. 动画参数含义

`setAngles()` 参数说明：
- `limbSwing` - 步态周期（0-2π 循环）
- `limbSwingAmount` - 步态强度（0-1，静止为0）
- `ageInTicks` - 年龄 tick（用于空闲动画，如手臂抖动）
- `netHeadYaw` - 头部偏航角（相对身体）
- `headPitch` - 头部俯仰角

### 6. 模型部件命名

子类使用基类部件时的别名：
```cpp
// BipedModel 中的别名引用
std::shared_ptr<ModelRenderer>& m_head = m_bipedHead;
std::shared_ptr<ModelRenderer>& m_rightArm = m_bipedRightArm;
// ...
```
子类可直接使用 `m_head`, `m_rightArm` 等简短名称。

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::monster {
    class ZombieModel;
    class SkeletonModel;
    class CreeperModel;
    class SpiderModel;
    class EndermanModel;
    class BlazeModel;
    // 变体模型
    class ZombieVillagerModel;
    class DrownedModel;
    class StrayModel;
    class HuskModel;
    class CaveSpiderModel;
    class GiantModel;
    // 更多怪物模型
    class IllagerModel;
    class VexModel;
    class IronGolemModel;
    class SnowGolemModel;
    class BeeModel;
    class FoxModel;
    class PandaModel;
    class ParrotModel;
    class PhantomModel;
    class RavagerModel;
    // 特殊怪物模型
    class WitherModel;
    class SlimeModel;
    class GuardianModel;
    class ElderGuardianModel;
    class ShulkerModel;
    class SilverfishModel;
    class EndermiteModel;
    // 女巫模型
    class WitchModel;
}
```
