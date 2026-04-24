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

```cpp
class PlayerRenderer : public LivingRenderer<PlayerEntity, PlayerModel> {
public:
    PlayerRenderer(bool slimArms = false);
    ResourceLocation getEntityTexture(PlayerEntity& entity);
    
private:
    void setupLayers();  // 添加盔甲层、手持物品层等
};
```

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
