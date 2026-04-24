# 模型系统

本目录包含实体模型的定义和管理。

## 目录结构

```
model/
├── core/                   # 模型核心
│   ├── EntityModel.hpp     # 模型基类
│   ├── ModelRenderer.hpp   # 模型部件渲染
│   └── AgeableModel.hpp    # 可成长模型基类
├── base/                   # 基础模型
│   ├── BipedModel.hpp      # 双足模型
│   └── QuadrupedModel.hpp  # 四足模型
├── animal/                 # 动物模型
│   └── AnimalModels.hpp    # 猪、牛、羊、鸡模型
├── monster/                # 怪物模型
│   ├── ZombieModel.hpp     # 僵尸模型
│   ├── SkeletonModel.hpp   # 骷髅模型
│   ├── CreeperModel.hpp    # 苦力怕模型
│   ├── SpiderModel.hpp     # 蜘蛛模型
│   └── EndermanModel.hpp   # 末影人模型
├── player/                 # 玩家模型
│   └── PlayerModel.hpp     # 玩家模型
├── projectile/             # 投掷物模型
│   └── ProjectileModels.hpp
└── vehicle/                # 载具模型
    └── VehicleModels.hpp
```

## 核心类

### EntityModel

所有实体模型的基类，定义动画和渲染接口。

```cpp
class EntityModel {
public:
    virtual void render(f64 scale = 1.0f / 16.0f);
    virtual void generateMesh(std::vector<ModelVertex>& vertices,
                              std::vector<u32>& indices,
                              f64 scale) const;
    virtual void setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale);
    
    void setTextureSize(i32 width, i32 height);
    const std::vector<std::shared_ptr<ModelRenderer>>& getParts() const;
};
```

### ModelRenderer

模型部件类，代表模型的一个部分（如头部、身体、腿等）。

```cpp
class ModelRenderer {
public:
    ModelRenderer& addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth);
    ModelRenderer& setTextureOffset(i32 x, i32 y);
    void setRotation(f64 x, f64 y, f64 z);
    void setRotationPoint(f64 x, f64 y, f64 z);
    void addChild(std::shared_ptr<ModelRenderer> child);
    void generateMesh(std::vector<ModelVertex>& vertices, std::vector<u32>& indices) const;
};
```

### BipedModel

双足动物模型基类，用于玩家、僵尸、骷髅等。

```cpp
class BipedModel : public EntityModel {
protected:
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_headwear;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};
```

### QuadrupedModel

四足动物模型基类，用于猪、牛、羊等。

```cpp
class QuadrupedModel : public EntityModel {
protected:
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;
    std::shared_ptr<ModelRenderer> m_rightBackLeg;
    std::shared_ptr<ModelRenderer> m_leftBackLeg;
};
```

## 动画系统

动画参数说明：

| 参数 | 说明 |
|------|------|
| `limbSwing` | 步态动画周期（0-2π） |
| `limbSwingAmount` | 步态动画强度（0-1） |
| `ageInTicks` | 年龄 tick（用于空闲动画） |
| `netHeadYaw` | 头部偏航角（相对身体） |
| `headPitch` | 头部俯仰角 |
| `scale` | 缩放因子 |

### 动画参数计算

```cpp
// LivingRenderer 中的动画参数计算
f64 getLimbSwing(TEntity& entity, f64 partialTicks) const {
    f64 prevLimbSwing = entity.prevLimbSwing();
    f64 limbSwing = entity.limbSwing();
    return prevLimbSwing + (limbSwing - prevLimbSwing) * partialTicks;
}

f64 getHeadYaw(TEntity& entity, f64 partialTicks) const {
    f64 bodyYaw = entity.prevRenderYawOffset() + 
                  (entity.renderYawOffset() - entity.prevRenderYawOffset()) * partialTicks;
    f64 headYaw = entity.prevRotationYawHead() + 
                  (entity.rotationYawHead() - entity.prevRotationYawHead()) * partialTicks;
    return headYaw - bodyYaw;
}
```

## 纹理布局

实体纹理通常为 64x64 或 64x32 像素，按照特定布局排列：

```
+--------+--------+
|  Head  |  Body  |
+--------+--------+
| Right  |  Left  |
|  Arm   |  Arm   |
+--------+--------+
| Right  |  Left  |
|  Leg   |  Leg   |
+--------+--------+
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::model {
    class EntityModel;
    class ModelRenderer;
    struct ModelVertex;
    struct TexturedQuad;
    struct ModelBox;
}

namespace mc::client::renderer::entity::model::base {
    class BipedModel;
    class QuadrupedModel;
}

namespace mc::client::renderer::entity::model::animal {
    class PigModel;
    class CowModel;
    class SheepModel;
    class ChickenModel;
}
```
