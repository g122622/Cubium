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

### 基本功能

```cpp
class EntityRendererManager {
public:
    // 注册渲染器
    void registerRenderer(const std::string& typeId, RendererCreator creator);
    
    // 渲染实体
    void renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks);
    
    // 带视锥剔除的渲染
    bool renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks,
                           const mc::math::frustum::Frustum& frustum);
    
    // 网格管理
    EntityMesh* getOrCreateMesh(ClientEntity& entity);
    void updateMesh(ClientEntity& entity);
    void removeMesh(EntityId entityId);
};
```

### 相机信息传递

EntityRendererManager 负责将相机信息传递给 NameTagRenderer，用于名称标签的视锥剔除和背面剔除：

```cpp
class EntityRendererManager {
public:
    /**
     * @brief 设置相机信息（用于名称标签渲染）
     *
     * 必须在每帧渲染实体前调用，以便名称标签渲染器进行视锥剔除和背面剔除。
     *
     * @param position 相机世界位置
     * @param viewMatrix 视图矩阵
     * @param frustum 视锥体
     */
    void setCameraInfo(
        const glm::dvec3& position,
        const glm::mat4& viewMatrix,
        const mc::math::frustum::Frustum& frustum
    );
    
private:
    // 相机信息（用于名称标签渲染）
    glm::dvec3 m_cameraPosition{0.0, 0.0, 0.0};
    glm::mat4 m_viewMatrix{1.0f};
    mc::math::frustum::Frustum m_frustum;
    bool m_hasCameraInfo = false;
};
```

### 使用示例

```cpp
// 在实体渲染回调中设置相机信息
renderer->setEntityRenderCallback([this](VkCommandBuffer cmd, f64 partialTick) {
    const auto& frustum = renderer->frustum();
    const auto& frameContext = renderer->frameContext();

    // 设置相机信息给 EntityRendererManager
    if (frameContext.camera) {
        renderer->entityRendererManager().setCameraInfo(
            frameContext.camera->position(),
            frameContext.viewMatrix,
            frustum
        );
    }

    // 渲染实体（带视锥剔除）
    world.entityManager().forEachEntity([&](client::ClientEntity& entity) {
        renderer->entityRendererManager().renderWithPipeline(cmd, entity, partialTick, frustum);
    });
});
```

### 内部实现

setCameraInfo 方法内部会调用 NameTagRenderer：

```cpp
void EntityRendererManager::setCameraInfo(
    const glm::dvec3& position,
    const glm::mat4& viewMatrix,
    const mc::math::frustum::Frustum& frustum)
{
    m_cameraPosition = position;
    m_viewMatrix = viewMatrix;
    m_frustum = frustum;
    m_hasCameraInfo = true;

    // 更新 NameTagRenderer 的相机信息
    util::NameTagRenderer::setCameraPosition(Vector3d(position.x, position.y, position.z));
    // 转换视图矩阵为 double 数组
    std::array<f64, 16> viewMatrixArray;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            viewMatrixArray[i * 4 + j] = static_cast<f64>(viewMatrix[j][i]);
        }
    }
    util::NameTagRenderer::setViewMatrix(viewMatrixArray);
    util::NameTagRenderer::setFrustum(frustum);
}
```

### 性能优化

实体渲染管理器实现了以下性能优化：

1. **视锥剔除**: 使用 `Frustum::isAABBVisibleWorld()` 跳过视锥外实体的渲染
2. **名称标签优化**: 通过 `NameTagRenderer` 进行视锥剔除和背面剔除
3. **动画网格节流**: 通过 `AnimatedMeshCache` 按帧率节流网格更新

### 动画网格管理

动画实体通过 AnimatedMeshCache 管理网格更新：
1. 姿态切换（坐下/蹲伏/游泳/骑乘/幼体）立即更新
2. 活跃动画按 2 帧节流更新
3. 非活跃动画按 6 帧节流更新
4. 最多 12 帧强制刷新一次，防止状态漂移
5. CPU 网格保持 MC 模型单位，实体着色器统一用 push constant scale 应用 1/16 缩放

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
EntityRendererManager ─────► NameTagRenderer ─────► WorldTextRenderer
                                    │                      │
                                    │                      ▼
                                    │               Frustum (视锥剔除)
                                    │               CameraForward (背面剔除)
```

## 数据流

```
TridentEngine::frustum()
        │
        ▼
ClientApplicationSession (实体渲染回调)
        │
        ▼
EntityRendererManager::setCameraInfo()
        │
        ├──► NameTagRenderer::setCameraPosition()
        ├──► NameTagRenderer::setViewMatrix()
        └──► NameTagRenderer::setFrustum()
                │
                └──► WorldTextRenderer::setFrustum()
                        │
                        └──► shouldRenderText()
                                │
                                ├──► Frustum::isSphereVisible()
                                └──► isBackFacing()
```
