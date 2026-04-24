# 动物模型

本目录包含动物实体的模型实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `AnimalModels.hpp/cpp` | 猪、牛、羊、鸡模型 |

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
