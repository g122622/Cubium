#include "HorseRenderer.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::HorseEntity;

HorseRenderer::HorseRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.85f);
    setupLayers();
}

void HorseRenderer::render(Entity& entity, f64 partialTicks) {
    // 选择模型（幼体或成体）
    bool isChild = entity.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置鞍和骑乘状态
    // TODO: 从实体获取实际状态
    model.setSaddled(false);
    model.setRidden(false);

    // 计算动画参数
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(entity.ticksExisted());
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

ResourceLocation HorseRenderer::getEntityTexture(::mc::HorseEntity& entity) {
    // TODO: 根据马变种和颜色选择纹理
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/horse/horse_white.png");
}

ResourceLocation HorseRenderer::getEntityTexture(const ::mc::HorseEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/horse/horse_white.png");
}

void HorseRenderer::setupLayers() {
    // TODO: 添加鞍层、马甲层等
}

void registerHorseRenderer(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:horse", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<HorseRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::animal
