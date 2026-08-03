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

#include "BatModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::animal {

BatModel::BatModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void BatModel::setupParts()
{
    // 头部
    m_head = std::make_shared<ModelRenderer>("batHead");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.0f, -3.0f, -3.0f, 6.0f, 6.0f, 6.0f);
    m_parts.push_back(m_head);

    // 右耳 - 作为头部子部件
    m_rightEar = std::make_shared<ModelRenderer>("rightEar");
    m_rightEar->setTextureOffset(24, 0);
    m_rightEar->addBox(-4.0f, -6.0f, -2.0f, 3.0f, 4.0f, 1.0f);
    m_head->addChild(m_rightEar);

    // 左耳 - 镜像
    m_leftEar = std::make_shared<ModelRenderer>("leftEar");
    m_leftEar->setTextureOffset(24, 0);
    m_leftEar->setMirror(true);
    m_leftEar->addBox(1.0f, -6.0f, -2.0f, 3.0f, 4.0f, 1.0f);
    m_head->addChild(m_leftEar);

    // 身体
    m_body = std::make_shared<ModelRenderer>("batBody");
    m_body->setTextureOffset(0, 16);
    m_body->addBox(-3.0f, 4.0f, -3.0f, 6.0f, 12.0f, 6.0f);
    m_body->setTextureOffset(0, 34);
    m_body->addBox(-5.0f, 16.0f, 0.0f, 10.0f, 6.0f, 1.0f);
    m_parts.push_back(m_body);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("batRightWing");
    m_rightWing->setTextureOffset(42, 0);
    m_rightWing->addBox(-12.0f, 1.0f, 1.5f, 10.0f, 16.0f, 1.0f);
    m_body->addChild(m_rightWing);

    // 右外翼
    m_outerRightWing = std::make_shared<ModelRenderer>("batOuterRightWing");
    m_outerRightWing->setTextureOffset(24, 16);
    m_outerRightWing->setRotationPoint(-12.0f, 1.0f, 1.5f);
    m_outerRightWing->addBox(-8.0f, 1.0f, 0.0f, 8.0f, 12.0f, 1.0f);
    m_rightWing->addChild(m_outerRightWing);

    // 左翼 - 镜像
    m_leftWing = std::make_shared<ModelRenderer>("batLeftWing");
    m_leftWing->setTextureOffset(42, 0);
    m_leftWing->setMirror(true);
    m_leftWing->addBox(2.0f, 1.0f, 1.5f, 10.0f, 16.0f, 1.0f);
    m_body->addChild(m_leftWing);

    // 左外翼 - 镜像
    m_outerLeftWing = std::make_shared<ModelRenderer>("batOuterLeftWing");
    m_outerLeftWing->setTextureOffset(24, 16);
    m_outerLeftWing->setMirror(true);
    m_outerLeftWing->setRotationPoint(12.0f, 1.0f, 1.5f);
    m_outerLeftWing->addBox(0.0f, 1.0f, 0.0f, 8.0f, 12.0f, 1.0f);
    m_leftWing->addChild(m_outerLeftWing);
}

void BatModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void BatModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 有两种状态：悬挂 (isBatHanging) 和飞行

    if (m_isHanging) {
        // 悬挂状态
        m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
        m_head->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE - netHeadYaw * mc::math::PI_DOUBLE / 180.0));
        m_head->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE));
        m_head->setRotationPoint(0.0f, -2.0f, 0.0f);

        m_rightWing->setRotationPoint(-3.0f, 0.0f, 3.0f);
        m_leftWing->setRotationPoint(3.0f, 0.0f, 3.0f);

        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE));
        m_rightWing->setRotateAngleX(-0.15707964f);     // -mc::math::PI_DOUBLE / 20
        m_rightWing->setRotateAngleY(-1.2566371f);      // -mc::math::PI_DOUBLE * 0.4
        m_outerRightWing->setRotateAngleY(-1.7278761f); // -mc::math::PI_DOUBLE * 0.55

        m_leftWing->setRotateAngleX(-0.15707964f);    // -mc::math::PI_DOUBLE / 20
        m_leftWing->setRotateAngleY(1.2566371f);      // mc::math::PI_DOUBLE * 0.4
        m_outerLeftWing->setRotateAngleY(1.7278761f); // mc::math::PI_DOUBLE * 0.55
    } else {
        // 飞行状态
        m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
        m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
        m_head->setRotateAngleZ(0.0f);
        m_head->setRotationPoint(0.0f, 0.0f, 0.0f);

        m_rightWing->setRotationPoint(0.0f, 0.0f, 0.0f);
        m_leftWing->setRotationPoint(0.0f, 0.0f, 0.0f);

        // 身体俯仰 + 扇翅微动
        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 4.0 + std::cos(ageInTicks * 0.1) * 0.15));
        m_body->setRotateAngleY(0.0f);

        // 翅膀扇动角度
        f32 wingAngle = static_cast<f32>(std::cos(ageInTicks * 1.3) * mc::math::PI_DOUBLE * 0.25);
        m_rightWing->setRotateAngleY(wingAngle);
        m_leftWing->setRotateAngleY(-wingAngle);

        // 外翼角度为内翼的一半
        m_outerRightWing->setRotateAngleY(wingAngle * 0.5f);
        m_outerLeftWing->setRotateAngleY(-wingAngle * 0.5f);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

void BatModel::setHanging(bool hanging)
{
    m_isHanging = hanging;
}

} // namespace mc::client::renderer::entity::model::animal
