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

#include "VehicleRenderers.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc::client::renderer::entity::renderer::vehicle {

namespace {
using mc::math::PI;
using mc::math::PI_DOUBLE;
} // namespace

// ==================== 船模型 ====================

BoatModel::BoatModel()
    : m_textureWidth(128)
    , m_textureHeight(64)
{
    _setupParts();
}

void BoatModel::_setupParts()
{
    // 纹理尺寸：128x64

    // 创建5个船体面（底部、右、左、前、后）

    // 底部面 (amodelrenderer[0])
    m_bottom = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBottom");
    m_bottom->setTextureSize(128, 64);
    m_bottom->setTextureOffset(0, 0);
    m_bottom->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_bottom->setRotationPoint(0.0f, 3.0f, 1.0f);
    m_bottom->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));

    // 右侧面 (amodelrenderer[1]) - 旋转 PI_DOUBLE*1.5
    m_right = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatRight");
    m_right->setTextureSize(128, 64);
    m_right->setTextureOffset(0, 19);
    m_right->addBox(-13.0f, -7.0f, -1.0f, 18.0f, 6.0f, 2.0f, 0.0f);
    m_right->setRotationPoint(-15.0f, 4.0f, 4.0f);
    m_right->setRotateAngleY(static_cast<f32>(PI_DOUBLE * 1.5));

    // 左侧面 (amodelrenderer[2]) - 旋转 PI_DOUBLE/2
    m_left = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatLeft");
    m_left->setTextureSize(128, 64);
    m_left->setTextureOffset(0, 19);
    m_left->addBox(-8.0f, -7.0f, -1.0f, 16.0f, 6.0f, 2.0f, 0.0f);
    m_left->setRotationPoint(15.0f, 4.0f, 0.0f);
    m_left->setRotateAngleY(static_cast<f32>(PI_DOUBLE / 2.0));

    // 前面 (amodelrenderer[3]) - 旋转 PI_DOUBLE
    m_front = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatFront");
    m_front->setTextureSize(128, 64);
    m_front->setTextureOffset(0, 19);
    m_front->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_front->setRotationPoint(0.0f, 4.0f, -9.0f);
    m_front->setRotateAngleY(static_cast<f32>(PI_DOUBLE));

    // 后面 (amodelrenderer[4])
    m_back = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatBack");
    m_back->setTextureSize(128, 64);
    m_back->setTextureOffset(0, 19);
    m_back->addBox(-14.0f, -7.0f, -1.0f, 28.0f, 6.0f, 2.0f, 0.0f);
    m_back->setRotationPoint(0.0f, 4.0f, 9.0f);

    // 桨
    m_paddleLeft = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleLeft");
    m_paddleLeft->setTextureSize(128, 64);
    m_paddleLeft->setTextureOffset(62, 0);
    m_paddleLeft->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleLeft->addBox(-1.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleLeft->setRotationPoint(3.0f, -5.0f, 9.0f);
    m_paddleLeft->setRotateAngleZ(PI / 16.0f);

    m_paddleRight = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("paddleRight");
    m_paddleRight->setTextureSize(128, 64);
    m_paddleRight->setTextureOffset(62, 20);
    m_paddleRight->addBox(-1.0f, 0.0f, -5.0f, 2.0f, 2.0f, 18.0f, 0.0f);
    m_paddleRight->addBox(0.001f, -3.0f, 8.0f, 1.0f, 6.0f, 7.0f, 0.0f);
    m_paddleRight->setRotationPoint(3.0f, -5.0f, -9.0f);
    m_paddleRight->setRotateAngleY(static_cast<f32>(PI_DOUBLE));
    m_paddleRight->setRotateAngleZ(PI / 16.0f);

    // 水面以下不可见的底部
    m_noWater = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("boatNoWater");
    m_noWater->setTextureSize(128, 64);
    m_noWater->setTextureOffset(0, 0);
    m_noWater->addBox(-14.0f, -9.0f, -3.0f, 28.0f, 16.0f, 3.0f, 0.0f);
    m_noWater->setRotationPoint(0.0f, -3.0f, 1.0f);
    m_noWater->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));
}

void BoatModel::render(f64 scale)
{
    m_bottom->render(scale);
    m_right->render(scale);
    m_left->render(scale);
    m_front->render(scale);
    m_back->render(scale);
    m_paddleLeft->render(scale);
    m_paddleRight->render(scale);
}

void BoatModel::renderNoWater(f64 scale)
{
    m_noWater->render(scale);
}

void BoatModel::setPaddleAngle(i32 paddleIndex, f32 angle)
{
    if (paddleIndex == 0) {
        m_paddleLeft->setRotateAngleX(angle);
    } else if (paddleIndex == 1) {
        m_paddleRight->setRotateAngleX(angle);
    }
}

// ==================== 船渲染器 ====================

BoatRenderer::BoatRenderer(BoatType type)
    : m_type(type)
    , m_model(std::make_unique<BoatModel>())
{
    m_shadowSize = 0.8f;
    m_shadowAlpha = 0.8f;
}

void BoatRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 完整实现船渲染：计算朝向和倾斜、受损抖动、水面花效果、划桨动画
    m_model->render(1.0 / 16.0);

    (void)entity;
    (void)partialTicks;
}

