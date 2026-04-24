# 实体渲染系统

实体渲染系统负责渲染游戏中的所有实体，包括玩家、生物、怪物、物品实体等。

## 目录结构

```
entity/
├── core/                   # 核心渲染器
│   ├── IEntityRenderer.hpp # 实体渲染器接口
│   ├── EntityRenderer.hpp  # 实体渲染器基类
│   ├── LivingRenderer.hpp  # 生物渲染器模板
│   ├── EntityRendererManager.hpp # 渲染器管理器
│   └── AgeableModel.hpp    # 可成长模型基类
├── pipeline/               # 渲染管线
│   ├── EntityPipeline.hpp  # Vulkan渲染管线
│   └── EntityTextureAtlas.hpp # 实体纹理图集
├── util/                   # 工具类
│   ├── ShadowRenderer.hpp  # 阴影渲染器
│   └── NameTagRenderer.hpp # 名称标签渲染器
├── renderer/               # 具体渲染器
│   ├── animal/             # 动物渲染器
│   ├── monster/            # 怪物渲染器
│   ├── player/             # 玩家渲染器
│   ├── projectile/         # 投掷物渲染器
│   └── vehicle/            # 载具渲染器
├── model/                  # 模型系统
│   ├── core/               # 模型核心
│   ├── base/               # 基础模型
│   ├── player/             # 玩家模型
│   ├── animal/             # 动物模型
│   ├── monster/            # 怪物模型
│   ├── projectile/         # 投掷物模型
│   └── vehicle/            # 载具模型
├── layer/                  # 层渲染器系统
│   ├── core/               # 层渲染器核心
│   ├── equipment/          # 装备层
│   ├── cosmetic/           # 外观层
│   ├── entity/             # 实体特性层
│   └── effect/             # 效果层
└── effect/                 # 特效系统
    ├── glow/               # 发光效果
    ├── fire/               # 着火效果
    └── hurt/               # 受伤效果
```

## 核心组件

### IEntityRenderer 接口

实体渲染器接口，将渲染器与模型类型解耦。

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

### EntityRenderer

所有实体渲染器的基类，定义渲染接口。

### LivingRenderer

生物渲染器模板类，支持层渲染器系统。

```cpp
template<typename TEntity, typename TModel>
class LivingRenderer : public EntityRenderer,
                        public IEntityRenderer<TEntity, TModel> {
public:
    template<typename TLayer, typename... TArgs>
    void addLayer(TArgs&&... args);
};
```

### LayerRenderer

层渲染器基类，用于在基础模型上添加额外渲染层（盔甲、鞍等）。

```cpp
template<typename TEntity>
class LayerRenderer {
public:
    virtual void render(TEntity& entity, f32 limbSwing, ...) = 0;
    virtual bool shouldRender(const TEntity& entity) const { return true; }
};
```

### AgeableModel

可成长模型基类，支持幼体和成年两种状态。

```cpp
class AgeableModel : public EntityModel {
public:
    void setChild(bool isChild);
    f64 getChildScale(f64 baseScale) const;
};
```

## 命名空间

```cpp
namespace mc::client::renderer::entity {
    namespace core { }      // 核心渲染器
    namespace pipeline { }  // 渲染管线
    namespace model { }     // 模型系统
    namespace layer { }     // 层渲染器
    namespace effect { }    // 特效系统
    
    namespace renderer {
        namespace animal { }    // 动物渲染器
        namespace monster { }   // 怪物渲染器
        namespace player { }    // 玩家渲染器
        namespace projectile { } // 投掷物渲染器
        namespace vehicle { }   // 载具渲染器
    }
}
```

## 使用方法

### 创建自定义渲染器

```cpp
class MyRenderer : public LivingRenderer<MyEntity, MyModel> {
public:
    MyRenderer() {
        m_shadowSize = 0.5f;
        // 添加层渲染器
        addLayer<MyLayer>(*this);
    }
};
```

### 创建自定义层渲染器

```cpp
class MyLayer : public LayerRenderer<MyEntity> {
public:
    void render(MyEntity& entity, f32 limbSwing, ...) override {
        // 渲染额外层
    }
};
```

### 创建可成长实体

```cpp
class MyAgeableModel : public AgeableModel {
public:
    void render(f64 scale) override {
        // 根据幼体状态调整渲染
        AgeableModel::render(scale);
    }
};

// 在渲染器中设置幼体状态
void MyRenderer::render(Entity& entity, f64 partialTicks) {
    m_model.setChild(entity.isChild());
    LivingRenderer::render(entity, partialTicks);
}
```
