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

#include "OcelotModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::animal {

namespace {

/// 肢体摆动频率系数
constexpr f64 LIMB_SWING_FREQ = 0.6662;

/// 肢体摆动相位偏移
constexpr f64 LIMB_PHASE_OFFSET = 0.3;

/// 尾巴基础旋转角度（弧度）
constexpr f64 TAIL_BASE_ANGLE = 1.7278761;

/// 非站立状态下尾巴摆动振幅（弧度）
constexpr f64 TAIL_CROUCH_AMPLITUDE = 0.47123894;

} // namespace

OcelotModel::OcelotModel(f32 scale)
    : AgeableModel()
{
    setTextureSize(64, 32);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    // 主头部盒子 (textureOffset 0, 0)
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.5f, -2.0f, -3.0f, 5.0f, 4.0f, 5.0f, static_cast<f64>(scale));
    // 鼻子 (textureOffset 0, 24)
    m_head->setTextureOffset(0, 24);
    m_head->addBox(-1.5f, 0.0f, -4.0f, 3.0f, 2.0f, 2.0f, static_cast<f64>(scale));
    // 左耳 (textureOffset 0, 10)
    m_head->setTextureOffset(0, 10);
    m_head->addBox(-2.0f, -3.0f, 0.0f, 1.0f, 1.0f, 2.0f, static_cast<f64>(scale));
    // 右耳 (textureOffset 6, 10)
    m_head->setTextureOffset(6, 10);
    m_head->addBox(1.0f, -3.0f, 0.0f, 1.0f, 1.0f, 2.0f, static_cast<f64>(scale));
    m_head->setRotationPoint(0.0f, 15.0f, -9.0f);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(20, 0);
    m_body->addBox(-2.0f, 3.0f, -8.0f, 4.0f, 16.0f, 6.0f, static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 12.0f, -10.0f);

    // 尾巴1
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(0, 15);
    m_tail->addBox(-0.5f, 0.0f, 0.0f, 1.0f, 8.0f, 1.0f, static_cast<f64>(scale));
    m_tail->setRotateAngleX(0.9f);
    m_tail->setRotationPoint(0.0f, 15.0f, 8.0f);

    // 尾巴2
    m_tail2 = std::make_shared<ModelRenderer>("tail2");
    m_tail2->setTextureOffset(4, 15);
    m_tail2->addBox(-0.5f, 0.0f, 0.0f, 1.0f, 8.0f, 1.0f, static_cast<f64>(scale));
    m_tail2->setRotationPoint(0.0f, 20.0f, 14.0f);

    // 后左腿
    m_backLeftLeg = std::make_shared<ModelRenderer>("backLeftLeg");
    m_backLeftLeg->setTextureOffset(8, 13);
    m_backLeftLeg->addBox(-1.0f, 0.0f, 1.0f, 2.0f, 6.0f, 2.0f, static_cast<f64>(scale));
    m_backLeftLeg->setRotationPoint(1.1f, 18.0f, 5.0f);

    // 后右腿
    m_backRightLeg = std::make_shared<ModelRenderer>("backRightLeg");
    m_backRightLeg->setTextureOffset(8, 13);
    m_backRightLeg->addBox(-1.0f, 0.0f, 1.0f, 2.0f, 6.0f, 2.0f, static_cast<f64>(scale));
    m_backRightLeg->setRotationPoint(-1.1f, 18.0f, 5.0f);

    // 前左腿
    m_frontLeftLeg = std::make_shared<ModelRenderer>("frontLeftLeg");
    m_frontLeftLeg->setTextureOffset(40, 0);
    m_frontLeftLeg->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 10.0f, 2.0f, static_cast<f64>(scale));
    m_frontLeftLeg->setRotationPoint(1.2f, 14.1f, -5.0f);

    // 前右腿
    m_frontRightLeg = std::make_shared<ModelRenderer>("frontRightLeg");
    m_frontRightLeg->setTextureOffset(40, 0);
    m_frontRightLeg->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 10.0f, 2.0f, static_cast<f64>(scale));
    m_frontRightLeg->setRotationPoint(-1.2f, 14.1f, -5.0f);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_backLeftLeg);
    m_parts.push_back(m_backRightLeg);
    m_parts.push_back(m_frontLeftLeg);
    m_parts.push_back(m_frontRightLeg);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_tail2);
}

void OcelotModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void OcelotModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 重置所有部件到默认位置
    m_body->setRotationPointY(12.0f);
    m_body->setRotationPointZ(-10.0f);
    m_head->setRotationPointY(15.0f);
    m_head->setRotationPointZ(-9.0f);
    m_tail->setRotationPointY(15.0f);
    m_tail->setRotationPointZ(8.0f);
    m_tail2->setRotationPointY(20.0f);
    m_tail2->setRotationPointZ(14.0f);
    m_frontLeftLeg->setRotationPointY(14.1f);
    m_frontLeftLeg->setRotationPointZ(-5.0f);
    m_frontRightLeg->setRotationPointY(14.1f);
    m_frontRightLeg->setRotationPointZ(-5.0f);
    m_backLeftLeg->setRotationPointY(18.0f);
    m_backLeftLeg->setRotationPointZ(5.0f);
    m_backRightLeg->setRotationPointY(18.0f);
    m_backRightLeg->setRotationPointZ(5.0f);
    m_tail->setRotateAngleX(0.9f);

    if (m_isCrouching) {
        // 蹲伏状态
        m_body->setRotationPointY(m_body->rotationPointY() + 1.0f);
        m_head->setRotationPointY(m_head->rotationPointY() + 2.0f);
        m_tail->setRotationPointY(m_tail->rotationPointY() + 1.0f);
        m_tail2->setRotationPointY(m_tail2->rotationPointY() - 4.0f);
        m_tail2->setRotationPointZ(m_tail2->rotationPointZ() + 2.0f);
        m_tail->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
        m_tail2->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
        m_state = 0;
    } else if (m_isSprinting) {
        // 奔跑状态
        m_tail2->setRotationPointY(m_tail->rotationPointY());
        m_tail2->setRotationPointZ(m_tail2->rotationPointZ() + 2.0f);
        m_tail->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
        m_tail2->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
        m_state = 2;
    } else {
        // 站立状态
        m_state = 1;
    }
}

void OcelotModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));

    if (m_state != 3) {
        // 身体水平
        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));

        if (m_state == 2) {
            // 奔跑动画
            m_backLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ) * limbSwingAmount));
            m_backRightLeg->setRotateAngleX(
                static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ + LIMB_PHASE_OFFSET) * limbSwingAmount));
            m_frontLeftLeg->setRotateAngleX(static_cast<f32>(
                std::cos(limbSwing * LIMB_SWING_FREQ + mc::math::PI_DOUBLE + LIMB_PHASE_OFFSET) * limbSwingAmount));
            m_frontRightLeg->setRotateAngleX(
                static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ + mc::math::PI_DOUBLE) * limbSwingAmount));
            m_tail2->setRotateAngleX(static_cast<f32>(
                TAIL_BASE_ANGLE + (mc::math::PI_DOUBLE / 10.0) * std::cos(limbSwing) * limbSwingAmount));
        } else {
            // 行走动画
            m_backLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ) * limbSwingAmount));
            m_backRightLeg->setRotateAngleX(
                static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ + mc::math::PI_DOUBLE) * limbSwingAmount));
            m_frontLeftLeg->setRotateAngleX(
                static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ + mc::math::PI_DOUBLE) * limbSwingAmount));
            m_frontRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * LIMB_SWING_FREQ) * limbSwingAmount));

            // state==1 时尾巴角度使用 π/4，其他情况使用 TAIL_CROUCH_AMPLITUDE
            if (m_state == 1) {
                m_tail2->setRotateAngleX(static_cast<f32>(
                    TAIL_BASE_ANGLE + (mc::math::PI_DOUBLE / 4.0) * std::cos(limbSwing) * limbSwingAmount));
            } else {
                m_tail2->setRotateAngleX(
                    static_cast<f32>(TAIL_BASE_ANGLE + TAIL_CROUCH_AMPLITUDE * std::cos(limbSwing) * limbSwingAmount));
            }
        }
    }
}

} // namespace mc::client::renderer::entity::model::animal
