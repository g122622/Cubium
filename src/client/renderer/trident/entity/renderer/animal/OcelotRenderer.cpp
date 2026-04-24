#include "OcelotRenderer.hpp"
#include "common/entity/entities/passive/tamable/OcelotEntity.hpp"

namespace mc::client::renderer::entity::renderer::animal {

using mc::OcelotEntity;

OcelotRenderer::OcelotRenderer()
    : m_model(0.0f)
    , m_modelBaby(0.0f)
{
    setShadowSize(0.5f);
}

void OcelotRenderer::render(Entity& entity, f64 partialTicks) {
    auto& ocelot = static_cast<OcelotEntity&>(entity);

    // 选择模型（幼体或成体）
    bool isChild = ocelot.isChild();
    auto& model = isChild ? m_modelBaby : m_model;

    // 设置状态（站立/奔跑）
    int state = ocelot.isFleeing() ? 2 : 1;
    model.setState(state);

    // 计算动画参数
    f64 limbSwing = 0.0;
    f64 limbSwingAmount = 0.0;
    f64 ageInTicks = static_cast<f64>(ocelot.ticksExisted());
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

ResourceLocation OcelotRenderer::getEntityTexture(OcelotEntity& entity) {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

ResourceLocation OcelotRenderer::getEntityTexture(const OcelotEntity& entity) const {
    (void)entity;
    return ResourceLocation("minecraft", "textures/entity/cat/ocelot.png");
}

void registerOcelotRenderer(EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:ocelot", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<OcelotRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::animal
