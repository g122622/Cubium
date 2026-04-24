#include "SpecialEntityRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"

namespace mc::client::renderer::entity::renderer::special {

// ==================== EnderCrystalRenderer ====================

EnderCrystalRenderer::EnderCrystalRenderer()
    : m_model(std::make_shared<model::projectile::EnderCrystalModel>())
{
    m_shadowSize = 0.0f;
}

void EnderCrystalRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== ShulkerBulletRenderer ====================

ShulkerBulletRenderer::ShulkerBulletRenderer()
    : m_model(std::make_shared<model::projectile::ShulkerBulletModel>())
{
    m_shadowSize = 0.0f;
}

void ShulkerBulletRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== LlamaSpitRenderer ====================

LlamaSpitRenderer::LlamaSpitRenderer()
    : m_model(std::make_shared<model::projectile::LlamaSpitModel>())
{
    m_shadowSize = 0.0f;
}

void LlamaSpitRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== SpectralArrowRenderer ====================

SpectralArrowRenderer::SpectralArrowRenderer()
    : m_model(std::make_shared<model::projectile::SpectralArrowModel>())
{
    m_shadowSize = 0.0f;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== WitherSkullRenderer ====================

WitherSkullRenderer::WitherSkullRenderer()
    : m_model(std::make_shared<model::projectile::WitherSkullModel>())
{
    m_shadowSize = 0.0f;
}

void WitherSkullRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== DragonFireballRenderer ====================

DragonFireballRenderer::DragonFireballRenderer()
    : m_model(std::make_shared<model::projectile::DragonFireballModel>())
{
    m_shadowSize = 0.0f;
}

void DragonFireballRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== EvokerFangsRenderer ====================

EvokerFangsRenderer::EvokerFangsRenderer()
    : m_model(std::make_shared<model::projectile::EvokerFangsModel>())
{
    m_shadowSize = 0.0f;
}

void EvokerFangsRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement rendering
}

// ==================== LightningBoltRenderer ====================

LightningBoltRenderer::LightningBoltRenderer() {
    m_shadowSize = 0.0f;
}

void LightningBoltRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement lightning bolt rendering
}

// ==================== AreaEffectCloudRenderer ====================

AreaEffectCloudRenderer::AreaEffectCloudRenderer() {
    m_shadowSize = 0.0f;
}

void AreaEffectCloudRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement area effect cloud rendering
}

// ==================== FallingBlockRenderer ====================

FallingBlockRenderer::FallingBlockRenderer() {
    m_shadowSize = 0.5f;
}

void FallingBlockRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement falling block rendering
}

// ==================== ItemFrameRenderer ====================

ItemFrameRenderer::ItemFrameRenderer() {
    m_shadowSize = 0.0f;
}

void ItemFrameRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement item frame rendering
}

// ==================== PaintingRenderer ====================

PaintingRenderer::PaintingRenderer() {
    m_shadowSize = 0.0f;
}

void PaintingRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement painting rendering
}

// ==================== LeashKnotRenderer ====================

LeashKnotRenderer::LeashKnotRenderer() {
    m_shadowSize = 0.0f;
}

void LeashKnotRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement leash knot rendering
}

// ==================== ArmorStandRenderer ====================

ArmorStandRenderer::ArmorStandRenderer() {
    m_shadowSize = 0.0f;
}

void ArmorStandRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement armor stand rendering
}

// ==================== TNTRenderer ====================

TNTRenderer::TNTRenderer() {
    m_shadowSize = 0.5f;
}

void TNTRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement TNT rendering
}

// ==================== FireworkRocketRenderer ====================

FireworkRocketRenderer::FireworkRocketRenderer() {
    m_shadowSize = 0.0f;
}

void FireworkRocketRenderer::render(Entity& entity, f64 partialTicks) {
    (void)entity;
    (void)partialTicks;
    // TODO: Implement firework rocket rendering
}

// ==================== Registration ====================

void registerSpecialEntityRenderers(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:end_crystal", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<EnderCrystalRenderer>();
    });

    manager.registerRenderer("minecraft:shulker_bullet", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ShulkerBulletRenderer>();
    });

    manager.registerRenderer("minecraft:llama_spit", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<LlamaSpitRenderer>();
    });

    manager.registerRenderer("minecraft:spectral_arrow", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<SpectralArrowRenderer>();
    });

    manager.registerRenderer("minecraft:wither_skull", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<WitherSkullRenderer>();
    });

    manager.registerRenderer("minecraft:dragon_fireball", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<DragonFireballRenderer>();
    });

    manager.registerRenderer("minecraft:evoker_fangs", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<EvokerFangsRenderer>();
    });

    manager.registerRenderer("minecraft:lightning_bolt", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<LightningBoltRenderer>();
    });

    manager.registerRenderer("minecraft:area_effect_cloud", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<AreaEffectCloudRenderer>();
    });

    manager.registerRenderer("minecraft:falling_block", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<FallingBlockRenderer>();
    });

    manager.registerRenderer("minecraft:item_frame", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ItemFrameRenderer>();
    });

    manager.registerRenderer("minecraft:painting", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<PaintingRenderer>();
    });

    manager.registerRenderer("minecraft:leash_knot", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<LeashKnotRenderer>();
    });

    manager.registerRenderer("minecraft:armor_stand", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ArmorStandRenderer>();
    });

    manager.registerRenderer("minecraft:tnt", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<TNTRenderer>();
    });

    manager.registerRenderer("minecraft:firework_rocket", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<FireworkRocketRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::special
