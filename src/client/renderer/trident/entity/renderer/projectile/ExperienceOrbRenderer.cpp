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
 * The above copyright notice shall this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ExperienceOrbRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
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
    // 经验球的实际渲染由 EntityRendererManager::renderWithPipeline() 中的特殊路径处理，
    // 包括 billboard 网格、浮动动画、颜色动画、图标选择和缩放。
    // 此方法为空操作，因为管线管理器已完全接管渲染逻辑。
    (void)entity;
    (void)partialTicks;
}

void ExperienceOrbRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

i32 ExperienceOrbRenderer::getSizeByValue(i32 xpValue)
{
    return mc::entity::experience::utils::getOrbSize(xpValue);
}

f64 ExperienceOrbRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick)
{
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * BOB_FREQUENCY) * BOB_AMPLITUDE + BOB_BASE;
}

math::Vector4f ExperienceOrbRenderer::calculateColor(f64 time)
{
    return mc::entity::experience::utils::calculateOrbColor(time);
}

void ExperienceOrbRenderer::calculateIconUV(i32 iconIndex, f64& u0, f64& v0, f64& u1, f64& v1)
{
    // 委托给 ExperienceUtils 中的通用计算函数
    mc::entity::experience::utils::calculateOrbIconUV(iconIndex, u0, v0, u1, v1);
}

} // namespace mc::client::renderer::entity::renderer::projectile
