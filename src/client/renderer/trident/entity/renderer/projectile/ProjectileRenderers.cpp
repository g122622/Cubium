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

#include "ProjectileRenderers.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

// ==================== 箭矢渲染器 ====================

ArrowRenderer::ArrowRenderer()
{
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
    generateArrowMesh();
}

void ArrowRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 ArrowRenderer.render()
    // 1. 计算箭矢朝向（插值）
    // 2. 处理箭矢抖动动画
    // 3. 渲染箭矢网格

    // 计算插值朝向
    f64 yaw = static_cast<f64>(entity.prevYaw()) +
        (static_cast<f64>(entity.yaw()) - static_cast<f64>(entity.prevYaw())) * partialTicks;
    f64 pitch = static_cast<f64>(entity.prevPitch()) +
        (static_cast<f64>(entity.pitch()) - static_cast<f64>(entity.prevPitch())) * partialTicks;

    // 箭矢抖动 - ArrowEntity 有 inGround 和 arrowShake 时间
    // 抖动动画需要在箭矢刚着地时应用

    // 渲染箭矢 - 需要渲染管线支持
    (void)entity;
    (void)partialTicks;
    (void)yaw;
    (void)pitch;
}

ResourceLocation ArrowRenderer::getArrowTexture()
{
    return ResourceLocation("minecraft", "textures/entity/projectiles/arrow.png");
}

void ArrowRenderer::generateArrowMesh()
{
    // 参考 MC 1.16.5 ArrowRenderer
    // 箭矢顶点：
    // - 箭杆：-7 到 8，宽度 2x2
    // - 箭头：四个面
    // 纹理坐标：
    // - 箭杆侧面: (0, 0) - (0.5, 0.15625)
    // - 箭头: (0.5, 0) - (0.5, 0.15625)

    m_meshGenerated = true;
}

// ==================== 光灵箭渲染器 ====================

SpectralArrowRenderer::SpectralArrowRenderer()
{
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 SpectralArrowRenderer
    // 与普通箭矢类似，但带有发光效果
    // 发光效果通过 RenderType.getOutline() 实现
    // 需要后处理管线支持

    f64 yaw = static_cast<f64>(entity.prevYaw()) +
        (static_cast<f64>(entity.yaw()) - static_cast<f64>(entity.prevYaw())) * partialTicks;
    f64 pitch = static_cast<f64>(entity.prevPitch()) +
        (static_cast<f64>(entity.pitch()) - static_cast<f64>(entity.prevPitch())) * partialTicks;

    (void)entity;
    (void)partialTicks;
    (void)yaw;
    (void)pitch;
}

ResourceLocation SpectralArrowRenderer::getSpectralArrowTexture()
{
    return ResourceLocation("minecraft", "textures/entity/projectiles/spectral_arrow.png");
}

// ==================== 三叉戟渲染器 ====================

TridentRenderer::TridentRenderer()
{
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
    generateTridentMesh();
}

void TridentRenderer::render(Entity& entity, f64 partialTicks)
{
    // 参考 MC 1.16.5 TridentRenderer.render()
    // 1. 计算三叉戟朝向
    // 2. 渲染三叉戟模型
    // 3. 投掷时有旋转动画

    f64 yaw = static_cast<f64>(entity.prevYaw()) +
        (static_cast<f64>(entity.yaw()) - static_cast<f64>(entity.prevYaw())) * partialTicks;
    f64 pitch = static_cast<f64>(entity.prevPitch()) +
        (static_cast<f64>(entity.pitch()) - static_cast<f64>(entity.prevPitch())) * partialTicks;

    (void)entity;
    (void)partialTicks;
    (void)yaw;
    (void)pitch;
}

ResourceLocation TridentRenderer::getTridentTexture()
{
    return ResourceLocation("minecraft", "textures/entity/trident.png");
}

void TridentRenderer::generateTridentMesh()
{
    // 参考 MC 1.16.5 TridentModel
    // 三叉戟模型：
    // - 长杆
    // - 三个叉尖
    m_meshGenerated = true;
}

// ==================== 注册函数 ====================

void registerProjectileRenderers(::mc::client::renderer::entity::EntityRendererManager& manager)
{
    manager.registerRenderer(
        "minecraft:arrow", []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<ArrowRenderer>(); });

    manager.registerRenderer("minecraft:spectral_arrow",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<SpectralArrowRenderer>(); });

    manager.registerRenderer("minecraft:trident",
        []() -> std::unique_ptr<core::EntityRenderer> { return std::make_unique<TridentRenderer>(); });
}

} // namespace mc::client::renderer::entity::renderer::projectile
