#include "SpecialEntityRenderers.hpp"
#include "../../core/EntityRendererManager.hpp"
#include "common/entity/core/Entity.hpp"

namespace mc::client::renderer::entity::renderer::special {

// ==================== EnderCrystalRenderer ====================

EnderCrystalRenderer::EnderCrystalRenderer()
    : m_model(std::make_shared<model::projectile::EnderCrystalModel>())
{
    m_shadowSize = 0.0f;
}

void EnderCrystalRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 EnderCrystalRenderer.render()
    // 末影水晶渲染：
    // 1. 计算水晶旋转动画（innerRotation）
    // 2. 计算水晶浮动偏移（上下浮动）
    // 3. 渲染水晶核心和玻璃外壳
    // 4. 如果有束目标，渲染指向末影龙的光束

    // 设置模型角度（旋转动画）
    f64 ageInTicks = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0);

    // 渲染模型
    m_model->render(1.0 / 16.0);
}

// ==================== ShulkerBulletRenderer ====================

ShulkerBulletRenderer::ShulkerBulletRenderer()
    : m_model(std::make_shared<model::projectile::ShulkerBulletModel>())
{
    m_shadowSize = 0.0f;
}

void ShulkerBulletRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 ShulkerBulletRenderer.render()
    // 潜影贝子弹渲染：
    // 1. 计算子弹飞行方向
    // 2. 渲染子弹模型（旋转）

    f64 ageInTicks = static_cast<f64>(entity.ticksExisted()) + partialTicks;
    m_model->setAngles(0.0, 0.0, ageInTicks, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
}

// ==================== LlamaSpitRenderer ====================

LlamaSpitRenderer::LlamaSpitRenderer()
    : m_model(std::make_shared<model::projectile::LlamaSpitModel>())
{
    m_shadowSize = 0.0f;
}

void LlamaSpitRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 LlamaSpitRenderer.render()
    // 羊驼唾沫渲染：简单的投射物

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== SpectralArrowRenderer ====================

SpectralArrowRenderer::SpectralArrowRenderer()
    : m_model(std::make_shared<model::projectile::SpectralArrowModel>())
{
    m_shadowSize = 0.0f;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 SpectralArrowRenderer.render()
    // 光灵箭渲染：与普通箭类似，但有发光效果

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== WitherSkullRenderer ====================

WitherSkullRenderer::WitherSkullRenderer()
    : m_model(std::make_shared<model::projectile::WitherSkullModel>())
{
    m_shadowSize = 0.0f;
}

void WitherSkullRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 WitherSkullRenderer.render()
    // 凋灵之首渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== DragonFireballRenderer ====================

DragonFireballRenderer::DragonFireballRenderer()
    : m_model(std::make_shared<model::projectile::DragonFireballModel>())
{
    m_shadowSize = 0.0f;
}

void DragonFireballRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 DragonFireballRenderer.render()
    // 龙火球渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), static_cast<f64>(entity.pitch()), 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== EvokerFangsRenderer ====================

EvokerFangsRenderer::EvokerFangsRenderer()
    : m_model(std::make_shared<model::projectile::EvokerFangsModel>())
{
    m_shadowSize = 0.0f;
}

void EvokerFangsRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 EvokerFangsRenderer.render()
    // 唤魔者尖牙渲染

    m_model->setAngles(0.0, 0.0, 0.0, static_cast<f64>(entity.yaw()), 0.0, 1.0);
    m_model->render(1.0 / 16.0);
    (void)partialTicks;
}

// ==================== LightningBoltRenderer ====================

LightningBoltRenderer::LightningBoltRenderer() {
    m_shadowSize = 0.0f;
}

void LightningBoltRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 LightningBoltRenderer.render()
    // 闪电渲染：
    // 1. 渲染多段闪电束
    // 2. 发光效果
    // 3. 随机分支

    // 闪电使用程序化生成，不需要预定义模型
    // 需要渲染管线支持线框渲染
    (void)entity;
    (void)partialTicks;
}

// ==================== AreaEffectCloudRenderer ====================

AreaEffectCloudRenderer::AreaEffectCloudRenderer() {
    m_shadowSize = 0.0f;
}

void AreaEffectCloudRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 AreaEffectCloudRenderer.render()
    // 区域效果云渲染：
    // 1. 根据半径缩放
    // 2. 根据效果类型选择颜色
    // 3. 半透明渲染

    (void)entity;
    (void)partialTicks;
}

// ==================== FallingBlockRenderer ====================

FallingBlockRenderer::FallingBlockRenderer() {
    m_shadowSize = 0.5f;
}

void FallingBlockRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 FallingBlockRenderer.render()
    // 下落方块渲染：
    // 1. 获取方块状态
    // 2. 渲染方块模型
    // 3. 位置插值

    (void)entity;
    (void)partialTicks;
}

// ==================== ItemFrameRenderer ====================

ItemFrameRenderer::ItemFrameRenderer() {
    m_shadowSize = 0.0f;
}

void ItemFrameRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 ItemFrameRenderer.render()
    // 物品展示框渲染：
    // 1. 渲染边框
    // 2. 渲染物品（如果有的话）
    // 3. 渲染地图（如果是地图）

    (void)entity;
    (void)partialTicks;
}

// ==================== PaintingRenderer ====================

PaintingRenderer::PaintingRenderer() {
    m_shadowSize = 0.0f;
}

void PaintingRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 PaintingRenderer.render()
    // 画渲染：
    // 1. 根据画类型选择纹理
    // 2. 渲染画布

    (void)entity;
    (void)partialTicks;
}

// ==================== LeashKnotRenderer ====================

LeashKnotRenderer::LeashKnotRenderer() {
    m_shadowSize = 0.0f;
}

void LeashKnotRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 LeashKnotRenderer.render()
    // 拴绳结渲染：简单的模型

    (void)entity;
    (void)partialTicks;
}

// ==================== ArmorStandRenderer ====================

ArmorStandRenderer::ArmorStandRenderer() {
    m_shadowSize = 0.0f;
}

void ArmorStandRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 ArmorStandRenderer.render()
    // 盔甲架渲染：
    // 1. 渲染盔甲架基座
    // 2. 渲染装备的盔甲
    // 3. 渲染手持物品
    // 4. 处理破坏动画

    (void)entity;
    (void)partialTicks;
}

// ==================== TNTRenderer ====================

TNTRenderer::TNTRenderer() {
    m_shadowSize = 0.5f;
}

void TNTRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 TNTRenderer.render()
    // TNT渲染：
    // 1. 渲染TNT方块
    // 2. 处理点燃闪烁

    (void)entity;
    (void)partialTicks;
}

// ==================== FireworkRocketRenderer ====================

FireworkRocketRenderer::FireworkRocketRenderer() {
    m_shadowSize = 0.0f;
}

void FireworkRocketRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 FireworkRocketRenderer.render()
    // 烟花火箭渲染：
    // 1. 渲染火箭模型
    // 2. 处理粒子尾迹

    (void)entity;
    (void)partialTicks;
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
