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

#include "PufferfishModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::aquatic {

// ==================== PufferfishSmallModel ====================

PufferfishSmallModel::PufferfishSmallModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void PufferfishSmallModel::_setupParts()
{
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 27);
    m_body->addBox(-1.5f, -2.0f, -1.5f, 3.0f, 2.0f, 3.0f);
    m_body->setRotationPoint(0.0f, 23.0f, 0.0f);

    m_rightEye = std::make_shared<ModelRenderer>("rightEye");
    m_rightEye->setTextureOffset(24, 6);
    m_rightEye->addBox(-1.5f, 0.0f, -1.5f, 1.0f, 1.0f, 1.0f);
    m_rightEye->setRotationPoint(0.0f, 20.0f, 0.0f);

    m_leftEye = std::make_shared<ModelRenderer>("leftEye");
    m_leftEye->setTextureOffset(28, 6);
    m_leftEye->addBox(0.5f, 0.0f, -1.5f, 1.0f, 1.0f, 1.0f);
    m_leftEye->setRotationPoint(0.0f, 20.0f, 0.0f);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(-3, 0);
    m_tail->addBox(-1.5f, 0.0f, 0.0f, 3.0f, 0.0f, 3.0f);
    m_tail->setRotationPoint(0.0f, 22.0f, 1.5f);

    m_rightFin = std::make_shared<ModelRenderer>("rightFin");
    m_rightFin->setTextureOffset(25, 0);
    m_rightFin->addBox(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 2.0f);
    m_rightFin->setRotationPoint(-1.5f, 22.0f, -1.5f);

    m_leftFin = std::make_shared<ModelRenderer>("leftFin");
    m_leftFin->setTextureOffset(25, 0);
    m_leftFin->addBox(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 2.0f);
    m_leftFin->setRotationPoint(1.5f, 22.0f, -1.5f);

    m_parts.push_back(m_body);
    m_parts.push_back(m_rightEye);
    m_parts.push_back(m_leftEye);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_rightFin);
    m_parts.push_back(m_leftFin);
}

void PufferfishSmallModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 ageInTicks, f64 /*netHeadYaw*/, f64 /*headPitch*/, f64 /*scale*/)
{
    // 鳍摆动动画
    f32 finAngle = -0.2f + 0.4f * static_cast<f32>(std::sin(ageInTicks * 0.2));
    m_rightFin->setRotateAngleZ(finAngle);
    m_leftFin->setRotateAngleZ(-finAngle);
}

void PufferfishSmallModel::render(f64 scale)
{
    EntityModel::render(scale);
}

// ==================== PufferfishMediumModel ====================

