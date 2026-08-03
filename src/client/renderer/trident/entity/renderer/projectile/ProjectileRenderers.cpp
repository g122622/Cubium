/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the following conditions:
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

#include "ProjectileRenderers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc::client::renderer::entity::renderer::projectile {

// ==================== 箭矢渲染器 ====================

ArrowRenderer::ArrowRenderer()
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
    _generateArrowMesh();
}

void ArrowRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现箭矢渲染 - 计算插值朝向、箭矢抖动动画、渲染箭矢网格
    (void)entity;
    (void)partialTicks;
}

ResourceLocation ArrowRenderer::getArrowTexture()
{
    return ResourceLocation("minecraft", "textures/entity/projectiles/arrow.png");
}

void ArrowRenderer::_generateArrowMesh()
{
    // TODO: 实现箭矢网格生成 - 箭杆(-7到8, 宽2x2)和箭头四个面
    m_meshGenerated = true;
}

// ==================== 光灵箭渲染器 ====================

SpectralArrowRenderer::SpectralArrowRenderer()
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现光灵箭渲染 - 与普通箭矢类似但带发光效果，需要后处理管线支持
    (void)entity;
    (void)partialTicks;
}

ResourceLocation SpectralArrowRenderer::getSpectralArrowTexture()
{
    return ResourceLocation("minecraft", "textures/entity/projectiles/spectral_arrow.png");
}

// ==================== 三叉戟渲染器 ====================

TridentRenderer::TridentRenderer()
{
    m_shadowSize = 0.0;
    m_shadowAlpha = 0.0;
    _generateTridentMesh();
}

void TridentRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 实现三叉戟渲染 - 计算朝向、渲染模型、投掷旋转动画
    (void)entity;
    (void)partialTicks;
}

ResourceLocation TridentRenderer::getTridentTexture()
{
    return ResourceLocation("minecraft", "textures/entity/trident.png");
}

void TridentRenderer::_generateTridentMesh()
{
    // TODO: 实现三叉戟网格生成 - 长杆和三个叉尖
    m_meshGenerated = true;
}

} // namespace mc::client::renderer::entity::renderer::projectile
