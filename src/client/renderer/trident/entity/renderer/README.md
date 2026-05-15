# 渲染器

本目录包含具体实体类型的渲染器实现。

## 目录结构

```
renderer/
├── animal/                 # 动物渲染器
│   └── AnimalRenderers.hpp # 猪、牛、羊、鸡渲染器
├── monster/                # 怪物渲染器
│   ├── ZombieRenderer.hpp  # 僵尸渲染器
│   ├── SkeletonRenderer.hpp # 骷髅渲染器
│   ├── CreeperRenderer.hpp # 苦力怕渲染器
│   ├── SpiderRenderer.hpp  # 蜘蛛渲染器
│   └── EndermanRenderer.hpp # 末影人渲染器
├── player/                 # 玩家渲染器
│   └── PlayerRenderer.hpp  # 玩家渲染器
├── projectile/             # 投掷物渲染器
│   ├── ItemEntityRenderer.hpp # 物品实体渲染器
│   └── ExperienceOrbRenderer.hpp # 经验球渲染器
└── vehicle/                # 载具渲染器
    ├── BoatRenderer.hpp    # 船渲染器
    └── MinecartRenderer.hpp # 矿车渲染器
```

## 渲染器类

### 动物渲染器 (animal/)

```cpp
// 猪渲染器
class PigRenderer : public LivingRenderer<LivingEntity, PigModel> {
public:
    PigRenderer() { m_shadowSize = 0.5f; }
};

// 牛渲染器
class CowRenderer : public LivingRenderer<LivingEntity, CowModel> {
public:
    CowRenderer() { m_shadowSize = 0.7f; }
};

// 羊渲染器
class SheepRenderer : public LivingRenderer<LivingEntity, SheepModel> {
public:
    SheepRenderer() { m_shadowSize = 0.7f; }
};

// 鸡渲染器
class ChickenRenderer : public LivingRenderer<LivingEntity, ChickenModel> {
public:
    ChickenRenderer() { m_shadowSize = 0.3f; }
};
```

### 投掷物渲染器 (projectile/)

```cpp
// 物品实体渲染器
class ItemEntityRenderer : public EntityRenderer {
public:
    void render(Entity& entity, f64 partialTicks) override;
    void setItemTextureAtlas(EntityTextureAtlas* atlas);
};

// 经验球渲染器
class ExperienceOrbRenderer : public EntityRenderer {
public:
    void render(Entity& entity, f64 partialTicks) override;
};
```

### 玩家渲染器 (player/)

玩家渲染器支持标准手臂和纤细手臂两种模式，并集成了多个层渲染器。

```cpp
class PlayerRenderer : public EntityRenderer,
                        public IEntityRenderer<Player, PlayerModel> {
public:
    explicit PlayerRenderer(bool slimArms = false);
    
    // IEntityRenderer 接口
    PlayerModel& getModel() override;
    ResourceLocation getEntityTexture(Player& entity) override;
    
    // 纹理设置
    void setSkinTexture(const TextureRegion* region);
    void setCapeTexture(const TextureRegion* region);
    void setElytraTexture(const TextureRegion* region);
    
    // 层渲染器支持
    bool supportsLayers() const override { return true; }
    void renderLayersPipeline(Entity& entity, VkCommandBuffer cmd,
        const AnimationContext& context, EntityPipeline& pipeline) override;
    
private:
    void setupLayers();  // 初始化层渲染器
};
```

#### 层渲染器配置

MC 1.16.5 PlayerRenderer 按以下顺序设置层渲染器：

```cpp
void PlayerRenderer::setupLayers() {
    // 1. 手持物品层
    m_layers.push_back(std::make_unique<HeldItemLayer<Player>>());
    
    // 2. 头部物品层（头盔、南瓜等）
    m_layers.push_back(std::make_unique<HeadLayer<Player, PlayerModel>>(*this));
    
    // 3. 披风层
    m_layers.push_back(std::make_unique<CapeLayer>());
    
    // 4. 鞘翅层
    m_layers.push_back(std::make_unique<ElytraLayer<Player>>());
}
```

#### 纹理传递

在 `renderLayersPipeline()` 中，纹理通过 `dynamic_cast` 传递给对应的层渲染器：

```cpp
void PlayerRenderer::renderLayersPipeline(...) {
    for (auto& layer : m_layers) {
        if (layer && layer->shouldRender(player)) {
            // 传递披风纹理
            if (m_capeRegion) {
                auto* capeLayer = dynamic_cast<CapeLayer*>(layer.get());
                if (capeLayer) capeLayer->setCapeTexture(m_capeRegion);
            }
            // 传递鞘翅纹理
            if (m_elytraRegion || m_capeRegion) {
                auto* elytraLayer = dynamic_cast<ElytraLayer<Player>*>(layer.get());
                if (elytraLayer) {
                    if (m_elytraRegion) elytraLayer->setElytraTexture(m_elytraRegion);
                    if (m_capeRegion) elytraLayer->setCapeTexture(m_capeRegion);
                }
            }
            layer->renderPipeline(player, cmd, context, pipeline);
        }
    }
}
```

#### 参考文件
- `PlayerRenderer.hpp/cpp` - 玩家渲染器实现
- `tests/client/renderer/entity/test_player_layers.cpp` - 层渲染器单元测试

### 怪物渲染器 (monster/)

```cpp
// 僵尸渲染器
class ZombieRenderer : public LivingRenderer<ZombieEntity, ZombieModel> {
public:
    ZombieRenderer();
};

// 骷髅渲染器
class SkeletonRenderer : public LivingRenderer<SkeletonEntity, SkeletonModel> {
public:
    SkeletonRenderer();
};

// 苦力怕渲染器
class CreeperRenderer : public LivingRenderer<CreeperEntity, CreeperModel> {
public:
    CreeperRenderer();
};

// 蜘蛛渲染器
class SpiderRenderer : public LivingRenderer<SpiderEntity, SpiderModel> {
public:
    SpiderRenderer();
};

// 末影人渲染器
class EndermanRenderer : public LivingRenderer<EndermanEntity, EndermanModel> {
public:
    EndermanRenderer();
};
```

## 渲染器注册

```cpp
void EntityRendererManager::initializeDefaults() {
    registerRenderer("minecraft:pig", []() {
        return std::make_unique<PigRenderer>();
    });
    registerRenderer("minecraft:cow", []() {
        return std::make_unique<CowRenderer>();
    });
    // ...
}
```

## 命名空间

```cpp
namespace mc::client::renderer::entity::renderer {
    namespace animal {
        class PigRenderer;
        class CowRenderer;
        class SheepRenderer;
        class ChickenRenderer;
    }
    namespace projectile {
        class ItemEntityRenderer;
        class ExperienceOrbRenderer;
    }
    namespace player {
        class PlayerRenderer;
    }
    namespace monster {
        class ZombieRenderer;
        class SkeletonRenderer;
        class CreeperRenderer;
        class SpiderRenderer;
        class EndermanRenderer;
    }
    namespace vehicle {
        class BoatRenderer;
        class MinecartRenderer;
    }
}
```

## 参考

- MC 1.16.5 EntityRenderer
- MC 1.16.5 LivingRenderer
- MC 1.16.5 MobRenderer