PufferfishMediumModel::PufferfishMediumModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void PufferfishMediumModel::_setupParts()
{
    constexpr f32 PI_4 = mc::math::QUARTER_PI;

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(12, 22);
    m_body->addBox(-2.5f, -5.0f, -2.5f, 5.0f, 5.0f, 5.0f);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_rightFin = std::make_shared<ModelRenderer>("rightFin");
    m_rightFin->setTextureOffset(24, 0);
    m_rightFin->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_rightFin->setRotationPoint(-2.5f, 17.0f, -1.5f);

    m_leftFin = std::make_shared<ModelRenderer>("leftFin");
    m_leftFin->setTextureOffset(24, 3);
    m_leftFin->addBox(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_leftFin->setRotationPoint(2.5f, 17.0f, -1.5f);

    m_frontTopSpines = std::make_shared<ModelRenderer>("frontTopSpines");
    m_frontTopSpines->setTextureOffset(15, 16);
    m_frontTopSpines->addBox(-2.5f, -1.0f, 0.0f, 5.0f, 1.0f, 1.0f);
    m_frontTopSpines->setRotationPoint(0.0f, 17.0f, -2.5f);
    m_frontTopSpines->setRotateAngleX(PI_4);

    m_backTopSpines = std::make_shared<ModelRenderer>("backTopSpines");
    m_backTopSpines->setTextureOffset(10, 16);
    m_backTopSpines->addBox(-2.5f, -1.0f, -1.0f, 5.0f, 1.0f, 1.0f);
    m_backTopSpines->setRotationPoint(0.0f, 17.0f, 2.5f);
    m_backTopSpines->setRotateAngleX(-PI_4);

    m_frontRightSpines = std::make_shared<ModelRenderer>("frontRightSpines");
    m_frontRightSpines->setTextureOffset(8, 16);
    m_frontRightSpines->addBox(-1.0f, -5.0f, 0.0f, 1.0f, 5.0f, 1.0f);
    m_frontRightSpines->setRotationPoint(-2.5f, 22.0f, -2.5f);
    m_frontRightSpines->setRotateAngleY(-PI_4);

    m_backRightSpines = std::make_shared<ModelRenderer>("backRightSpines");
    m_backRightSpines->setTextureOffset(8, 16);
    m_backRightSpines->addBox(-1.0f, -5.0f, 0.0f, 1.0f, 5.0f, 1.0f);
    m_backRightSpines->setRotationPoint(-2.5f, 22.0f, 2.5f);
    m_backRightSpines->setRotateAngleY(PI_4);

    m_backLeftSpines = std::make_shared<ModelRenderer>("backLeftSpines");
    m_backLeftSpines->setTextureOffset(4, 16);
    m_backLeftSpines->addBox(0.0f, -5.0f, 0.0f, 1.0f, 5.0f, 1.0f);
    m_backLeftSpines->setRotationPoint(2.5f, 22.0f, 2.5f);
    m_backLeftSpines->setRotateAngleY(-PI_4);

    m_frontLeftSpines = std::make_shared<ModelRenderer>("frontLeftSpines");
    m_frontLeftSpines->setTextureOffset(0, 16);
    m_frontLeftSpines->addBox(0.0f, -5.0f, 0.0f, 1.0f, 5.0f, 1.0f);
    m_frontLeftSpines->setRotationPoint(2.5f, 22.0f, -2.5f);
    m_frontLeftSpines->setRotateAngleY(PI_4);

    m_backBottomSpine = std::make_shared<ModelRenderer>("backBottomSpine");
    m_backBottomSpine->setTextureOffset(8, 22);
    m_backBottomSpine->addBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_backBottomSpine->setRotationPoint(0.5f, 22.0f, 2.5f);
    m_backBottomSpine->setRotateAngleX(PI_4);

    m_frontBottomSpines = std::make_shared<ModelRenderer>("frontBottomSpines");
    m_frontBottomSpines->setTextureOffset(17, 21);
    m_frontBottomSpines->addBox(-2.5f, 0.0f, 0.0f, 5.0f, 1.0f, 1.0f);
    m_frontBottomSpines->setRotationPoint(0.0f, 22.0f, -2.5f);
    m_frontBottomSpines->setRotateAngleX(-PI_4);

    m_parts.push_back(m_body);
    m_parts.push_back(m_rightFin);
    m_parts.push_back(m_leftFin);
    m_parts.push_back(m_frontTopSpines);
    m_parts.push_back(m_backTopSpines);
    m_parts.push_back(m_frontRightSpines);
    m_parts.push_back(m_backRightSpines);
    m_parts.push_back(m_backLeftSpines);
    m_parts.push_back(m_frontLeftSpines);
    m_parts.push_back(m_backBottomSpine);
    m_parts.push_back(m_frontBottomSpines);
}

void PufferfishMediumModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 ageInTicks, f64 /*netHeadYaw*/, f64 /*headPitch*/, f64 /*scale*/)
{
    f32 finAngle = -0.2f + 0.4f * static_cast<f32>(std::sin(ageInTicks * 0.2));
    m_rightFin->setRotateAngleZ(finAngle);
    m_leftFin->setRotateAngleZ(-finAngle);
}

void PufferfishMediumModel::render(f64 scale)
{
    EntityModel::render(scale);
}

// ==================== PufferfishBigModel ====================

