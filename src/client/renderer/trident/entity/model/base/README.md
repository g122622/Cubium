# 基础模型

本目录包含实体模型的基础模板类。

## 文件列表

| 文件 | 描述 |
|------|------|
| `BipedModel.hpp/cpp` | 双足模型基类 |
| `QuadrupedModel.hpp/cpp` | 四足模型基类 |

## BipedModel

双足动物模型基类，用于玩家、僵尸、骷髅等双足生物。

### 模型部件

```cpp
class BipedModel : public EntityModel {
protected:
    std::shared_ptr<ModelRenderer> m_bipedHead;      // 头部
    std::shared_ptr<ModelRenderer> m_bipedHeadwear;  // 帽子层（第二层）
    std::shared_ptr<ModelRenderer> m_bipedBody;      // 身体
    std::shared_ptr<ModelRenderer> m_bipedRightArm;  // 右臂
    std::shared_ptr<ModelRenderer> m_bipedLeftArm;   // 左臂
    std::shared_ptr<ModelRenderer> m_bipedRightLeg;  // 右腿
    std::shared_ptr<ModelRenderer> m_bipedLeftLeg;   // 左腿

    // 兼容性别名
    std::shared_ptr<ModelRenderer>& m_head = m_bipedHead;
    std::shared_ptr<ModelRenderer>& m_headwear = m_bipedHeadwear;
    std::shared_ptr<ModelRenderer>& m_body = m_bipedBody;
    std::shared_ptr<ModelRenderer>& m_rightArm = m_bipedRightArm;
    std::shared_ptr<ModelRenderer>& m_leftArm = m_bipedLeftArm;
    std::shared_ptr<ModelRenderer>& m_rightLeg = m_bipedRightLeg;
    std::shared_ptr<ModelRenderer>& m_leftLeg = m_bipedLeftLeg;
};
```

### 部件访问器

BipedModel 提供以下访问器方法获取模型部件：

```cpp
std::shared_ptr<ModelRenderer> getModelHead();      // 获取头部
std::shared_ptr<ModelRenderer> getModelHeadwear();  // 获取帽子层
std::shared_ptr<ModelRenderer> getModelBody();      // 获取身体
std::shared_ptr<ModelRenderer> getLeftArm();        // 获取左臂
std::shared_ptr<ModelRenderer> getRightArm();       // 获取右臂
std::shared_ptr<ModelRenderer> getLeftLeg();        // 获取左腿
std::shared_ptr<ModelRenderer> getRightLeg();       // 获取右腿
std::shared_ptr<ModelRenderer> getArmForSide(HandSide side); // 根据边获取手臂
```

### 可见性控制

```cpp
// 设置所有部件可见性
void setVisible(bool visible);

// 设置所有部件可见性（EntityModel 接口）
void setAllVisible(bool visible);

// 单个部件可见性
model.getModelHead()->setVisible(true);
model.getLeftArm()->setVisible(false);
```

### 盔甲槽位可见性（参考 MC 1.16.5 BipedArmorLayer.setModelSlotVisible）

```cpp
void setArmorSlotVisibility(BipedModel& model, ArmorSlot slot) {
    model.setAllVisible(false);
    
    switch (slot) {
        case ArmorSlot::Head:
            model.getModelHead()->setVisible(true);
            model.getModelHeadwear()->setVisible(true);
            break;
        case ArmorSlot::Chest:
            model.getModelBody()->setVisible(true);
            model.getLeftArm()->setVisible(true);
            model.getRightArm()->setVisible(true);
            break;
        case ArmorSlot::Legs:
            model.getModelBody()->setVisible(true);
            model.getLeftLeg()->setVisible(true);
            model.getRightLeg()->setVisible(true);
            break;
        case ArmorSlot::Feet:
            model.getLeftLeg()->setVisible(true);
            model.getRightLeg()->setVisible(true);
            break;
    }
}
```

### 纹理布局

标准玩家纹理布局（64x64）：

```
+--------+--------+
|  Head  |  Body  |  行 0-15
+--------+--------+
| Right  |  Left  |  行 16-31
|  Arm   |  Arm   |
+--------+--------+
| Right  |  Left  |  行 32-47
|  Leg   |  Leg   |
+--------+--------+
| Head   |  Body  |  行 48-63 (第二层)
| Layer  | Layer  |
+--------+--------+
```

### 使用方法

```cpp
class ZombieModel : public BipedModel {
public:
    ZombieModel() {
        // 设置纹理尺寸
        setTextureSize(64, 64);
        
        // 自定义部件（如果需要）
        m_rightArm->setRotation(-90.0f * DEG_TO_RAD, 0.0f, 0.0f);
        m_leftArm->setRotation(-90.0f * DEG_TO_RAD, 0.0f, 0.0f);
    }
    
    void setAngles(f64 limbSwing, ...) override {
        // 调用基类
        BipedModel::setAngles(limbSwing, ...);
        
        // 自定义动画
        // 僵尸手臂向前伸
    }
};
```

## QuadrupedModel

四足动物模型基类，用于猪、牛、羊等四足生物。

### 模型部件

```cpp
class QuadrupedModel : public EntityModel {
protected:
    std::shared_ptr<ModelRenderer> m_head;           // 头部
    std::shared_ptr<ModelRenderer> m_body;           // 身体
    std::shared_ptr<ModelRenderer> m_rightFrontLeg;  // 右前腿
    std::shared_ptr<ModelRenderer> m_leftFrontLeg;   // 左前腿
    std::shared_ptr<ModelRenderer> m_rightBackLeg;   // 右后腿
    std::shared_ptr<ModelRenderer> m_leftBackLeg;    // 左后腿
};
```

### 使用方法

```cpp
class PigModel : public QuadrupedModel {
public:
    PigModel() {
        setTextureSize(64, 32);
        
        // 设置头部
        m_head = std::make_shared<ModelRenderer>("head");
        m_head->setTextureOffset(0, 0)
               .addBox(-4.0f, -4.0f, -8.0f, 8, 8, 8);
        m_head->setRotationPoint(0.0f, 12.0f, -6.0f);
        m_parts.push_back(m_head);
        
        // 设置身体
        m_body = std::make_shared<ModelRenderer>("body");
        m_body->setTextureOffset(28, 8)
               .addBox(-5.0f, -10.0f, -7.0f, 10, 16, 8);
        m_body->setRotationPoint(0.0f, 11.0f, 2.0f);
        m_parts.push_back(m_body);
        
        // 设置腿部...
    }
};
```

## 动画参数

### 步态动画

```cpp
// 腿部动画
f32 legAngle = std::sin(limbSwing) * limbSwingAmount;
m_rightLeg->setRotateAngleX(legAngle);
m_leftLeg->setRotateAngleX(-legAngle);

// 手臂动画（双足）
m_rightArm->setRotateAngleX(legAngle);
m_leftArm->setRotateAngleX(-legAngle);
```

### 头部动画

```cpp
m_head->setRotateAngleX(headPitch * DEG_TO_RAD);
m_head->setRotateAngleY(netHeadYaw * DEG_TO_RAD);
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::model {
    class BipedModel;
    class QuadrupedModel;
}
```
