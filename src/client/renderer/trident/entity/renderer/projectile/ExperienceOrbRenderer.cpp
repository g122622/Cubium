#include "ExperienceOrbRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

ExperienceOrbRenderer::ExperienceOrbRenderer()
{
    // MC 1.16.5: 经验球阴影大小为 0.15
    m_shadowSize = 0.15f;
    // MC 1.16.5: 经验球阴影透明度 0.75
    m_shadowAlpha = 0.75f;
}

void ExperienceOrbRenderer::render(Entity& entity, f64 partialTicks)
{
    // 经验球渲染由 EntityRendererManager::renderWithPipeline 处理
    // 这里是传统渲染路径，暂时不实现
    (void)entity;
    (void)partialTicks;
}

void ExperienceOrbRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    // MC 1.16.5: 经验球有阴影
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

i32 ExperienceOrbRenderer::getSizeByValue(i32 xpValue)
{
    // MC 1.16.5 ExperienceOrbEntity.sizeByValue
    // 根据经验值返回大小等级 (0-10)
    if (xpValue >= 2477) {
        return 10;
    } else if (xpValue >= 1237) {
        return 9;
    } else if (xpValue >= 617) {
        return 8;
    } else if (xpValue >= 307) {
        return 7;
    } else if (xpValue >= 149) {
        return 6;
    } else if (xpValue >= 73) {
        return 5;
    } else if (xpValue >= 37) {
        return 4;
    } else if (xpValue >= 17) {
        return 3;
    } else if (xpValue >= 7) {
        return 2;
    } else if (xpValue >= 3) {
        return 1;
    } else {
        return 0;
    }
}

f64 ExperienceOrbRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick) const
{
    // MC 1.16.5 ExperienceOrbRenderer:
    // 经验球上下浮动动画
    // 基础浮动频率比 ItemEntity 慢（/20.0 而非 /10.0）
    // 基础高度偏移比 ItemEntity 高（0.3 而非 0.1）

    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * BOB_FREQUENCY) * BOB_AMPLITUDE + BOB_BASE;
}

f64 ExperienceOrbRenderer::calculateColorPhase(u32 ticksExisted, f64 partialTick) const
{
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::fmod(ticks * COLOR_SPEED, 1.0);
}

math::Vector4f ExperienceOrbRenderer::calculateColor(f64 phase) const
{
    // MC 1.16.5 经验球颜色：绿色系渐变
    // 颜色随时间变化，从深绿到亮绿
    //
    // MC 1.16.5 使用的颜色：
    // int i = this.ticksExisted / 3 + this.xpValue;
    // float f = (float)(i % 16) / 16.0F;
    // int j = ExperienceOrbEntity.getColorFromXP(this.xpValue);
    // float f1 = (float)(j >> 16 & 255) / 255.0F;
    // float f2 = (float)(j >> 8 & 255) / 255.0F;
    // float f3 = (float)(j & 255) / 255.0F;
    //
    // 经验球颜色基于经验值：
    // - 低值 (0-6): RGB(66, 209, 54) - 亮绿色
    // - 中值 (7-16): RGB(101, 208, 42)
    // - 高值 (17+): RGB(124, 197, 46)

    // 简化实现：使用绿色渐变
    f32 r = static_cast<f32>(0.25 + phase * 0.2); // 0.25 - 0.45
    f32 g = static_cast<f32>(0.8 + phase * 0.15); // 0.8 - 0.95
    f32 b = static_cast<f32>(0.2 + phase * 0.1);  // 0.2 - 0.3

    return math::Vector4f(r, g, b, 1.0f);
}

} // namespace mc::client::renderer::entity::renderer::projectile
