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
    bool isSitting = false;  // TODO: wolf.isSitting()
    bool isAngry = false;    // TODO: wolf.isAngry()
    bool isWet = false;      // TODO: wolf.isWet()
    f32 tailRotation = wolf.getTailAngle();
    f32 shakeAngle = 0.0f;   // TODO: wolf.getShakeAngle()
    f32 interestedAngle = 0.0f; // TODO: wolf.getInterestedAngle()

    // 选择模型（幼体或成体）
    bool isChild = wolf.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    model.setAnimState(isSitting, isAngry, isWet, tailRotation, shakeAngle, interestedAngle);

    // 设置湿状态着色
    if (isWet) {
        f32 shading = 1.0f; // TODO: wolf.getShadingWhileWet()
        model.setTint(shading, shading, shading);
    } else {
        model.setTint(1.0f, 1.0f, 1.0f);
    }

    // 计算动画参数
    // TODO: 从实体获取实际动画参数
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(wolf.ticksExisted());
    f64 headYaw = 0.0;
    f64 headPitch = 0.0;
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
    (void)entity;
    // TODO: 根据 entity.isTamed() 和 entity.isAngry() 选择纹理
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

ResourceLocation WolfRenderer::getEntityTexture(const WolfEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/wolf/wolf.png");
}

void WolfRenderer::setupLayers() {
    // 参考 MC 1.16.5 WolfRenderer 构造函数
    // 添加狼项圈层
    // TODO: addLayer<layer::entity::WolfCollarLayer>();
}

void registerWolfRenderer(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:wolf", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<WolfRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::animal
