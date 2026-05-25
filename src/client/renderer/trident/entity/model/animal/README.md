# 动物模型

本目录包含动物实体的模型实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `AnimalModels.hpp/cpp` | 猪、牛、羊、鸡模型 |
| `BatModel.hpp/cpp` | 蝙蝠模型 |
| `CatModel.hpp/cpp` | 猫模型 |
| `HorseModel.hpp/cpp` | 马模型（含驴、骡、骷髅马、僵尸马） |
| `LlamaModel.hpp/cpp` | 羊驼模型 |
| `OcelotModel.hpp/cpp` | 豹猫模型 |
| `PolarBearModel.hpp/cpp` | 北极熊模型 |
| `RabbitModel.hpp/cpp` | 兔子模型 |
| `SquidModel.hpp/cpp` | 鱿鱼模型 |
| `VillagerModel.hpp/cpp` | 村民模型 |
| `WolfModel.hpp/cpp` | 狼模型 |

## 模型类

### PigModel（猪模型）

```cpp
class PigModel : public QuadrupedModel {
public:
    PigModel();
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;
};
```

**纹理布局**（64x32）：
- 头部：0,0（8x8x8）
- 身体：28,8（10x16x8）
- 腿部：各 6x8

### CowModel（牛模型）

```cpp
class CowModel : public QuadrupedModel {
public:
    CowModel();
    void setAngles(...) override;
};
```

**纹理布局**（64x32）：
- 头部：0,0（8x8x8）
- 身体：18,4（12x18x10）
- 腿部：各 4x12

### SheepModel（羊模型）

```cpp
class SheepModel : public QuadrupedModel {
public:
    SheepModel();
    void setAngles(...) override;
    
    void setWool(bool hasWool);  // 设置是否有羊毛
    
private:
    bool m_hasWool = true;
};
```

**纹理布局**（64x32）：
- 头部：0,0（8x8x8）
- 身体：28,8（12x18x10）
- 腿部：各 4x12
- 羊毛：单独纹理层

### ChickenModel（鸡模型）

```cpp
class ChickenModel : public EntityModel {
public:
    ChickenModel();
    void render(f64 scale) override;
    void setAngles(...) override;
    
private:
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_beak;    // 喙
    std::shared_ptr<ModelRenderer> m_wattle;  // 肉垂
    std::shared_ptr<ModelRenderer> m_comb;    // 鸡冠
};
```

**纹理布局**（64x32）：
- 头部：0,0（2x2x2）
- 喙：14,0（2x1x1）
- 肉垂：14,1（1x1x1）
- 鸡冠：14,2（1x1x1）
- 身体：8,8（4x6x3）
- 翅膀：各 4x3
- 腿部：各 1x3

### LlamaModel（羊驼模型）

```cpp
class LlamaModel : public AgeableModel {
public:
    explicit LlamaModel(f32 scale = 0.0f);
    
    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;
    
    void setHasChest(bool hasChest);  // 设置是否装备箱子
    
protected:
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;
    
private:
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_backRightLeg;
    std::shared_ptr<ModelRenderer> m_backLeftLeg;
    std::shared_ptr<ModelRenderer> m_frontRightLeg;
    std::shared_ptr<ModelRenderer> m_frontLeftLeg;
    std::shared_ptr<ModelRenderer> m_chest1;  // 左侧箱子
    std::shared_ptr<ModelRenderer> m_chest2;  // 右侧箱子
};
```

**纹理布局**（128x64）：
- 头部主体：0,0（4x4x9）
- 头部延伸：0,14（8x18x6）
- 耳朵：17,0（3x3x2）x2
- 身体：29,0（12x18x10）
- 腿部：29,29（4x14x4）x4
- 箱子：45,28（8x8x3）和 45,41（8x8x3）

**颜色变体**：
- Creamy（奶油色）：textures/entity/llama/creamy.png
- White（白色）：textures/entity/llama/white.png
- Brown（棕色）：textures/entity/llama/brown.png
- Gray（灰色）：textures/entity/llama/gray.png

**特性**：
- 继承 AgeableModel，支持成年体和幼体渲染
- 支持箱子装饰显示（成年且有箱子时）
- 箱子部件使用身体作为父部件

### PolarBearModel（北极熊模型）

```cpp
class PolarBearModel : public QuadrupedModel {
public:
    PolarBearModel();
    
    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;
    
    void setStandingProgress(f32 standingProgress);
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount,
                             f64 partialTick) override;
    
protected:
    void setupParts() override;
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;
    
private:
    f32 m_standingProgress = 0.0f;
};
```

**纹理布局**（128x64）：
- 头部主体：0,0（7x7x7）
- 鼻子：0,44（5x3x3）
- 耳朵：26,0（2x2x1）x2
- 身体上部：0,19（14x14x11）
- 身体下部：39,0（12x12x10）
- 后腿：50,22（4x10x8）x2
- 前腿：50,40（4x10x6）x2

**站立动画**：
- 参考 MC 1.16.5 PolarBearModel.setRotationAngles
- 站立进度范围 [0, 1]，0 为四足站立，1 为后腿站立
- 身体旋转：PI/2 - progress * PI * 0.35
- 前腿移动：Y 和 Z 偏移，X 旋转增加
- 头部移动：成年/幼体有不同的 Y 和 Z 偏移

**特性**：
- 继承 QuadrupedModel，支持四足动物基础动画
- 支持成年体和幼体渲染（AgeableModel）
- 支持后腿站立动画
- 纹理与 MC 1.16.5 完全一致

## 动画特性

### 四足动物步态

```cpp
// 四足动物的腿部交替动画
m_rightFrontLeg->setRotateAngleX(std::sin(limbSwing) * limbSwingAmount);
m_leftFrontLeg->setRotateAngleX(-std::sin(limbSwing) * limbSwingAmount);
m_rightBackLeg->setRotateAngleX(-std::sin(limbSwing) * limbSwingAmount);
m_leftBackLeg->setRotateAngleX(std::sin(limbSwing) * limbSwingAmount);
```

### 鸡的翅膀动画

```cpp
// 翅膀拍动
f32 wingAngle = std::sin(ageInTicks * 0.3f) * 0.2f;
m_rightWing->setRotateAngleZ(wingAngle);
m_leftWing->setRotateAngleZ(-wingAngle);
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::model::animal {
    class PigModel;
    class CowModel;
    class SheepModel;
    class ChickenModel;
}
```

## 参考

- MC 1.16.5 PigModel
- MC 1.16.5 CowModel
- MC 1.16.5 SheepModel
- MC 1.16.5 ChickenModel
