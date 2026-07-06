# 动物模型

本目录包含动物实体的模型实现。

## 目录结构

```
animal/
├── BatModel.hpp/cpp             # 蝙蝠模型
├── CatModel.hpp/cpp             # 猫模型
├── ChickenModel.hpp/cpp         # 鸡模型
├── CowModel.hpp/cpp             # 牛模型（含牛角；同时被哞菇复用）
├── HorseModel.hpp/cpp           # 马模型（含驴、骡、骷髅马、僵尸马）
├── LlamaModel.hpp/cpp           # 羊驼模型
├── OcelotModel.hpp/cpp          # 豹猫模型
├── PigModel.hpp/cpp             # 猪模型（在四足模型上追加猪鼻子）
├── PolarBearModel.hpp/cpp       # 北极熊模型
├── RabbitModel.hpp/cpp          # 兔子模型
├── SheepModel.hpp/cpp           # 羊模型（含吃草动画）
├── SquidModel.hpp/cpp           # 鱿鱼模型
├── VillagerModel.hpp/cpp        # 村民模型
├── WolfModel.hpp/cpp            # 狼模型
└── README.md
```

## 内部模块关系

所有动物模型均继承自 `core/` 和 `base/` 下的基类：

- `EntityModel` — 模型基类，定义动画和渲染接口
- `AgeableModel` — 可成长模型基类（支持成年/幼体渲染），继承自 EntityModel
- `QuadrupedModel` — 四足模型基类，继承自 AgeableModel
- `ModelRenderer` — 模型部件类，代表模型的一个部分

继承关系：
- PigModel、CowModel、SheepModel、HorseModel、WolfModel、OcelotModel、PolarBearModel → QuadrupedModel
- ChickenModel、LlamaModel、RabbitModel、CatModel、BatModel、SquidModel、VillagerModel → AgeableModel

## 上下游外部依赖关系

**上游依赖：**
- `core/EntityModel.hpp` — 模型基类
- `core/AgeableModel.hpp` — 可成长模型基类
- `core/ModelRenderer.hpp` — 模型部件渲染器
- `base/QuadrupedModel.hpp` — 四足模型基类

**下游使用：**
- `model/ModelRegistration.cpp` — 注册所有模型到 ModelFactory
- `renderer/animal/*Renderer.hpp/cpp` — 各动物渲染器使用对应模型
- `core/EntityRendererManager.cpp` — 实体渲染管理器
- `layer/entity/VillagerLayer.hpp` — 村民渲染层

## 容易踩的坑

1. **动画参数理解**：`limbSwing` 是步态动画周期（0-2π），`limbSwingAmount` 是步态强度（0-1），`ageInTicks` 用于空闲动画，这些参数由 LivingRenderer 计算并传入。

2. **SheepModel 羊毛状态**：必须调用 `setWool(bool)` 设置羊毛状态，否则可能显示错误。同时支持 `setEatingGrass()` 和 `setHeadRotation()` 用于吃草动画。

3. **PolarBearModel 站立动画**：需要调用 `setStandingProgress(f32)` 设置站立进度（0=四足，1=后腿站立），并调用 `setLivingAnimations()` 更新动画。

4. **LlamaModel 箱子装饰**：需要调用 `setHasChest(bool)` 设置是否装备箱子。

5. ** HorseModel 变体**：同一模型支持马、驴、骡、骷髅马、僵尸马，通过纹理区分。

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::animal
```
