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
    virtual void renderPipeline(TEntity& entity, VkCommandBuffer cmd,
        const AnimationContext& context, EntityPipeline& pipeline) = 0;
    virtual bool shouldRender(const TEntity& entity) const { return true; }
};
```

主要层渲染器：

| 层渲染器 | 类型 | 描述 |
|---------|------|------|
| HeldItemLayer | equipment | 手持物品渲染 |
| HeadLayer | equipment | 头部物品（头盔、南瓜）渲染 |
| ArmorLayer | equipment | 盔甲渲染 |
| CapeLayer | cosmetic | 披风渲染 |
| ElytraLayer | cosmetic | 鞘翅渲染 |
| SaddleLayer | entity | 鞍渲染 |
| EnergyGlintLayer | effect | 附魔光效 |

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

## 实体渲染决策流程

实体渲染器根据实体类型和可用数据选择渲染方式，决策顺序如下：

```
1. ItemEntity Billboard 渲染
   ↓ (非 ItemEntity)
2. ModelFactory 动画模型渲染
   ↓ (无模型)
3. PipelineMeshProvider 自定义网格渲染
   ↓ (无 Provider)
4. 默认渲染（空/错误标记）
```

### 渲染决策详解

| 优先级 | 渲染方式 | 适用实体 | 描述 |
|--------|----------|----------|------|
| 1 | ItemEntity Billboard | 物品实体 | 使用物品纹理的 Billboard 渲染 |
| 2 | ModelFactory 动画模型 | 大多数生物 | 通过 ModelFactory 创建的动画模型 |
| 3 | PipelineMeshProvider | 投掷物等 | 自定义网格渲染器 |

### PipelineMeshProvider 接口

用于提供自定义渲染网格的接口，适用于投掷物、特殊效果等不需要完整模型的实体。

```cpp
class PipelineMeshProvider {
public:
    virtual ~PipelineMeshProvider() = default;
    
    // 获取渲染网格
    virtual MeshData getMesh(const Entity& entity, f32 partialTicks) = 0;
    
    // 获取纹理区域
    virtual TextureRegion getTexture() const = 0;
    
    // 是否应该渲染
    virtual bool shouldRender(const Entity& entity) const { return true; }
};
```

### 新增渲染器

| 渲染器 | 实体类型 | 渲染方式 | 描述 |
|--------|----------|----------|------|
| SnowballRenderer | 雪球 | Billboard | 投掷物 Billboard 渲染 |
| EggRenderer | 鸡蛋 | Billboard | 投掷物 Billboard 渲染 |
| EnderPearlRenderer | 末影珍珠 | Billboard | 投掷物 Billboard 渲染 |
| PotionRenderer | 药水 | Billboard | 投掷物 Billboard 渲染 |
| ExperienceBottleRenderer | 附魔之瓶 | Billboard | 投掷物 Billboard 渲染 |
| EyeOfEnderRenderer | 末影之眼 | Billboard + Particle | 带粒子效果的投掷物 |
| FireballRenderer | 火球 | Billboard | 大火球渲染 |
| SmallFireballRenderer | 小火球 | Billboard | 小火球渲染（烈焰人火球等） |
| FishingBobberRenderer | 钓鱼浮漂 | Billboard + Line | 带线渲染的浮漂 |

### PufferfishRenderer 河豚状态切换

河豚渲染器根据河豚的膨胀状态动态切换模型：

```cpp
class PufferfishRenderer : public LivingRenderer<Pufferfish, PufferfishModel> {
public:
    PufferfishRenderer() {
        // 根据膨胀状态选择模型
        // puffState: 0 = 未膨胀, 1 = 中等膨胀, 2 = 完全膨胀
    }
    
    PufferfishModel& getModel(Pufferfish& entity) override {
        i32 puffState = entity.getPuffState();
        switch (puffState) {
            case 0: return m_smallModel;    // 未膨胀
            case 1: return m_mediumModel;   // 中等膨胀
            case 2: return m_largeModel;    // 完全膨胀
        }
    }
};
```

### BlendMode::Lines 线条渲染

用于 `LINE_LIST` 拓扑的特殊混合模式：

```cpp
enum class BlendMode {
    None,       // 无混合
    Alpha,      // Alpha 混合
    Additive,   // 加法混合
    Lines,      // 线条渲染（LINE_LIST 拓扑）
};
```

`BlendMode::Lines` 特性：
- 使用 `LINE_LIST` 图元拓扑
- 适用于 FishingBobberRenderer 的钓鱼线
- 不写入深度缓冲区
- 支持颜色混合

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
