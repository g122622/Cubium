# 动物渲染器

本目录包含动物实体的渲染器实现。

## 文件列表

| 文件 | 描述 |
|------|------|
| `AnimalRenderers.hpp` | 猪、牛、羊、鸡渲染器 |
| `LlamaRenderer.hpp/cpp` | 羊驼渲染器 |

## 渲染器类

### PigRenderer（猪渲染器）

```cpp
class PigRenderer : public LivingRenderer<LivingEntity, PigModel> {
public:
    PigRenderer() {
        m_shadowSize = 0.7f;
    }
};
```

**特性**：
- 阴影大小：0.7
- 可添加鞍层（用于骑乘）

### CowRenderer（牛渲染器）

```cpp
class CowRenderer : public LivingRenderer<LivingEntity, CowModel> {
public:
    CowRenderer() {
        m_shadowSize = 0.7f;
    }
};
```

**特性**：
- 阴影大小：0.7
- 可添加蘑菇层（哞菇）

### SheepRenderer（羊渲染器）

```cpp
class SheepRenderer : public LivingRenderer<LivingEntity, SheepModel> {
public:
    SheepRenderer() {
        m_shadowSize = 0.7f;
    }
    
    void render(Entity& entity, f64 partialTicks) override;
};
```

**特性**：
- 阴影大小：0.7
- 支持羊毛层渲染
- 可被剪毛

### ChickenRenderer（鸡渲染器）

```cpp
class ChickenRenderer : public LivingRenderer<LivingEntity, ChickenModel> {
public:
    ChickenRenderer() {
        m_shadowSize = 0.3f;
    }
};
```

**特性**：
- 阴影大小：0.3
- 翅膀拍动动画
- 喙和鸡冠渲染

### LlamaRenderer（羊驼渲染器）

```cpp
class LlamaRenderer : public EntityRenderer {
public:
    LlamaRenderer();
    
    void render(Entity& entity, f64 partialTicks) override;
    ResourceLocation getEntityTexture(LlamaEntity& entity);
    
private:
    LlamaModel m_model;
    LlamaModel m_modelBaby;
};

void registerLlamaRenderer(EntityRendererManager& manager);
```

**特性**：
- 阴影大小：0.7
- 支持 4 种颜色变体：Creamy、White、Brown、Gray
- 支持成年体和幼体渲染
- 支持箱子装饰显示（通过 LlamaModel::setHasChest）

**纹理选择逻辑**：
```cpp
static const char* colorNames[] = {"creamy", "white", "brown", "gray"};
i32 variant = static_cast<i32>(entity.getColor());
std::string textureName = "textures/entity/llama/" + colorNames[variant] + ".png";
```

## 使用方法

```cpp
// 注册动物渲染器
EntityRendererManager manager;
manager.registerRenderer("minecraft:pig", []() {
    return std::make_unique<PigRenderer>();
});
manager.registerRenderer("minecraft:cow", []() {
    return std::make_unique<CowRenderer>();
});
manager.registerRenderer("minecraft:sheep", []() {
    return std::make_unique<SheepRenderer>();
});
manager.registerRenderer("minecraft:chicken", []() {
    return std::make_unique<ChickenRenderer>();
});
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer::animal {
    class PigRenderer;
    class CowRenderer;
    class SheepRenderer;
    class ChickenRenderer;
    class LlamaRenderer;
}
```

## 参考

- MC 1.16.5 PigRenderer
- MC 1.16.5 CowRenderer
- MC 1.16.5 SheepRenderer
- MC 1.16.5 ChickenRenderer
- MC 1.16.5 LlamaRenderer
