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
│   ├── ZombieModel        → 僵尸，手臂前伸（含激怒状态攻击抬臂动画）
│   │   ├── HuskModel          → 尸壳（与僵尸结构相同）
│   │   ├── DrownedModel       → 溺尸（64x64 纹理）
│   │   ├── ZombieVillagerModel → 僵尸村民（64x64 纹理，带鼻子部件）
│   │   └── GiantModel         → 巨人（与僵尸结构相同，缩放更大）
│   ├── SkeletonModel      → 骷髅，细臂细腿，支持弓箭姿态
│   │   └── StrayModel         → 流浪者（与骷髅结构相同）
│   ├── EndermanModel      → 末影人，高瘦身材，长臂长腿
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

**重要**：`HuskModel`/`DrownedModel`/`ZombieVillagerModel`/`GiantModel` **继承自 `ZombieModel`**（而非直接继承 `BipedModel`），以共享 `ZombieModel::setAngles` 中的 `animateZombieArms` 攻击抬臂动画与 `setAggressive` 状态。对应 MC 1.21.11 中 `HuskModel`/`DrownedModel`/`ZombieVillagerModel`/`GiantModel` 均继承 `ZombieModel`。`StrayModel` 仍继承 `SkeletonModel`。

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
- **64x64**：普通僵尸、巨人、溺尸、僵尸村民
- **64x32**：骷髅、末影人、尸壳
- 苦力怕、蜘蛛等有各自独特的纹理布局

### 2. 模型状态同步

渲染前必须正确设置模型状态，否则动画不正确：
- `ZombieModel.setAggressive()` - 攻击状态（影响手臂动画，详见下文）
- `SkeletonModel.setRightArmPose()/setLeftArmPose()` - 手臂姿态（拉弓、持弩等），直接转发到基类 `BipedModel::m_rightArmPose/m_leftArmPose` 字段，由 `BipedModel::handleRightArmPose/handleLeftArmPose` 消费
- `EndermanModel.setCarrying()/setAttacking()` - 携带方块/尖叫状态
- `CreeperModel` 的充能状态通过 `renderArmor()` 单独渲染
- `GuardianModel.setSpikeAnimation()/setTailAnimation()` - 尖刺和尾巴动画
- `WitherModel.setSideHeadRotations(yaw0, pitch0, yaw1, pitch1)` - 注入两侧头独立朝向（度）。`yaw0/pitch0` 为左头（`m_heads[1]`），`yaw1/pitch1` 为右头（`m_heads[2]`）。调用后 `m_hasSideHeadRotations=true`，`setAngles()` 中侧头使用注入值（度→弧度转换）而非复制主头。主头（`m_heads[0]`）始终由 `netHeadYaw`/`headPitch` 参数驱动，不受此方法影响。对应 MC 1.21.11 `WitherBossModel.setupHeadRotation(state, head, index)`：`head.yRot = (yHeadRots[index] - bodyRot) * PI/180`、`head.xRot = xHeadRots[index] * PI/180`。调用时机：`setSideHeadRotations` 在 `setAngles` **之前**调用（仅存储），`setAngles` 时应用。未调用时回退到复制主头旋转，保持视觉一致。

#### ZombieModel.setAggressive 与 animateZombieArms

对应 MC 1.21.11 `AnimationUtils.animateZombieArms`。`ZombieModel::setAngles` 在 `BipedModel::setAngles` 之后按以下公式设置双臂角度：

```
f1 = -PI / (m_isAggressive ? 1.5 : 2.25)   // 手臂前伸基础角度
f2 = sin(swingProgress * PI)
f3 = sin((1 - (1-swingProgress)^2) * PI)
rightArm: zRot=0, yRot=-(0.1 - f2*0.6), xRot=f1 + f2*1.2 - f3*0.4
leftArm:  zRot=0, yRot= (0.1 - f2*0.6), xRot=f1 + f2*1.2 - f3*0.4
bobArms(rightArm, leftArm, ageInTicks)  // 无条件执行
```

- **`m_isAggressive=true`**：`f1 = -PI/1.5 ≈ -2.094`（抬臂更高，呈攻击姿态）
- **`m_isAggressive=false`**：`f1 = -PI/2.25 ≈ -1.396`（抬臂较低，呈自然站立姿态）
- **`swingProgress`**：通过 `f2`/`f3` 叠加攻击挥动因子，0 时无影响
- **`bobArms`**：无条件执行，根据 `ageInTicks` 添加手臂抖动偏移（`rightArm.zRot += cos(age*0.09)*0.05 + 0.05`、`rightArm.xRot += sin(age*0.067)*0.05`，左臂取反）

**数据流**：服务端 `MobEntity::setAggressive` → `DATA_MOB_FLAGS_PARAM` 位 2 → 客户端 `ClientEntity::syncMetadataFromDataManager` → `m_isAggressive` → `EntityRendererManager::_applyZombieState` → `ZombieModel::setAggressive` → `setAngles` 应用 `animateZombieArms` 公式。

