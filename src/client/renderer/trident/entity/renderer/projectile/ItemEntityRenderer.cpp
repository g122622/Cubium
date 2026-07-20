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

#include "ItemEntityRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

ItemEntityRenderer::ItemEntityRenderer()
{
    m_shadowSize = 0.15f;
    m_shadowAlpha = 0.75f;
}

void ItemEntityRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: ItemEntity 渲染由 EntityRendererManager::renderWithPipeline 处理，传统渲染路径尚未实现
    (void)entity;
    (void)partialTicks;
}

void ItemEntityRenderer::renderShadow(Entity& entity, f64 partialTicks)
{
    core::EntityRenderer::renderShadow(entity, partialTicks);
}

f64 ItemEntityRenderer::calculateBobOffset(u32 ticksExisted, f64 partialTick, f32 hoverStart)
{
    const f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks / BOB_PERIOD + static_cast<f64>(hoverStart)) * BOB_AMPLITUDE + BOB_BASE +
        GROUND_TRANSFORM_Y_OFFSET;
}

f64 ItemEntityRenderer::calculateRotation(u32 ticksExisted, f64 partialTick, f32 hoverStart)
{
    return ((static_cast<f64>(ticksExisted) + partialTick) / ROTATION_PERIOD + static_cast<f64>(hoverStart)) *
        static_cast<f64>(math::RAD_TO_DEG);
}

i32 ItemEntityRenderer::getItemCountForRender(i32 count)
{
    if (count <= 1) {
        return 1;
    } else if (count <= 16) {
        return 2;
    } else if (count <= 32) {
        return 3;
    } else if (count <= 48) {
        return 4;
    } else {
        return 5;
    }
}

} // namespace mc::client::renderer::entity::renderer::projectile
