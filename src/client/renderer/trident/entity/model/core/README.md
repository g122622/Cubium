# 模型核心

本目录包含实体模型系统的核心类。

## 文件列表

| 文件 | 描述 |
|------|------|
| `EntityModel.hpp/cpp` | 实体模型基类 |
| `ModelRenderer.hpp/cpp` | 模型部件渲染器 |
| `AgeableModel.hpp/cpp` | 可成长模型基类 |

## EntityModel

所有实体模型的基类，定义动画和渲染接口。

```cpp
class EntityModel {
public:
    EntityModel();
    virtual ~EntityModel() = default;
    
    // 渲染
    virtual void render(f64 scale = 1.0f / 16.0f);
    virtual void generateMesh(std::vector<ModelVertex>& vertices,
                              std::vector<u32>& indices,
                              f64 scale) const;
    
    // 动画
    virtual void setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale);
    virtual void copyAnglesTo(EntityModel& target) const;
    virtual void copyAnglesFrom(const EntityModel& source);
    
    // 纹理
    i32 textureWidth() const;
    i32 textureHeight() const;
    void setTextureSize(i32 width, i32 height);
    
    // 部件访问
    const std::vector<std::shared_ptr<ModelRenderer>>& getParts() const;
    
protected:
    i32 m_textureWidth = 64;
    i32 m_textureHeight = 32;
    std::vector<std::shared_ptr<ModelRenderer>> m_parts;
};
```

## ModelRenderer

模型部件类，代表模型的一个部分（如头部、身体、腿等）。

### 主要功能

- 添加立方体盒子
- 设置旋转和位置
- 管理子部件
- 生成网格（盒子局部顶点按 `scale` 从 MC 1/16 模型单位转换，旋转点同样按 `scale` 平移）

```cpp
class ModelRenderer {
public:
    explicit ModelRenderer(const String& name = "");
    
    // 纹理
    void setTextureSize(i32 width, i32 height);
    ModelRenderer& setTextureOffset(i32 offsetX, i32 offsetY);
    
    // 变换
    void setOffset(f64 x, f64 y, f64 z);
    void setRotationPoint(f64 x, f64 y, f64 z);
    void setRotation(f64 x, f64 y, f64 z);
    void setScale(f64 x, f64 y, f64 z);
    
    // 盒子
    ModelRenderer& addBox(f64 x, f64 y, f64 z, f64 width, f64 height, f64 depth);
    ModelRenderer& addBox(i32 texX, i32 texY, f64 x, f64 y, f64 z,
                         f64 width, f64 height, f64 depth);
    
    // 镜像
    void setMirror(bool mirror);
    
    // 子部件
    void addChild(std::shared_ptr<ModelRenderer> child);
    std::shared_ptr<ModelRenderer> createChild(const String& name = "");
    
    // 网格生成
    void generateMesh(std::vector<ModelVertex>& vertices,
                     std::vector<u32>& indices,
                     f64 scale) const;
    void generateMesh(std::vector<ModelVertex>& vertices,
                     std::vector<u32>& indices,
                     const std::array<f64, 16>& parentMatrix,
                     f64 scale) const;
    
    // 可见性
    bool isVisible() const;
    void setVisible(bool visible);
    
    // 旋转访问器
    f64 rotateAngleX() const;
    f64 rotateAngleY() const;
    f64 rotateAngleZ() const;
    void setRotateAngleX(f64 angle);
    void setRotateAngleY(f64 angle);
    void setRotateAngleZ(f64 angle);
    
    // 复制旋转
    void copyModelAngles(const ModelRenderer& other);
};
```

## AgeableModel

可成长模型基类，支持幼体和成年两种状态。

```cpp
class AgeableModel : public EntityModel {
public:
    void setChild(bool isChild);
    bool isChild() const;
    void generateMesh(std::vector<ModelVertex>& vertices,
                      std::vector<u32>& indices,
                      f64 scale) const override;
    
protected:
    bool m_isChild = false;
    f32 m_childHeadScale = 2.0f;
    f32 m_childBodyScale = 2.0f;
    f32 m_childHeadOffsetY = 5.0f;
    f32 m_childBodyOffsetY = 24.0f;
};
```

## 数据结构

### ModelVertex

模型顶点，包含位置、纹理坐标和法线。

```cpp
struct ModelVertex {
    Vector3f position;   // 顶点位置
    Vector2f texCoord;   // UV 坐标
    Vector3f normal;     // 法线
};
```

### TexturedQuad

纹理四边形，代表一个四边形面。

```cpp
struct TexturedQuad {
    std::array<ModelVertex, 4> vertices;
    Vector3f normal;
};
```

### ModelBox

模型盒子，每个盒子有6个面。

```cpp
struct ModelBox {
    f64 posX1, posY1, posZ1;  // 最小角
    f64 posX2, posY2, posZ2;  // 最大角
    std::array<TexturedQuad, 6> quads;  // 6个面
};
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::model {
    class EntityModel;
    class ModelRenderer;
    class AgeableModel;
    struct ModelVertex;
    struct TexturedQuad;
    struct ModelBox;
}
```