**变体继承**：`HuskModel`/`DrownedModel`/`ZombieVillagerModel`/`GiantModel` 均继承 `ZombieModel`，自动复用 `setAggressive` 和 `setAngles` 中的攻击抬臂动画，无需各自重写。对应 MC 1.21.11 中这些变体模型也继承 `ZombieModel`。

**骷髅 ArmPose 管道**：`SkeletonModel` 不再自定义 `ArmPose` 枚举与 `m_rightArmPose/m_leftArmPose` 字段（避免遮蔽基类），改用 `using ArmPose = model::ArmPose;` 复用 `BipedModel::ArmPose`。弩姿态（`CrossbowCharge`/`CrossbowHold`）由基类 `handleCrossbowCharge/handleCrossbowHold` 完整处理，子类无需重复实现。`EntityRendererManager::_applySkeletonArmPose` 根据 `ClientEntity::isChargingBow()`（通过 `AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM` 同步）设置右臂 `BowAndArrow` 姿态。

**溺尸游泳动画管道**：`DrownedModel::setAngles` 覆盖 `ZombieModel::setAngles`，在 super 调用后执行两步覆盖（对应 MC 1.21.11 `DrownedModel.setupAnim`）：
1. **ThrowSpear 重应用**：`animateZombieArms`（在 `ZombieModel::setAngles` 中）会覆盖 `BipedModel::handleRightArmPose` 已设置的 ThrowSpear 手臂角度，故 DrownedModel 在 super 返回后重新应用：`rightArm.xRot = rightArm.xRot * 0.5 - PI`、`rightArm.yRot = 0`（左臂同理）。ArmPose 由 `EntityRendererManager::_applyDrownedTridentPose` 在 `_applyZombieState` 之前根据 `ClientEntity::isAggressive()` 设置（对应 MC `DrownedRenderer.getArmPose`：`isAggressive && holdsTrident → THROW_TRIDENT`；本项目以 isAggressive 作为信号，待主手装备同步落地后恢复完整判定）。
2. **游泳覆盖**（`swimAmount > 0` 时）：手臂 X `rotLerpRad(f, cur, -4π/5) ± 0.35*f*sin(0.1*age)`（划水摆动）、手臂 Z `rotLerpRad(f, cur, ∓0.15)`（略微内收/外展）、腿部 X 叠加 `∓0.55*f*sin(0.1*age)`（打水摆动）、头部 X 归零（平视前方）。`swimAmount` 由 `EntityRendererManager::_createModelForEntity` 从 `ClientEntity::getInterpolatedSwimAmount(partialTicks)` 填入 `AnimationContext.swimAmount`，再经 `_applyZombieState` 推送 `setSwimAnimation`。服务端 `DrownedEntity::updateSwimming` 按 `areEyesInWater && isInWater && wantsToSwim && !isRiding` 设置 `EntityFlags::Swimming` 位，经 `DATA_FLAGS_PARAM` 同步到客户端，客户端 `LivingEntity`/`ClientEntity` 每帧 `updateSwimAmount` 渐变（±0.09/tick）。

**渲染器层游泳身体倾斜（未实现）**：MC 1.21.11 `DrownedRenderer.setupRotations` 在 `swimAmount > 0` 时将整个实体身体绕 X 轴倾斜 `lerp(swimAmount, 0, -10 - xRot)` 度（枢轴位于包围盒垂直中心）。本项目实体矩阵构建集中在 `EntityRendererManager`（无 `setupRotations` 虚函数钩子，且 `AnimationContext` 缺少 `xRot`/`boundingBoxHeight` 字段），该效果暂留作 TODO（见 `MonsterVariantRenderers.hpp` 中 DrownedRenderer 类注释）。

**凋灵侧头朝向管道**：`EntityRendererManager::_createModelForEntity` 中对凋灵分支读取 `ClientEntity::getInterpolatedWitherSideHeadYaw/Pitch(index, partialTick)`，通过 `math::wrapDegrees(absoluteYaw - bodyYaw)` 将绝对 yaw 转为 body 相对值（对齐 MC `yHeadRots[index] - bodyRot`），再调用 `WitherModel::setSideHeadRotations()` 注入。详见 `src/client/world/entity/README.md` 的凋灵侧头朝向章节。

### 3. 变体模型复用

部分变体模型直接继承基础模型，仅改变纹理或缩放：
- `CaveSpiderModel` 继承 `SpiderModel`，渲染时缩放 0.7 倍
- `StrayModel` 继承 `SkeletonModel`，结构与骷髅相同
- `HuskModel`/`DrownedModel`/`ZombieVillagerModel`/`GiantModel` 继承 `ZombieModel`，共享 `animateZombieArms` 攻击抬臂动画与 `setAggressive` 状态（纹理尺寸差异：DrownedModel/ZombieVillagerModel=64x64，HuskModel/GiantModel=64x64/64x32）
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
