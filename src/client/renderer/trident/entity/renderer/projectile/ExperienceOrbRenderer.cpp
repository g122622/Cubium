#include "ExperienceOrbRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

ExperienceOrbRenderer::ExperienceOrbRenderer()
{
    // 经验球没有阴影
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void ExperienceOrbRenderer::render(Entity& entity, f64 partialTicks) {
    // 经验球渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ExperienceOrbRenderer::renderShadow(Entity& entity, f64 partialTicks) {
    // 经验球没有阴影
    (void)entity;
    (void)partialTicks;
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
