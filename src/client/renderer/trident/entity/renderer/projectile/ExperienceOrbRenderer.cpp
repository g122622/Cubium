/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ExperienceOrbRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

ExperienceOrbRenderer::ExperienceOrbRenderer()
{
    m_shadowSize = 0.15f;
    m_shadowAlpha = 0.75f;
}

void ExperienceOrbRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 经验球渲染尚未实现，等待渲染管线集成
    (void)entity;
    (void)partialTicks;
}

void ExperienceOrbRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

i32 ExperienceOrbRenderer::getSizeByValue(i32 xpValue)
{
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

f64 ExperienceOrbRenderer::_calculateBobOffset(u32 ticksExisted, f64 partialTick) const
{
    // 经验球上下浮动动画，频率比 ItemEntity 慢，基础高度偏移比 ItemEntity 高

    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * BOB_FREQUENCY) * BOB_AMPLITUDE + BOB_BASE;
}

f64 ExperienceOrbRenderer::_calculateColorPhase(u32 ticksExisted, f64 partialTick) const
{
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::fmod(ticks * COLOR_SPEED, 1.0);
}

math::Vector4f ExperienceOrbRenderer::_calculateColor(f64 phase) const
{
    // TODO: 当前为简化的绿色渐变实现，后续需要实现基于经验值的精确颜色计算
    // 经验球颜色基于经验值分档：
    // - 低值 (0-6): 亮绿色
    // - 中值 (7-16): 中绿色
    // - 高值 (17+): 深绿色
    f32 r = static_cast<f32>(0.25 + phase * 0.2); // 0.25 - 0.45
    f32 g = static_cast<f32>(0.8 + phase * 0.15); // 0.8 - 0.95
    f32 b = static_cast<f32>(0.2 + phase * 0.1);  // 0.2 - 0.3

    return math::Vector4f(r, g, b, 1.0f);
}

} // namespace mc::client::renderer::entity::renderer::projectile
