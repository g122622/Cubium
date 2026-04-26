#include "ExperienceOrbRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

ExperienceOrbRenderer::ExperienceOrbRenderer()
{
    // MC 1.16.5: 经验球阴影大小为 0.15
    m_shadowSize = 0.15f;
    m_shadowAlpha = 0.8f;
}

void ExperienceOrbRenderer::render(Entity& entity, f64 partialTicks) {
    // 经验球渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ExperienceOrbRenderer::renderShadow(Entity& entity, f64 partialTicks) {
    // MC 1.16.5: 经验球有阴影
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

f64 ExperienceOrbRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick) const {
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * BOB_FREQUENCY) * BOB_AMPLITUDE;
}

f64 ExperienceOrbRenderer::calculateColorPhase(u32 ticksExisted, f64 partialTick) const {
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::fmod(ticks * COLOR_SPEED, 1.0f);
}

} // namespace mc::client::renderer::entity::renderer::projectile
