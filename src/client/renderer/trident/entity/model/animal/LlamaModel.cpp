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

#include "LlamaModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

LlamaModel::LlamaModel(f32 scale)
    : AgeableModel(true, 16.2f, 1.36f, 2.7272f, 2.0f, 20.0f) // 使用与马类似的幼体参数
    , m_scale(scale)
{
    // 纹理尺寸：128x64
    setTextureSize(128, 64);

    // ==================== 头部 ====================
    // 头部基础盒子：位置 (-2, -14, -10)，尺寸 4x4x9
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.0f, -14.0f, -10.0f, 4.0f, 4.0f, 9.0f, static_cast<f64>(scale));
    m_head->setRotationPoint(0.0f, 7.0f, -6.0f);

    // 头部延伸部分（颈部）
    auto headExtension = m_head->createChild("headExtension");
    headExtension->setTextureOffset(0, 14);
    headExtension->addBox(-4.0f, -16.0f, -6.0f, 8.0f, 18.0f, 6.0f, static_cast<f64>(scale));

    // 左耳
    auto leftEar = m_head->createChild("leftEar");
    leftEar->setTextureOffset(17, 0);
    leftEar->addBox(-4.0f, -19.0f, -4.0f, 3.0f, 3.0f, 2.0f, static_cast<f64>(scale));

    // 右耳
    auto rightEar = m_head->createChild("rightEar");
    rightEar->setTextureOffset(17, 0);
    rightEar->addBox(1.0f, -19.0f, -4.0f, 3.0f, 3.0f, 2.0f, static_cast<f64>(scale));

    // ==================== 身体 ====================
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(29, 0);
    m_body->addBox(-6.0f, -10.0f, -7.0f, 12.0f, 18.0f, 10.0f, static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);
    m_body->setRotateAngleX(mc::math::HALF_PI);

    // ==================== 腿部 ====================
    // 后右腿
    m_backRightLeg = std::make_shared<ModelRenderer>("backRightLeg");
    m_backRightLeg->setTextureOffset(29, 29);
    m_backRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_backRightLeg->setRotationPoint(-2.5f, 10.0f, 6.0f);

    // 后左腿
    m_backLeftLeg = std::make_shared<ModelRenderer>("backLeftLeg");
    m_backLeftLeg->setTextureOffset(29, 29);
    m_backLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_backLeftLeg->setRotationPoint(2.5f, 10.0f, 6.0f);

    // 前右腿
    m_frontRightLeg = std::make_shared<ModelRenderer>("frontRightLeg");
    m_frontRightLeg->setTextureOffset(29, 29);
    m_frontRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_frontRightLeg->setRotationPoint(-2.5f, 10.0f, -4.0f);

    // 前左腿
    m_frontLeftLeg = std::make_shared<ModelRenderer>("frontLeftLeg");
    m_frontLeftLeg->setTextureOffset(29, 29);
    m_frontLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_frontLeftLeg->setRotationPoint(2.5f, 10.0f, -4.0f);

    // ==================== 箱子部件 ====================
    // 仅在成年且有箱子时显示

    // 左侧箱子
    m_chest1 = std::make_shared<ModelRenderer>("chest1");
    m_chest1->setTextureOffset(45, 28);
    m_chest1->addBox(-3.0f, 0.0f, 0.0f, 8.0f, 8.0f, 3.0f, static_cast<f64>(scale));
    m_chest1->setRotationPoint(-8.5f, 3.0f, 3.0f);
    m_chest1->setRotateAngleY(mc::math::HALF_PI);
    m_body->addChild(m_chest1);
    m_chest1->setVisible(false);

    // 右侧箱子
    m_chest2 = std::make_shared<ModelRenderer>("chest2");
    m_chest2->setTextureOffset(45, 41);
    m_chest2->addBox(-3.0f, 0.0f, 0.0f, 8.0f, 8.0f, 3.0f, static_cast<f64>(scale));
    m_chest2->setRotationPoint(5.5f, 3.0f, 3.0f);
    m_chest2->setRotateAngleY(mc::math::HALF_PI);
    m_body->addChild(m_chest2);
    m_chest2->setVisible(false);
}

std::vector<std::shared_ptr<ModelRenderer>> LlamaModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> LlamaModel::getBodyParts() const
{
    return {m_body, m_backRightLeg, m_backLeftLeg, m_frontRightLeg, m_frontLeftLeg};
}

void LlamaModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void LlamaModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 /*ageInTicks*/, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 头部旋转
    m_head->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));
    m_head->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));

    // 身体旋转（站立姿态）
    m_body->setRotateAngleX(mc::math::HALF_PI);

    // 腿部行走动画
    f32 walkAngle = static_cast<f32>(limbSwing * 0.6662);
    f32 walkAmount = static_cast<f32>(limbSwingAmount * 1.4);

    m_backRightLeg->setRotateAngleX(std::cos(walkAngle) * walkAmount);
    m_backLeftLeg->setRotateAngleX(std::cos(walkAngle + mc::math::PI) * walkAmount);
    m_frontRightLeg->setRotateAngleX(std::cos(walkAngle + mc::math::PI) * walkAmount);
    m_frontLeftLeg->setRotateAngleX(std::cos(walkAngle) * walkAmount);

    // 箱子可见性
    bool showChest = !m_isChild && m_hasChest;
    m_chest1->setVisible(showChest);
    m_chest2->setVisible(showChest);
}

} // namespace mc::client::renderer::entity::model::animal
