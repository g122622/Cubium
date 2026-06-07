# 基础模型

本目录包含实体模型的基础模板类，为具体实体模型提供通用骨架结构。

## 目录结构

```
base/
├── BipedModel.hpp           # 双足模型基类（玩家、僵尸、骷髅等）
├── BipedModel.cpp           # 双足模型实现
├── QuadrupedModel.hpp       # 四足模型基类（猪、牛、羊等）
└── QuadrupedModel.cpp       # 四足模型实现
```

## 内部模块关系

```
┌─────────────────────────────────────────────────────────────┐
│                         base/                               │
│  ┌─────────────────┐      ┌───────────────────┐            │
│  │   BipedModel    │      │  QuadrupedModel   │            │
│  │ (双足模型基类)   │      │  (四足模型基类)    │            │
│  └────────┬────────┘      └────────┬──────────┘            │
│           │                        │                        │
│           └──────────┬─────────────┘                        │
│                      │                                      │
│                      ▼                                      │
│           ┌─────────────────────┐                           │
│           │    AgeableModel     │  (core/ 可成长模型基类)    │
│           │  (支持幼体/成年体)   │                           │
│           └──────────┬──────────┘                           │
│                      │                                      │
│                      ▼                                      │
│           ┌─────────────────────┐                           │
│           │    EntityModel      │  (core/ 实体模型基类)      │
│           │  (模型渲染接口)      │                           │
│           └─────────────────────┘                           │
└─────────────────────────────────────────────────────────────┘
```

## 上下游外部依赖关系

### 本目录依赖的外部模块

| 模块 | 用途 |
|------|------|
| `core/AgeableModel` | 父类，提供幼体/成年体缩放支持 |
| `core/ModelRenderer` | 模型部件渲染器 |
| `common/util/math/Vector3` | 数学向量类型 |

### 依赖本目录的外部模块

| 模块 | 用途 |
|------|------|
| `model/player/PlayerModel` | 玩家模型，继承 BipedModel |
| `model/monster/ZombieModel` | 僵尸模型，继承 BipedModel |
| `model/monster/SkeletonModel` | 骷髅模型，继承 BipedModel |
| `model/monster/EndermanModel` | 末影人模型，继承 BipedModel |
| `model/animal/AnimalModels` | 猪、牛、羊模型，继承 QuadrupedModel |
| `model/aquatic/AquaticModels` | 水生生物模型 |
| `layer/equipment/ArmorLayer` | 盔甲渲染层，使用 BipedModel 部件可见性 |
| `layer/equipment/HeldItemLayer` | 手持物品渲染层，使用 translateHand 方法 |

## 容易踩的坑

### BipedModel 部件命名别名

BipedModel 有两套部件命名：`m_bipedXxx`（原名）和 `m_xxx`（别名引用）。子类应使用 `m_head`、`m_body` 等简短别名，避免直接访问 `m_bipedHead` 等原名称。

### translateHand 矩阵变换顺序

`translateHand()` 的变换顺序为：平移到旋转点 → Z轴旋转 → Y轴旋转 → X轴旋转。HeldItemLayer 组合物品变换时需遵循此顺序。

### 幼体模型缩放

AgeableModel 父类通过 `m_isChild` 控制幼体渲染，幼体会对头部和身体应用不同的缩放矩阵。子类构造时需正确传入 `childHeadScale`、`childBodyScale` 等参数，否则幼体渲染会错位。

### 盔甲槽位可见性

BipedModel 的盔甲槽位可见性设置需参考 MC 1.16.5 `BipedArmorLayer.setModelSlotVisible`：
- Head 槽位：显示头部 + 帽子层
- Chest 槽位：显示身体 + 双臂
- Legs 槽位：显示身体 + 双腿
- Feet 槽位：显示双腿

### 动画参数来源

`setAngles()` 的参数由 LivingRenderer 计算：
- `limbSwing`：步态周期（插值后的 prevLimbSwing → limbSwing）
- `netHeadYaw`：头部偏航角（rotationYawHead - renderYawOffset）
- 不要在模型内部重新计算这些值，应直接使用传入参数

### 命名空间

所有模型类位于 `mc::client::renderer::entity::model` 命名空间（注意：不是 `model::base` 子命名空间）。
