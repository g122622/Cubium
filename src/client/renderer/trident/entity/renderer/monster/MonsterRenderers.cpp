#include "MonsterRenderers.hpp"
#include "../../layer/equipment/HeldItemLayer.hpp"
#include "../../layer/effect/EyesLayer.hpp"
#include "../../layer/effect/EnergyGlintLayer.hpp"

namespace mc::client::renderer::entity::renderer::monster {

using mc::LivingEntity;
namespace layer_equipment = layer::equipment;
namespace layer_effect = layer::effect;

// ==================== 僵尸渲染器 ====================

ZombieRenderer::ZombieRenderer() {
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation ZombieRenderer::getEntityTexture(LivingEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
}

ResourceLocation ZombieRenderer::getEntityTexture(const LivingEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/zombie/zombie.png");
}

void ZombieRenderer::setupLayers() {
    // 参考 MC 1.16.5 ZombieRenderer 构造函数
    // 僵尸有以下层渲染器：
    // - BipedArmorLayer（盔甲）
    // - HeldItemLayer（手持物品）
    // - HeadLayer（头部物品 - 仅僵尸村民）
    // 注意：ArmorLayer 需要渲染器引用，暂时注释
    // addLayer<layer_equipment::ArmorLayer<LivingEntity, model::monster::ZombieModel>>(*this);
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 骷髅渲染器 ====================

SkeletonRenderer::SkeletonRenderer() {
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation SkeletonRenderer::getEntityTexture(LivingEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/skeleton/skeleton.png");
}

ResourceLocation SkeletonRenderer::getEntityTexture(const LivingEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/skeleton/skeleton.png");
}

void SkeletonRenderer::setupLayers() {
    // 参考 MC 1.16.5 SkeletonRenderer 构造函数
    // 骷髅有以下层渲染器：
    // - BipedArmorLayer（盔甲）
    // - HeldItemLayer（手持物品）
    // addLayer<layer_equipment::ArmorLayer<LivingEntity, model::monster::SkeletonModel>>(*this);
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
}

// ==================== 苦力怕渲染器 ====================

CreeperRenderer::CreeperRenderer() {
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation CreeperRenderer::getEntityTexture(LivingEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/creeper/creeper.png");
}

ResourceLocation CreeperRenderer::getEntityTexture(const LivingEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/creeper/creeper.png");
}

void CreeperRenderer::setupLayers() {
    // 参考 MC 1.16.5 CreeperRenderer 构造函数
    // 苦力怕有以下层渲染器：
    // - EnergySwirlLayer（充能光效，当闪电苦力怕时）
    addLayer<layer_effect::EnergyGlintLayer<LivingEntity>>();
}

// ==================== 蜘蛛渲染器 ====================

SpiderRenderer::SpiderRenderer() {
    setShadowSize(0.7f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation SpiderRenderer::getEntityTexture(LivingEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/spider/spider.png");
}

ResourceLocation SpiderRenderer::getEntityTexture(const LivingEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/spider/spider.png");
}

void SpiderRenderer::setupLayers() {
    // 参考 MC 1.16.5 SpiderRenderer 构造函数
    // 蜘蛛有以下层渲染器：
    // - EyesLayer（发光眼睛）
    // - SaddleLayer（鞍，如果是洞穴蜘蛛骑乘时）
    addLayer<layer_effect::EyesLayer<LivingEntity>>();
}

// ==================== 末影人渲染器 ====================

EndermanRenderer::EndermanRenderer() {
    setShadowSize(0.5f);
    setShadowAlpha(0.8f);
    setupLayers();
}

ResourceLocation EndermanRenderer::getEntityTexture(LivingEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/enderman/enderman.png");
}

ResourceLocation EndermanRenderer::getEntityTexture(const LivingEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/enderman/enderman.png");
}

void EndermanRenderer::render(Entity& entity, f64 partialTicks) {
    auto& living = static_cast<LivingEntity&>(entity);
    updateEndermanState(living);
    LivingRenderer::render(entity, partialTicks);
}

void EndermanRenderer::updateEndermanState(LivingEntity& entity) {
    m_model.setCarrying(false);
    m_model.setAttacking(false);
    (void)entity;
}

void EndermanRenderer::setupLayers() {
    // 参考 MC 1.16.5 EndermanRenderer 构造函数
    // 末影人有以下层渲染器：
    // - HeldItemLayer（手持方块）
    // - EyesLayer（发光眼睛）
    addLayer<layer_equipment::HeldItemLayer<LivingEntity>>();
    addLayer<layer_effect::EyesLayer<LivingEntity>>();
}

// ==================== 注册函数 ====================

void registerMonsterRenderers(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:zombie", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ZombieRenderer>();
    });

    manager.registerRenderer("minecraft:skeleton", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<SkeletonRenderer>();
    });

    manager.registerRenderer("minecraft:creeper", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<CreeperRenderer>();
    });

    manager.registerRenderer("minecraft:spider", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<SpiderRenderer>();
    });

    manager.registerRenderer("minecraft:enderman", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<EndermanRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::monster
