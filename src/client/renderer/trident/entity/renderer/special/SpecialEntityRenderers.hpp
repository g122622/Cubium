#pragma once

#include "../../core/EntityRenderer.hpp"
#include "../../model/projectile/ProjectileModels.hpp"
#include <memory>

namespace mc::client::renderer::entity {
class EntityRendererManager;
}

namespace mc::client::renderer::entity::renderer::special {

/**
 * @brief 末影水晶渲染器
 */
class EnderCrystalRenderer : public core::EntityRenderer {
public:
    EnderCrystalRenderer();
    ~EnderCrystalRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::EnderCrystalModel> m_model;
};

/**
 * @brief 潜影贝子弹渲染器
 */
class ShulkerBulletRenderer : public core::EntityRenderer {
public:
    ShulkerBulletRenderer();
    ~ShulkerBulletRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::ShulkerBulletModel> m_model;
};

/**
 * @brief 羊驼唾沫渲染器
 */
class LlamaSpitRenderer : public core::EntityRenderer {
public:
    LlamaSpitRenderer();
    ~LlamaSpitRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::LlamaSpitModel> m_model;
};

/**
 * @brief 光灵箭渲染器
 */
class SpectralArrowRenderer : public core::EntityRenderer {
public:
    SpectralArrowRenderer();
    ~SpectralArrowRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::SpectralArrowModel> m_model;
};

/**
 * @brief 凋灵之首渲染器
 */
class WitherSkullRenderer : public core::EntityRenderer {
public:
    WitherSkullRenderer();
    ~WitherSkullRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::WitherSkullModel> m_model;
};

/**
 * @brief 龙火球渲染器
 */
class DragonFireballRenderer : public core::EntityRenderer {
public:
    DragonFireballRenderer();
    ~DragonFireballRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::DragonFireballModel> m_model;
};

/**
 * @brief 唤魔者尖牙渲染器
 */
class EvokerFangsRenderer : public core::EntityRenderer {
public:
    EvokerFangsRenderer();
    ~EvokerFangsRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

private:
    std::shared_ptr<model::projectile::EvokerFangsModel> m_model;
};

/**
 * @brief 闪电渲染器
 */
class LightningBoltRenderer : public core::EntityRenderer {
public:
    LightningBoltRenderer();
    ~LightningBoltRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 区域效果云渲染器
 */
class AreaEffectCloudRenderer : public core::EntityRenderer {
public:
    AreaEffectCloudRenderer();
    ~AreaEffectCloudRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 下落方块渲染器
 */
class FallingBlockRenderer : public core::EntityRenderer {
public:
    FallingBlockRenderer();
    ~FallingBlockRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 物品展示框渲染器
 */
class ItemFrameRenderer : public core::EntityRenderer {
public:
    ItemFrameRenderer();
    ~ItemFrameRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 画渲染器
 */
class PaintingRenderer : public core::EntityRenderer {
public:
    PaintingRenderer();
    ~PaintingRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 栓绳结渲染器
 */
class LeashKnotRenderer : public core::EntityRenderer {
public:
    LeashKnotRenderer();
    ~LeashKnotRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 盔甲架渲染器
 */
class ArmorStandRenderer : public core::EntityRenderer {
public:
    ArmorStandRenderer();
    ~ArmorStandRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief TNT渲染器
 */
class TNTRenderer : public core::EntityRenderer {
public:
    TNTRenderer();
    ~TNTRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

/**
 * @brief 烟花火箭渲染器
 */
class FireworkRocketRenderer : public core::EntityRenderer {
public:
    FireworkRocketRenderer();
    ~FireworkRocketRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;
};

void registerSpecialEntityRenderers(EntityRendererManager& manager);

} // namespace mc::client::renderer::entity::renderer::special
