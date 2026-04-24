#include "VehicleRenderers.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::vehicle {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== 船模型 ====================

BoatModel::BoatModel() {
    setupParts();
}

void BoatModel::setupParts() {
    // 参考 MC 1.16.5 BoatModel
    // 纹理尺寸：128x64

    // 船体底部
    m_bottom = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBottom");
    m_bottom->setTextureSize(128, 64);
    m_bottom->setTextureOffset(0, 0);
    // 船体形状比较复杂，用多个盒子近似

    // 船尾
    m_back = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBack");
    m_back->setTextureSize(128, 64);

    // 船首
    m_front = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatFront");
    m_front->setTextureSize(128, 64);

    // 左侧
    m_left = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatLeft");
    m_left->setTextureSize(128, 64);

    // 右侧
    m_right = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatRight");
    m_right->setTextureSize(128, 64);

    // 桨
    m_paddleLeft = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleLeft");
    m_paddleRight = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleRight");
}

void BoatModel::render(f64 scale) {
    if (m_bottom) m_bottom->render(scale);
    if (m_back) m_back->render(scale);
    if (m_front) m_front->render(scale);
    if (m_left) m_left->render(scale);
    if (m_right) m_right->render(scale);
    if (m_paddleLeft) m_paddleLeft->render(scale);
    if (m_paddleRight) m_paddleRight->render(scale);
}

// ==================== 船渲染器 ====================

BoatRenderer::BoatRenderer(BoatType type)
    : m_type(type)
    , m_model(std::make_unique<BoatModel>())
{
    m_shadowSize = 0.8f;
}

void BoatRenderer::render(Entity& entity, f64 partialTicks) {
    // TODO: 实现船渲染
    // 参考 MC 1.16.5 BoatRenderer.render()
    // 1. 计算船的朝向和倾斜
    // 2. 处理受损抖动
    // 3. 渲染船模型
    // 4. 如果不在水中，渲染水花
    (void)entity;
    (void)partialTicks;
}

ResourceLocation BoatRenderer::getTexture() const {
    static const ResourceLocation textures[] = {
        ResourceLocation("minecraft", "textures/entity/boat/oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/spruce.png"),
        ResourceLocation("minecraft", "textures/entity/boat/birch.png"),
        ResourceLocation("minecraft", "textures/entity/boat/jungle.png"),
        ResourceLocation("minecraft", "textures/entity/boat/acacia.png"),
        ResourceLocation("minecraft", "textures/entity/boat/dark_oak.png")
    };
    return textures[static_cast<size_t>(m_type)];
}

f64 BoatRenderer::calculateRockingAngle(::mc::BoatEntity& boat, f64 partialTicks) const {
    (void)boat;
    (void)partialTicks;
    return 0.0;
}

// ==================== 矿车模型 ====================

MinecartModel::MinecartModel() {
    setupParts();
}

void MinecartModel::setupParts() {
    // 参考 MC 1.16.5 MinecartModel
    // 纹理尺寸：64x32

    m_body = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("cartBody");
    m_body->setTextureSize(64, 32);
    m_body->setTextureOffset(0, 0);
    // 矿车主体：底部和侧边

    m_wheels = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("cartWheels");
    m_wheels->setTextureSize(64, 32);
}

void MinecartModel::render(f64 scale) {
    if (m_body) m_body->render(scale);
    if (m_wheels) m_wheels->render(scale);
}

// ==================== 矿车渲染器 ====================

MinecartRenderer::MinecartRenderer()
    : m_model(std::make_unique<MinecartModel>())
{
    m_shadowSize = 0.7f;
}

void MinecartRenderer::render(Entity& entity, f64 partialTicks) {
    // TODO: 实现矿车渲染
    // 参考 MC 1.16.5 MinecartRenderer.render()
    // 1. 计算矿车方向和位置
    // 2. 处理受损抖动
    // 3. 渲染矿车模型
    // 4. 如果有内容物，渲染内容物
    (void)entity;
    (void)partialTicks;
}

ResourceLocation MinecartRenderer::getMinecartTexture() {
    return ResourceLocation("minecraft", "textures/entity/minecart.png");
}

void MinecartRenderer::calculateCartDirection(::mc::AbstractMinecartEntity& minecart, f64 partialTicks) {
    (void)minecart;
    (void)partialTicks;
}

// ==================== 注册函数 ====================

void registerVehicleRenderers(::mc::client::renderer::entity::EntityRendererManager& manager) {
    // 注册各种木材类型的船
    manager.registerRenderer("minecraft:boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::Oak);
    });

    manager.registerRenderer("minecraft:spruce_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::Spruce);
    });

    manager.registerRenderer("minecraft:birch_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::Birch);
    });

    manager.registerRenderer("minecraft:jungle_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::Jungle);
    });

    manager.registerRenderer("minecraft:acacia_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::Acacia);
    });

    manager.registerRenderer("minecraft:dark_oak_boat", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<BoatRenderer>(BoatType::DarkOak);
    });

    // 注册矿车
    manager.registerRenderer("minecraft:minecart", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MinecartRenderer>();
    });

    manager.registerRenderer("minecraft:chest_minecart", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MinecartRenderer>();
    });

    manager.registerRenderer("minecraft:furnace_minecart", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MinecartRenderer>();
    });

    manager.registerRenderer("minecraft:hopper_minecart", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MinecartRenderer>();
    });

    manager.registerRenderer("minecraft:tnt_minecart", []() -> std::unique_ptr<core::EntityRenderer> {
        return std::make_unique<MinecartRenderer>();
    });
}

} // namespace mc::client::renderer::entity::renderer::vehicle
