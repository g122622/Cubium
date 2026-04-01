#include "ExperienceOrbRenderer.hpp"
#include "../../../world/entity/ClientEntity.hpp"
#include "../../../../common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "../../../../common/entity/experience/ExperienceUtils.hpp"
#include <cmath>

namespace mc::client::renderer {

namespace {
    // 动画常量（参考 MC 1.16.5 ExperienceOrbRenderer）
    constexpr f64 BOB_AMPLITUDE = 0.1f;       // 浮动高度
    constexpr f64 BOB_FREQUENCY = 0.05f;      // 浮动速度（弧度/tick）
    constexpr f64 COLOR_SPEED = 0.1f;         // 颜色变化速度
    constexpr f64 BASE_SIZE = 0.25f;          // 基础大小
    constexpr f64 SIZE_INCREMENT = 0.015f;    // 每级大小增量

    // 经验球颜色常量（绿色主色调）
    constexpr f64 GREEN_BASE = 0.85f;
    constexpr f64 RED_AMPLITUDE = 0.15f;
    constexpr f64 BLUE_AMPLITUDE = 0.1f;
}

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

} // namespace mc::client::renderer
