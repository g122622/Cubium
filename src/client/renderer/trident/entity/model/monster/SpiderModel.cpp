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

#include "SpiderModel.hpp"

#include <cmath>
#include <memory>

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc::client::renderer::entity::model::monster {

SpiderModel::SpiderModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void SpiderModel::_setupParts()
{
    // 纹理尺寸：64x32

    // 头部：8x8x8，纹理位置 (32, 4)，位置 (0, 15, -3)
    m_head = std::make_shared<ModelRenderer>("spiderHead");
    m_head->setTextureSize(64, 32);
    m_head->setTextureOffset(32, 4);
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f, 0.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -3.0f);
    m_parts.push_back(m_head);

    // 颈部：6x6x6，纹理位置 (0, 0)，位置 (0, 15, 0)
    m_neck = std::make_shared<ModelRenderer>("spiderNeck");
    m_neck->setTextureSize(64, 32);
    m_neck->setTextureOffset(0, 0);
    m_neck->addBox(-3.0f, -3.0f, -3.0f, 6.0f, 6.0f, 6.0f, 0.0f);
    m_neck->setRotationPoint(0.0f, 15.0f, 0.0f);
    m_parts.push_back(m_neck);

    // 身体：10x8x12，纹理位置 (0, 12)，位置 (0, 15, 9)
    m_body = std::make_shared<ModelRenderer>("spiderBody");
    m_body->setTextureSize(64, 32);
    m_body->setTextureOffset(0, 12);
    m_body->addBox(-5.0f, -4.0f, -6.0f, 10.0f, 8.0f, 12.0f, 0.0f);
    m_body->setRotationPoint(0.0f, 15.0f, 9.0f);
    m_parts.push_back(m_body);

    // 8 条腿，纹理位置 (18, 0)
    // 腿部尺寸：16x2x2

    // 腿1（右前上）- 位置 (-4, 15, 2)
    m_legs[0] = std::make_shared<ModelRenderer>("spiderLeg1");
    m_legs[0]->setTextureSize(64, 32);
    m_legs[0]->setTextureOffset(18, 0);
    m_legs[0]->addBox(-15.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[0]->setRotationPoint(-4.0f, 15.0f, 2.0f);
    m_parts.push_back(m_legs[0]);

    // 腿2（左前上）- 位置 (4, 15, 2)
    m_legs[1] = std::make_shared<ModelRenderer>("spiderLeg2");
    m_legs[1]->setTextureSize(64, 32);
    m_legs[1]->setTextureOffset(18, 0);
    m_legs[1]->addBox(-1.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[1]->setRotationPoint(4.0f, 15.0f, 2.0f);
    m_parts.push_back(m_legs[1]);

    // 腿3（右前下）- 位置 (-4, 15, 1)
    m_legs[2] = std::make_shared<ModelRenderer>("spiderLeg3");
    m_legs[2]->setTextureSize(64, 32);
    m_legs[2]->setTextureOffset(18, 0);
    m_legs[2]->addBox(-15.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[2]->setRotationPoint(-4.0f, 15.0f, 1.0f);
    m_parts.push_back(m_legs[2]);

    // 腿4（左前下）- 位置 (4, 15, 1)
    m_legs[3] = std::make_shared<ModelRenderer>("spiderLeg4");
    m_legs[3]->setTextureSize(64, 32);
    m_legs[3]->setTextureOffset(18, 0);
    m_legs[3]->addBox(-1.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[3]->setRotationPoint(4.0f, 15.0f, 1.0f);
    m_parts.push_back(m_legs[3]);

    // 腿5（右后上）- 位置 (-4, 15, 0)
    m_legs[4] = std::make_shared<ModelRenderer>("spiderLeg5");
    m_legs[4]->setTextureSize(64, 32);
    m_legs[4]->setTextureOffset(18, 0);
    m_legs[4]->addBox(-15.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[4]->setRotationPoint(-4.0f, 15.0f, 0.0f);
    m_parts.push_back(m_legs[4]);

    // 腿6（左后上）- 位置 (4, 15, 0)
    m_legs[5] = std::make_shared<ModelRenderer>("spiderLeg6");
    m_legs[5]->setTextureSize(64, 32);
    m_legs[5]->setTextureOffset(18, 0);
    m_legs[5]->addBox(-1.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[5]->setRotationPoint(4.0f, 15.0f, 0.0f);
    m_parts.push_back(m_legs[5]);

    // 腿7（右后下）- 位置 (-4, 15, -1)
    m_legs[6] = std::make_shared<ModelRenderer>("spiderLeg7");
    m_legs[6]->setTextureSize(64, 32);
    m_legs[6]->setTextureOffset(18, 0);
    m_legs[6]->addBox(-15.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[6]->setRotationPoint(-4.0f, 15.0f, -1.0f);
    m_parts.push_back(m_legs[6]);

    // 腿8（左后下）- 位置 (4, 15, -1)
    m_legs[7] = std::make_shared<ModelRenderer>("spiderLeg8");
    m_legs[7]->setTextureSize(64, 32);
    m_legs[7]->setTextureOffset(18, 0);
    m_legs[7]->addBox(-1.0f, -1.0f, -1.0f, 16.0f, 2.0f, 2.0f, 0.0f);
    m_legs[7]->setRotationPoint(4.0f, 15.0f, -1.0f);
    m_parts.push_back(m_legs[7]);
}

void SpiderModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SpiderModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 头部旋转
    m_head->setRotateAngleY(netHeadYaw * mc::math::PI_DOUBLE / 180.0);
    m_head->setRotateAngleX(headPitch * mc::math::PI_DOUBLE / 180.0);

    // 腿部基础角度
    // Z 轴旋转（向外张开）
    m_legs[0]->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)); // 腿1
    m_legs[1]->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));  // 腿2
    m_legs[2]->setRotateAngleZ(-0.58119464f);                                 // 腿3
    m_legs[3]->setRotateAngleZ(0.58119464f);                                  // 腿4
    m_legs[4]->setRotateAngleZ(-0.58119464f);                                 // 腿5
    m_legs[5]->setRotateAngleZ(0.58119464f);                                  // 腿6
    m_legs[6]->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)); // 腿7
    m_legs[7]->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));  // 腿8

    // Y 轴旋转（前后摆动基础）
    m_legs[0]->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));  // 腿1
    m_legs[1]->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)); // 腿2
    m_legs[2]->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 8.0));  // 腿3
    m_legs[3]->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 8.0)); // 腿4
    m_legs[4]->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 8.0)); // 腿5
    m_legs[5]->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 8.0));  // 腿6
    m_legs[6]->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)); // 腿7
    m_legs[7]->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));  // 腿8

    // 腿部摆动动画
    f32 f1 = -static_cast<f32>(std::cos(limbSwing * 0.6662 * 2.0 + 0.0) * 0.4) * static_cast<f32>(limbSwingAmount);
    f32 f2 = -static_cast<f32>(std::cos(limbSwing * 0.6662 * 2.0 + mc::math::PI_DOUBLE) * 0.4) *
        static_cast<f32>(limbSwingAmount);
    f32 f3 = -static_cast<f32>(std::cos(limbSwing * 0.6662 * 2.0 + mc::math::PI_DOUBLE / 2.0) * 0.4) *
        static_cast<f32>(limbSwingAmount);
    f32 f4 = -static_cast<f32>(std::cos(limbSwing * 0.6662 * 2.0 + mc::math::PI_DOUBLE * 1.5) * 0.4) *
        static_cast<f32>(limbSwingAmount);

    f32 f5 = static_cast<f32>(std::abs(std::sin(limbSwing * 0.6662 + 0.0) * 0.4)) * static_cast<f32>(limbSwingAmount);
    f32 f6 = static_cast<f32>(std::abs(std::sin(limbSwing * 0.6662 + mc::math::PI_DOUBLE) * 0.4)) *
        static_cast<f32>(limbSwingAmount);
    f32 f7 = static_cast<f32>(std::abs(std::sin(limbSwing * 0.6662 + mc::math::PI_DOUBLE / 2.0) * 0.4)) *
        static_cast<f32>(limbSwingAmount);
    f32 f8 = static_cast<f32>(std::abs(std::sin(limbSwing * 0.6662 + mc::math::PI_DOUBLE * 1.5) * 0.4)) *
        static_cast<f32>(limbSwingAmount);

    // Y 轴摆动
    m_legs[0]->setRotateAngleY(m_legs[0]->rotateAngleY() + f1);
    m_legs[1]->setRotateAngleY(m_legs[1]->rotateAngleY() - f1);
    m_legs[2]->setRotateAngleY(m_legs[2]->rotateAngleY() + f2);
    m_legs[3]->setRotateAngleY(m_legs[3]->rotateAngleY() - f2);
    m_legs[4]->setRotateAngleY(m_legs[4]->rotateAngleY() + f3);
    m_legs[5]->setRotateAngleY(m_legs[5]->rotateAngleY() - f3);
    m_legs[6]->setRotateAngleY(m_legs[6]->rotateAngleY() + f4);
    m_legs[7]->setRotateAngleY(m_legs[7]->rotateAngleY() - f4);

    // Z 轴摆动
    m_legs[0]->setRotateAngleZ(m_legs[0]->rotateAngleZ() + f5);
    m_legs[1]->setRotateAngleZ(m_legs[1]->rotateAngleZ() - f5);
    m_legs[2]->setRotateAngleZ(m_legs[2]->rotateAngleZ() + f6);
    m_legs[3]->setRotateAngleZ(m_legs[3]->rotateAngleZ() - f6);
    m_legs[4]->setRotateAngleZ(m_legs[4]->rotateAngleZ() + f7);
    m_legs[5]->setRotateAngleZ(m_legs[5]->rotateAngleZ() - f7);
    m_legs[6]->setRotateAngleZ(m_legs[6]->rotateAngleZ() + f8);
    m_legs[7]->setRotateAngleZ(m_legs[7]->rotateAngleZ() - f8);

    (void)ageInTicks; // 蜘蛛不使用 ageInTicks
    (void)scale;      // 已在 render() 中使用
}

} // namespace mc::client::renderer::entity::model::monster