PufferfishBigModel::PufferfishBigModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void PufferfishBigModel::_setupParts()
{
    constexpr f32 PI_4 = mc::math::QUARTER_PI;

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_rightFin = std::make_shared<ModelRenderer>("rightFin");
    m_rightFin->setTextureOffset(24, 0);
    m_rightFin->addBox(-2.0f, 0.0f, -1.0f, 2.0f, 1.0f, 2.0f);
    m_rightFin->setRotationPoint(-4.0f, 15.0f, -2.0f);

    m_leftFin = std::make_shared<ModelRenderer>("leftFin");
    m_leftFin->setTextureOffset(24, 3);
    m_leftFin->addBox(0.0f, 0.0f, -1.0f, 2.0f, 1.0f, 2.0f);
    m_leftFin->setRotationPoint(4.0f, 15.0f, -2.0f);

    m_frontTopSpines = std::make_shared<ModelRenderer>("frontTopSpines");
    m_frontTopSpines->setTextureOffset(15, 17);
    m_frontTopSpines->addBox(-4.0f, -1.0f, 0.0f, 8.0f, 1.0f, 0.0f);
    m_frontTopSpines->setRotationPoint(0.0f, 14.0f, -4.0f);
    m_frontTopSpines->setRotateAngleX(PI_4);

    m_topMidSpines = std::make_shared<ModelRenderer>("topMidSpines");
    m_topMidSpines->setTextureOffset(14, 16);
    m_topMidSpines->addBox(-4.0f, -1.0f, 0.0f, 8.0f, 1.0f, 1.0f);
    m_topMidSpines->setRotationPoint(0.0f, 14.0f, 0.0f);

    m_backTopSpines = std::make_shared<ModelRenderer>("backTopSpines");
    m_backTopSpines->setTextureOffset(23, 18);
    m_backTopSpines->addBox(-4.0f, -1.0f, 0.0f, 8.0f, 1.0f, 0.0f);
    m_backTopSpines->setRotationPoint(0.0f, 14.0f, 4.0f);
    m_backTopSpines->setRotateAngleX(-PI_4);

    m_frontRightSpines = std::make_shared<ModelRenderer>("frontRightSpines");
    m_frontRightSpines->setTextureOffset(5, 17);
    m_frontRightSpines->addBox(-1.0f, -8.0f, 0.0f, 1.0f, 8.0f, 0.0f);
    m_frontRightSpines->setRotationPoint(-4.0f, 22.0f, -4.0f);
    m_frontRightSpines->setRotateAngleY(-PI_4);

    m_frontLeftSpines = std::make_shared<ModelRenderer>("frontLeftSpines");
    m_frontLeftSpines->setTextureOffset(1, 17);
    m_frontLeftSpines->addBox(0.0f, -8.0f, 0.0f, 1.0f, 8.0f, 0.0f);
    m_frontLeftSpines->setRotationPoint(4.0f, 22.0f, -4.0f);
    m_frontLeftSpines->setRotateAngleY(PI_4);

    m_frontBottomSpines = std::make_shared<ModelRenderer>("frontBottomSpines");
    m_frontBottomSpines->setTextureOffset(15, 20);
    m_frontBottomSpines->addBox(-4.0f, 0.0f, 0.0f, 8.0f, 1.0f, 0.0f);
    m_frontBottomSpines->setRotationPoint(0.0f, 22.0f, -4.0f);
    m_frontBottomSpines->setRotateAngleX(-PI_4);

    m_bottomMidSpines = std::make_shared<ModelRenderer>("bottomMidSpines");
    m_bottomMidSpines->setTextureOffset(15, 20);
    m_bottomMidSpines->addBox(-4.0f, 0.0f, 0.0f, 8.0f, 1.0f, 0.0f);
    m_bottomMidSpines->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_bottomBackSpines = std::make_shared<ModelRenderer>("bottomBackSpines");
    m_bottomBackSpines->setTextureOffset(15, 20);
    m_bottomBackSpines->addBox(-4.0f, 0.0f, 0.0f, 8.0f, 1.0f, 0.0f);
    m_bottomBackSpines->setRotationPoint(0.0f, 22.0f, 4.0f);
    m_bottomBackSpines->setRotateAngleX(PI_4);

    m_backRightSpines = std::make_shared<ModelRenderer>("backRightSpines");
    m_backRightSpines->setTextureOffset(9, 17);
    m_backRightSpines->addBox(-1.0f, -8.0f, 0.0f, 1.0f, 8.0f, 0.0f);
    m_backRightSpines->setRotationPoint(-4.0f, 22.0f, 4.0f);
    m_backRightSpines->setRotateAngleY(PI_4);

    m_backLeftSpines = std::make_shared<ModelRenderer>("backLeftSpines");
    m_backLeftSpines->setTextureOffset(9, 17);
    m_backLeftSpines->addBox(0.0f, -8.0f, 0.0f, 1.0f, 8.0f, 0.0f);
    m_backLeftSpines->setRotationPoint(4.0f, 22.0f, 4.0f);
    m_backLeftSpines->setRotateAngleY(-PI_4);

    m_parts.push_back(m_body);
    m_parts.push_back(m_rightFin);
    m_parts.push_back(m_leftFin);
    m_parts.push_back(m_frontTopSpines);
    m_parts.push_back(m_topMidSpines);
    m_parts.push_back(m_backTopSpines);
    m_parts.push_back(m_frontRightSpines);
    m_parts.push_back(m_frontLeftSpines);
    m_parts.push_back(m_frontBottomSpines);
    m_parts.push_back(m_bottomMidSpines);
    m_parts.push_back(m_bottomBackSpines);
    m_parts.push_back(m_backRightSpines);
    m_parts.push_back(m_backLeftSpines);
}

void PufferfishBigModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 ageInTicks, f64 /*netHeadYaw*/, f64 /*headPitch*/, f64 /*scale*/)
{
    // 鳍摆动动画（与小型模型相同）
    f32 finAngle = -0.2f + 0.4f * static_cast<f32>(std::sin(ageInTicks * 0.2));
    m_rightFin->setRotateAngleZ(finAngle);
    m_leftFin->setRotateAngleZ(-finAngle);
}

void PufferfishBigModel::render(f64 scale)
{
    EntityModel::render(scale);
}

} // namespace mc::client::renderer::entity::model::aquatic
