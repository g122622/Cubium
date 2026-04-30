# 核心渲染器

本目录包含实体渲染系统的核心组件。

## 文件列表

| 文件 | 描述 |
|------|------|
| `IEntityRenderer.hpp` | 实体渲染器接口，将渲染器与模型类型解耦 |
| `EntityRenderer.hpp/cpp` | 实体渲染器基类，定义渲染接口 |
| `LivingRenderer.hpp` | 生物渲染器模板类，支持层渲染器系统 |
| `EntityRendererManager.hpp/cpp` | 渲染器管理器，管理所有实体渲染器 |
| `AnimatedMeshCache.hpp/cpp` | 动画网格缓存，按状态变化节流更新 GPU 网格 |
| `AgeableModel.hpp/cpp` | 可成长模型基类，支持幼体/成年状态 |

## 类图

```
┌─────────────────┐
│  EntityRenderer │ (基类)
│  ───────────────│
│  + render()     │
│  + renderShadow()│
│  + renderNameTag()│
└────────┬────────┘
         │
┌────────▼────────┐
│ LivingRenderer  │ (模板类)
│ ─────────────── │
│ + addLayer()    │
│ + renderLayers()│
└────────┬────────┘
         │
┌────────▼────────────────────────────────┐
│ IEntityRenderer<TEntity, TModel>        │
│ ─────────────────────────────────────── │
│ + getModel()                             │
│ + getEntityTexture()                     │
└─────────────────────────────────────────┘
```

## IEntityRenderer 接口

实体渲染器接口，将渲染器与模型类型解耦，使层渲染器可以访问模型。

```cpp
template<typename TEntity, typename TModel>
class IEntityRenderer {
public:
    virtual ~IEntityRenderer() = default;
    virtual TModel& getModel() = 0;
    virtual const TModel& getModel() const = 0;
    virtual ResourceLocation getEntityTexture(TEntity& entity) = 0;
};
```

## EntityRenderer 基类

所有实体渲染器的基类，提供阴影渲染和名称标签渲染的基础实现。

```cpp
class EntityRenderer {
public:
    virtual void render(Entity& entity, f64 partialTicks) = 0;
    virtual void renderShadow(Entity& entity, f64 partialTicks);
    virtual void renderNameTag(Entity& entity);
    
    f64 shadowSize() const;
    void setShadowSize(f64 size);
};
```

## LivingRenderer 模板

生物渲染器模板类，支持层渲染器系统和动画参数计算。

```cpp
template<typename TEntity, typename TModel>
class LivingRenderer : public EntityRenderer,
                        public IEntityRenderer<TEntity, TModel> {
public:
    // 添加层渲染器
    template<typename TLayer, typename... TArgs>
    void addLayer(TArgs&&... args);
    
protected:
    // 动画参数计算
    f64 getLimbSwing(TEntity& entity, f64 partialTicks) const;
    f64 getLimbSwingAmount(TEntity& entity, f64 partialTicks) const;
    f64 getHeadYaw(TEntity& entity, f64 partialTicks) const;
    f64 getHeadPitch(TEntity& entity, f64 partialTicks) const;
    
    // 渲染所有层
    void renderLayers(TEntity& entity, ...);
};
```

## EntityRendererManager

管理所有实体渲染器，根据实体类型分派渲染。

```cpp
class EntityRendererManager {
public:
    // 注册渲染器
    void registerRenderer(const String& typeId, RendererCreator creator);
    
    // 渲染实体
    void renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks);
    
    // 网格管理
    EntityMesh* getOrCreateMesh(ClientEntity& entity);
    void updateMesh(ClientEntity& entity);
    void removeMesh(EntityId entityId);
};

// 动画实体通过 AnimatedMeshCache 管理网格更新：
// 1. 姿态切换（坐下/蹲伏/游泳/骑乘/幼体）立即更新
// 2. 活跃动画按 2 帧节流更新
// 3. 非活跃动画按 6 帧节流更新
// 4. 最多 12 帧强制刷新一次，防止状态漂移
// 5. CPU 网格保持 MC 模型单位，实体着色器统一用 push constant scale 应用 1/16 缩放
```

## AgeableModel

可成长模型基类，支持幼体和成年两种状态。

```cpp
class AgeableModel : public EntityModel {
public:
    void setChild(bool isChild);
    bool isChild() const;
    f64 getChildScale(f64 baseScale) const;
    
protected:
    bool m_isChild = false;
    f64 m_childHeadScale = 2.0f;   // 幼体头部缩放
    f64 m_childBodyScale = 0.5f;   // 幼体身体缩放
};
```

## 依赖关系

```
IEntityRenderer ◄────────── LivingRenderer ◄─────── 具体渲染器
       │                            │
       │                            │
       ▼                            ▼
  EntityRenderer              LayerRenderer
       │
       ▼
EntityRendererManager
```
