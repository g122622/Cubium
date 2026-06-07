# 模型系统

本目录包含实体模型的定义和管理。

## 目录结构

```
model/
├── core/                       # 模型核心
│   ├── EntityModel.hpp         # 模型基类，定义动画和渲染接口
│   ├── ModelRenderer.hpp       # 模型部件类，代表模型的一个部分
│   ├── AgeableModel.hpp        # 可成长模型基类（幼体/成年）
│   ├── SegmentedModel.hpp      # 分段模型基类（用于复杂实体如末影龙）
│   └── ModelFactory.hpp        # 模型工厂，注册表模式管理模型创建
├── base/                       # 基础模型基类
│   ├── BipedModel.hpp          # 双足模型基类（玩家、僵尸、骷髅等）
│   └── QuadrupedModel.hpp      # 四足模型基类（猪、牛、羊等）
├── animal/                     # 动物模型
│   ├── AnimalModels.hpp        # 猪、牛、羊、鸡模型（待拆分）
│   ├── WolfModel.hpp           # 狼模型
│   ├── HorseModel.hpp          # 马模型
│   ├── LlamaModel.hpp          # 羊驼模型
│   ├── OcelotModel.hpp         # 豹猫模型
│   ├── CatModel.hpp            # 猫模型
│   ├── RabbitModel.hpp         # 兔子模型
│   ├── PolarBearModel.hpp      # 北极熊模型
│   ├── SquidModel.hpp          # 鱿鱼模型
│   ├── BatModel.hpp            # 蝙蝠模型
│   └── VillagerModel.hpp       # 村民模型
├── monster/                    # 怪物模型
│   ├── ZombieModel.hpp         # 僵尸模型
│   ├── SkeletonModel.hpp       # 骷髅模型
│   ├── CreeperModel.hpp        # 苦力怕模型
│   ├── SpiderModel.hpp         # 蜘蛛模型
│   ├── EndermanModel.hpp       # 末影人模型
│   ├── BlazeModel.hpp          # 烈焰人模型
│   ├── MonsterVariantModels.hpp # 怪物变体模型
│   ├── MoreMonsterModels.hpp   # 更多怪物模型
│   └── SpecialMonsterModels.hpp # 特殊怪物模型
├── player/                     # 玩家模型
│   └── PlayerModel.hpp         # 玩家模型
├── projectile/                 # 投掷物模型
│   └── ProjectileModels.hpp    # 投掷物模型集合
├── aquatic/                    # 水生生物模型
│   ├── AquaticModels.hpp       # 水生生物模型集合
│   └── PufferfishModel.hpp     # 河豚模型
├── nether/                     # 下界生物模型
│   └── NetherModels.hpp        # 下界生物模型集合
└── ModelRegistration.hpp       # 模型注册初始化入口
```

## 内部模块关系

```
EntityModel (基类)
    ├── AgeableModel (可成长模型，支持幼体/成年)
    │     ├── BipedModel (双足模型)
    │     │     ├── PlayerModel
    │     │     ├── ZombieModel
    │     │     └── SkeletonModel 等
    │     ├── QuadrupedModel (四足模型)
    │     │     ├── PigModel
    │     │     ├── CowModel
    │     │     └── SheepModel 等
    │     └── ChickenModel 等
    └── SegmentedModel (分段模型)
          └── EnderDragonModel 等

ModelRenderer：模型部件类，被所有模型类组合使用

ModelFactory：模型工厂，统一创建所有实体模型实例
```

## 上下游外部依赖关系

**上游依赖（本目录依赖的模块）：**
- `common/core/Types.hpp` - 基础类型定义（i32, f32, f64 等）
- `common/util/math/Vector2.hpp`, `Vector3.hpp` - 数学向量类型

**下游依赖（依赖本目录的模块）：**
- `entity/renderer/` - 所有实体渲染器通过 ModelFactory 创建模型
- `entity/layer/` - 层渲染器（HeldItemLayer, ArmorLayer 等）访问模型部件
- `entity/core/LivingRenderer.hpp` - 生物渲染器模板持有模型实例
- `firstperson/PlayerModel.hpp` - 第一人称玩家模型
- `blockentity/model/` - 方块实体模型复用 ModelRenderer

## 容易踩的坑

1. **幼体模型缩放**：AgeableModel 的 `m_isChild` 默认为 `true`，新建模型实例时需显式调用 `setChild(false)` 设置为成年状态，否则会按幼体缩放渲染

2. **纹理尺寸**：不同模型使用不同的纹理尺寸（64x32 或 64x64），在 `addBox` 前必须确保 `setTextureSize` 正确设置，否则 UV 坐标会错误

3. **镜像模式**：ModelRenderer 的 `setMirror(true)` 会影响顶点顺序和 UV 映射，用于左右对称部件（如左臂/右臂），忘记设置会导致渲染异常

4. **动画参数单位**：`limbSwing` 是弧度制的步态周期（0-2π），`limbSwingAmount` 是 0-1 的插值系数，`netHeadYaw` 和 `headPitch` 是弧度制角度

5. **translateHand 变换顺序**：BipedModel::translateHand 的变换顺序是 平移→Z轴旋转→Y轴旋转→X轴旋转，与直觉可能相反，在 HeldItemLayer 中计算手持物品位置时需注意

6. **ModelFactory 注册**：新模型必须在 `ModelRegistration.cpp` 中通过 `REGISTER_ENTITY_MODEL` 宏注册，否则 ModelFactory::createModel 会返回 nullptr

7. **ModelRenderer::render 已废弃**：项目已改用 GPU 管线路径，应使用 `generateMesh()` 生成网格数据，然后通过 EntityPipeline 提交到 GPU，不应再调用 `render()` 方法

8. **AnimalModels.hpp 待拆分**：该文件包含多个模型类（PigModel, CowModel, SheepModel, ChickenModel），每个模型应拆分为独立文件
