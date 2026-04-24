#include "VehicleRenderers.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include <cmath>

namespace mc::client::renderer::entity::renderer::vehicle {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== 船模型 ====================

BoatModel::BoatModel()
    : m_textureWidth(128)
    , m_textureHeight(64)
{
    setupParts();
}

void BoatModel::setupParts() {
    // 参考 MC 1.16.5 BoatModel
    // 纹理尺寸：128x64

    // 创建5个船体面（底部、右、左、前、后）
    // MC: amodelrenderer[0].addBox(-14.0F, -9.0F, -3.0F, 28.0F, 16.0F, 3.0F, 0.0F);
    //     amodelrenderer[0].setRotationPoint(0.0F, 3.0F, 1.0F);
    //     amodelrenderer[0].rotateAngleX = PI/2

    // 底部面 (amodelrenderer[0])
    m_bottom = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBottom");
    m_bottom->setTextureSize(128, 64);
    m_bottom->setTextureOffset(0, 0);
    m_bottom->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_bottom->setRotationPoint(0.0f, 3.0f, 1.0f);
    m_bottom->setRotateAngleX(static_cast<f32>(PI / 2.0));

    // 右侧面 (amodelrenderer[1]) - 旋转 PI*1.5
    m_right = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatRight");
    m_right->setTextureSize(128, 64);
    m_right->setTextureOffset(0, 19);
    m_right->addBox(-13.0f, -7.0f, -1.0f, 18.0f, 6.0f, 2.0f, 0.0f);
    m_right->setRotationPoint(-15.0f, 4.0f, 4.0f);
    m_right->setRotateAngleY(static_cast<f32>(PI * 1.5));

    // 左侧面 (amodelrenderer[2]) - 旋转 PI/2
    m_left = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatLeft");
    m_left->setTextureSize(128, 64);
    m_left->setTextureOffset(0, 19);
    m_left->addBox(-8.0f, -7.0f, -1.0f, 16.0f, 6.0f, 2.0f, 0.0f);
    m_left->setRotationPoint(15.0f, 4.0f, 0.0f);
    m_left->setRotateAngleY(static_cast<f32>(PI / 2.0));

    // 前面 (amodelrenderer[3]) - 旋转 PI
    m_front = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatFront");
    m_front->setTextureSize(128, 64);
    m_front->setTextureOffset(0, 19);
    m_front->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_front->setRotationPoint(0.0f, 4.0f, -9.0f);
    m_front->setRotateAngleY(static_cast<f32>(PI));

    // 后面 (amodelrenderer[4])
    m_back = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBack");
    m_back->setTextureSize(128, 64);
    m_back->setTextureOffset(0, 19);
    m_back->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_back->setRotationPoint(0.0f, 4.0f, 9.0f);

    // 桨 - 参考 MC makePaddle()
    m_paddleLeft = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleLeft");
    m_paddleLeft->setTextureSize(128, 64);
    m_paddleLeft->setTextureOffset(62, 0);
    m_paddleLeft->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleLeft->addBox(-1.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleLeft->setRotationPoint(3.0f, -5.0f, 9.0f);
    m_paddleLeft->setRotateAngleZ(0.19634955f);

    m_paddleRight = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleRight");
    m_paddleRight->setTextureSize(128, 64);
    m_paddleRight->setTextureOffset(62, 20);
    m_paddleRight->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleRight->addBox(0.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleRight->setRotationPoint(3.0f, -5.0f, -9.0f);
    m_paddleRight->setRotateAngleY(static_cast<f32>(PI));
    m_paddleRight->setRotateAngleZ(0.19634955f);

    // 水面以下不可见的底部
    m_noWater = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatNoWater");
    m_noWater->setTextureSize(128, 64);
    m_noWater->setTextureOffset(0, 0);
    m_noWater->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_noWater->setRotationPoint(0.0f, -3.0f, 1.0f);
    m_noWater->setRotateAngleX(static_cast<f32>(PI / 2.0));
}

void BoatModel::render(f64 scale) {
    if (m_bottom) m_bottom->render(scale);
    if (m_right) m_right->render(scale);
    if (m_left) m_left->render(scale);
    if (m_front) m_front->render(scale);
    if (m_back) m_back->render(scale);
    if (m_paddleLeft) m_paddleLeft->render(scale);
    if (m_paddleRight) m_paddleRight->render(scale);
}

void BoatModel::renderNoWater(f64 scale) {
    if (m_noWater) m_noWater->render(scale);
}

void BoatModel::setPaddleAngle(i32 paddleIndex, f32 angle) {
    if (paddleIndex == 0 && m_paddleLeft) {
        m_paddleLeft->setRotateAngleX(angle);
    } else if (paddleIndex == 1 && m_paddleRight) {
        m_paddleRight->setRotateAngleX(angle);
    }
}

// ==================== 船渲染器 ====================

BoatRenderer::BoatRenderer(BoatType type)
    : m_type(type)
    , m_model(std::make_unique<BoatModel>())
{
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void BoatRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 BoatRenderer.render()
    // TODO: 实现船渲染
    // 1. 计算船的朝向和倾斜
    // 2. 处理受损抖动
    // 3. 渲染船模型
    // 4. 如果不在水中，渲染水花
    // 5. 划桨动画

    // 渲染船模型
    m_model->render(1.0 / 16.0);

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
    // 参考 MC 1.16.5 BoatRenderer
    // 计算船的摇晃角度
    // TODO: 需要访问 BoatEntity 的状态
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
    // 6个面：底部、左侧、右侧、前面、后面、内部底

    // sideModels[0] - 底部，旋转 PI/2
    m_sides[0] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBottom");
    m_sides[0]->setTextureSize(64, 32);
    m_sides[0]->setTextureOffset(0, 10);
    m_sides[0]->addBox(-10.0f, -8.0f, -1.0f, 20.0f, 16.0f, 2.0f, 0.0f);
    m_sides[0]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[0]->setRotateAngleX(static_cast<f32>(PI / 2.0));

    // sideModels[1] - 左侧，旋转 PI*1.5
    m_sides[1] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartLeft");
    m_sides[1]->setTextureSize(64, 32);
    m_sides[1]->setTextureOffset(0, 0);
    m_sides[1]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[1]->setRotationPoint(-9.0f, 4.0f, 0.0f);
    m_sides[1]->setRotateAngleY(static_cast<f32>(PI * 1.5));

    // sideModels[2] - 右侧，旋转 PI/2
    m_sides[2] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartRight");
    m_sides[2]->setTextureSize(64, 32);
    m_sides[2]->setTextureOffset(0, 0);
    m_sides[2]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[2]->setRotationPoint(9.0f, 4.0f, 0.0f);
    m_sides[2]->setRotateAngleY(static_cast<f32>(PI / 2.0));

    // sideModels[3] - 后面，旋转 PI
    m_sides[3] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBack");
    m_sides[3]->setTextureSize(64, 32);
    m_sides[3]->setTextureOffset(0, 0);
    m_sides[3]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[3]->setRotationPoint(0.0f, 4.0f, -7.0f);
    m_sides[3]->setRotateAngleY(static_cast<f32>(PI));

    // sideModels[4] - 前面
    m_sides[4] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartFront");
    m_sides[4]->setTextureSize(64, 32);
    m_sides[4]->setTextureOffset(0, 0);
    m_sides[4]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[4]->setRotationPoint(0.0f, 4.0f, 7.0f);

    // sideModels[5] - 内部底，旋转 -PI/2
    m_sides[5] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartInside");
    m_sides[5]->setTextureSize(64, 32);
    m_sides[5]->setTextureOffset(44, 10);
    m_sides[5]->addBox(-9.0f, -7.0f, -1.0f, 18.0f, 14.0f, 1.0f, 0.0f);
    m_sides[5]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[5]->setRotateAngleX(static_cast<f32>(-PI / 2.0));
}

void MinecartModel::render(f64 scale) {
    for (auto& side : m_sides) {
        if (side) {
            side->render(scale);
        }
    }
}

void MinecartModel::setInsideOffset(f32 yOffset) {
    if (m_sides[5]) {
        m_sides[5]->setRotationPointY(4.0f - yOffset);
    }
}

// ==================== 矿车渲染器 ====================

MinecartRenderer::MinecartRenderer()
    : m_model(std::make_unique<MinecartModel>())
{
    m_shadowSize = 0.0f;
    m_shadowAlpha = 0.0f;
}

void MinecartRenderer::render(Entity& entity, f64 partialTicks) {
    // 参考 MC 1.16.5 MinecartRenderer.render()
    // TODO: 实现矿车渲染
    // 1. 计算矿车方向和位置
    // 2. 处理受损抖动
    // 3. 渲染矿车模型
    // 4. 如果有内容物，渲染内容物（乘客、箱子、TNT等）

    // 矿车内部底板偏移动画（基于时间）
    // TODO: 从实体获取 tick 和动画状态
    m_model->setInsideOffset(0.0f);

    m_model->render(1.0 / 16.0);
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
