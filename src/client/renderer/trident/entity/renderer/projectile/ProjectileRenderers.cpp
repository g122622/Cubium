#include "ProjectileRenderers.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/entity/core/Entity.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::projectile {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== 箭矢渲染器 ====================

ArrowRenderer::ArrowRenderer() {
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
    generateArrowMesh();
}

void ArrowRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 ArrowRenderer.render()
    // 1. 计算箭矢朝向（插值）
    // 2. 处理箭矢抖动动画
    // 3. 渲染箭矢网格

    // 计算插值朝向
    f64 yaw = entity.prevYaw() + (entity.yaw() - entity.prevYaw()) * partialTicks;
    f64 pitch = entity.prevPitch() + (entity.pitch() - entity.prevPitch()) * partialTicks;

    // 箭矢抖动 (arrowShake)
    // TODO: 从实体获取 arrowShake 属性
    // f64 shake = entity.arrowShake() - partialTicks;
    // if (shake > 0.0) {
    //     f64 shakeAngle = std::sin(shake * 3.0) * shake;
    //     // 应用抖动旋转
    // }

    // 渲染箭矢
    // TODO: 使用 Pipeline 渲染箭矢网格
    (void)entity;
    (void)partialTicks;
    (void)yaw;
    (void)pitch;
}

ResourceLocation ArrowRenderer::getArrowTexture() {
    return ResourceLocation("minecraft", "textures/entity/projectiles/arrow.png");
}

void ArrowRenderer::generateArrowMesh() {
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

SpectralArrowRenderer::SpectralArrowRenderer() {
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void SpectralArrowRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 SpectralArrowRenderer
    // 与普通箭矢类似，但带有发光效果
    // TODO: 实现光灵箭渲染
    (void)entity;
    (void)partialTicks;
}

ResourceLocation SpectralArrowRenderer::getSpectralArrowTexture() {
    return ResourceLocation("minecraft", "textures/entity/projectiles/spectral_arrow.png");
}

// ==================== 三叉戟渲染器 ====================

TridentRenderer::TridentRenderer() {
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
    generateTridentMesh();
}

void TridentRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 TridentRenderer.render()
    // 1. 计算三叉戟朝向
    // 2. 渲染三叉戟模型
    (void)entity;
    (void)partialTicks;
}

ResourceLocation TridentRenderer::getTridentTexture() {
    return ResourceLocation("minecraft", "textures/entity/trident.png");
}

void TridentRenderer::generateTridentMesh() {
    // 参考 MC 1.16.5 TridentModel
    // 三叉戟模型：
    // - 长杆
    // - 三个叉尖
    m_meshGenerated = true;
}

// ==================== 注册函数 ====================

void registerProjectileRenderers(::mc::client::renderer::entity::EntityRendererManager& manager) {
    manager.registerRenderer("minecraft:arrow", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<ArrowRenderer>();
    });

    manager.registerRenderer("minecraft:spectral_arrow", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<SpectralArrowRenderer>();
    });

    manager.registerRenderer("minecraft:trident", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<TridentRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::projectile