ResourceLocation BoatRenderer::getTexture() const
{
    static const ResourceLocation textures[] = {ResourceLocation("minecraft", "textures/entity/boat/oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/spruce.png"),
        ResourceLocation("minecraft", "textures/entity/boat/birch.png"),
        ResourceLocation("minecraft", "textures/entity/boat/jungle.png"),
        ResourceLocation("minecraft", "textures/entity/boat/acacia.png"),
        ResourceLocation("minecraft", "textures/entity/boat/dark_oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/mangrove.png"),
        ResourceLocation("minecraft", "textures/entity/boat/cherry.png"),
        ResourceLocation("minecraft", "textures/entity/boat/pale_oak.png"),
        ResourceLocation("minecraft", "textures/entity/boat/bamboo.png")};
    return textures[static_cast<size_t>(m_type)];
}

f64 BoatRenderer::_calculateRockingAngle(::mc::entity::BoatEntity& boat, f64 partialTicks) const
{
    // TODO: 实现船的摇晃角度计算
    (void)boat;
    (void)partialTicks;
    return 0.0;
}

// ==================== 矿车模型 ====================

MinecartModel::MinecartModel()
{
    _setupParts();
}

void MinecartModel::_setupParts()
{
    // 纹理尺寸：64x32
    // 6个面：底部、左侧、右侧、前面、后面、内部底

    // sideModels[0] - 底部，旋转 PI_DOUBLE/2
    m_sides[0] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBottom");
    m_sides[0]->setTextureSize(64, 32);
    m_sides[0]->setTextureOffset(0, 10);
    m_sides[0]->addBox(-10.0f, -8.0f, -1.0f, 20.0f, 16.0f, 2.0f, 0.0f);
    m_sides[0]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[0]->setRotateAngleX(static_cast<f32>(PI_DOUBLE / 2.0));

    // sideModels[1] - 左侧，旋转 PI_DOUBLE*1.5
    m_sides[1] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartLeft");
    m_sides[1]->setTextureSize(64, 32);
    m_sides[1]->setTextureOffset(0, 0);
    m_sides[1]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[1]->setRotationPoint(-9.0f, 4.0f, 0.0f);
    m_sides[1]->setRotateAngleY(static_cast<f32>(PI_DOUBLE * 1.5));

    // sideModels[2] - 右侧，旋转 PI_DOUBLE/2
    m_sides[2] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartRight");
    m_sides[2]->setTextureSize(64, 32);
    m_sides[2]->setTextureOffset(0, 0);
    m_sides[2]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[2]->setRotationPoint(9.0f, 4.0f, 0.0f);
    m_sides[2]->setRotateAngleY(static_cast<f32>(PI_DOUBLE / 2.0));

    // sideModels[3] - 后面，旋转 PI_DOUBLE
    m_sides[3] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartBack");
    m_sides[3]->setTextureSize(64, 32);
    m_sides[3]->setTextureOffset(0, 0);
    m_sides[3]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[3]->setRotationPoint(0.0f, 4.0f, -7.0f);
    m_sides[3]->setRotateAngleY(static_cast<f32>(PI_DOUBLE));

    // sideModels[4] - 前面
    m_sides[4] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartFront");
    m_sides[4]->setTextureSize(64, 32);
    m_sides[4]->setTextureOffset(0, 0);
    m_sides[4]->addBox(-8.0f, -9.0f, -1.0f, 16.0f, 8.0f, 2.0f, 0.0f);
    m_sides[4]->setRotationPoint(0.0f, 4.0f, 7.0f);

    // sideModels[5] - 内部底，旋转 -PI_DOUBLE/2
    m_sides[5] = std::make_shared<::mc::client::renderer::entity::model::ModelRenderer>("minecartInside");
    m_sides[5]->setTextureSize(64, 32);
    m_sides[5]->setTextureOffset(44, 10);
    m_sides[5]->addBox(-9.0f, -7.0f, -1.0f, 18.0f, 14.0f, 1.0f, 0.0f);
    m_sides[5]->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_sides[5]->setRotateAngleX(static_cast<f32>(-PI_DOUBLE / 2.0));
}

void MinecartModel::render(f64 scale)
{
    for (auto& side : m_sides) {
        side->render(scale);
    }
}

void MinecartModel::setInsideOffset(f32 yOffset)
{
    m_sides[5]->setRotationPointY(4.0f - yOffset);
}

// ==================== 矿车渲染器 ====================

MinecartRenderer::MinecartRenderer()
    : m_model(std::make_unique<MinecartModel>())
{
    m_shadowSize = 0.5f;
    m_shadowAlpha = 0.8f;
}

void MinecartRenderer::render(Entity& entity, f64 partialTicks)
{
    // TODO: 完整实现矿车渲染：方向计算、受损抖动、内容物（乘客/箱子/TNT等）渲染
    // TODO: TNT矿车引燃闪烁效果：读取 ClientEntity::fuseTimer()，当 fuseTimer > 0 且 (fuseTimer / 5) % 2 == 0
    // 时渲染白色闪烁叠加层，
    //       闪烁缩放因子 = (1.0 - fuseTimer / 10.0)^4（参考 MC TntMinecartRenderer）
    m_model->setInsideOffset(0.0f);
    m_model->render(1.0 / 16.0);
    (void)entity;
    (void)partialTicks;
}

ResourceLocation MinecartRenderer::getMinecartTexture()
{
    return ResourceLocation("minecraft", "textures/entity/minecart.png");
}

void MinecartRenderer::_calculateCartDirection(::mc::entity::AbstractMinecartEntity& minecart, f64 partialTicks)
{
    // TODO: 实现矿车方向计算
    (void)minecart;
    (void)partialTicks;
}

} // namespace mc::client::renderer::entity::renderer::vehicle
