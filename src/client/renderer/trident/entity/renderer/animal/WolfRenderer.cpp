#include "WolfRenderer.hpp"
#include "common/entity/entities/passive/tamable/WolfEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::WolfEntity;

WolfRenderer::WolfRenderer()
    : m_model()
    , m_modelBaby()
{
    setShadowSize(0.5f);
    setupLayers();
}

void WolfRenderer::render(Entity& entity, f64 partialTicks) {
    auto& wolf = static_cast<WolfEntity&>(entity);

    // 设置模型动画状态
    bool isSitting = wolf.isSitting();
    bool isAngry = wolf.isAngry();
    bool isWet = wolf.isInWater();
    f32 tailRotation = wolf.getTailAngle();
    f32 shakeAngle = 0.0f;   // 甩水动画角度 - 需要实体状态追踪
    f32 interestedAngle = wolf.isInterested() ? 0.5f : 0.0f;

    // 选择模型（幼体或成体）
    bool isChild = wolf.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    model.setAnimState(isSitting, isAngry, isWet, tailRotation, shakeAngle, interestedAngle);

    // 设置湿状态着色
    if (isWet) {
        // 湿润时稍微变暗
        f32 shading = 0.75f;
        model.setTint(shading, shading, shading);
    } else {
        model.setTint(1.0f, 1.0f, 1.0f);
    }

    // 计算动画参数（从LivingEntity获取）
    f64 limbSwing = static_cast<f64>(wolf.prevLimbSwing()) + (static_cast<f64>(wolf.limbSwing()) - static_cast<f64>(wolf.prevLimbSwing())) * partialTicks;
    f64 limbSwingAmount = static_cast<f64>(wolf.prevLimbSwingAmount()) + (static_cast<f64>(wolf.limbSwingAmount()) - static_cast<f64>(wolf.prevLimbSwingAmount())) * partialTicks;
    f64 ageInTicks = static_cast<f64>(wolf.ticksExisted());
    f64 headYaw = static_cast<f64>(wolf.prevRotationYawHead()) + (static_cast<f64>(wolf.rotationYawHead()) - static_cast<f64>(wolf.prevRotationYawHead())) * partialTicks;
    f64 headPitch = static_cast<f64>(wolf.prevPitch()) + (static_cast<f64>(wolf.pitch()) - static_cast<f64>(wolf.prevPitch())) * partialTicks;
    f64 scale = isChild ? 0.5 : 1.0;

    model.setAngles(limbSwing, limbSwingAmount, ageInTicks, headYaw, headPitch, scale);
    model.render(scale / 16.0);

    // 渲染阴影
    if (m_shadowSize > 0.0) {
        renderShadow(entity, partialTicks);
    }
}

ResourceLocation WolfRenderer::getEntityTexture(WolfEntity& entity) {
    // 参考 MC 1.16.5 WolfRenderer.getEntityTexture
    // 根据狼的状态选择纹理：
    // - 驯服：wolf_tame.png
    // - 愤怒：wolf_angry.png
    // - 普通：wolf.png
    if (entity.isTamed()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_tame.png");
    }
    if (entity.isAngry()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_angry.png");
    }
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

ResourceLocation WolfRenderer::getEntityTexture(const WolfEntity& entity) const {
    if (entity.isTamed()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_tame.png");
    }
    if (entity.isAngry()) {
        return ResourceLocation("minecraft", "textures/entity/wolf/wolf_angry.png");
    }
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

void WolfRenderer::setupLayers() {
    // 参考 MC 1.16.5 WolfRenderer 构造函数
    // 添加狼项圈层 - 待层渲染器系统完善后实现
}

void registerWolfRenderer(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:wolf", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<WolfRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::animal
