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

#include "SkeletonModel.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

SkeletonModel::SkeletonModel()
    : BipedModel()
{
    // 骷髅使用 64x32 纹理
    setTextureSize(64, 32);
    setupParts();
}

void SkeletonModel::setupParts()
{
    // 骷髅的手臂和腿比玩家更细

    // 重置手臂部件为更细的尺寸
    // 骷髅手臂：2x12x2（玩家是 4x12x4）
    if (m_rightArm) {
        m_rightArm->setTextureOffset(40, 16);
        m_rightArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 12.0f, 2.0f, 0.0f);
        m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
    }

    if (m_leftArm) {
        m_leftArm->setTextureOffset(40, 16);
        m_leftArm->setMirror(true);
        m_leftArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 12.0f, 2.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);
    }

    // 骷髅腿：2x12x2（玩家是 4x12x4）
    if (m_rightLeg) {
        m_rightLeg->setTextureOffset(0, 16);
        m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 12.0f, 2.0f, 0.0f);
        m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);
    }

    if (m_leftLeg) {
        m_leftLeg->setTextureOffset(0, 16);
        m_leftLeg->setMirror(true);
        m_leftLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 12.0f, 2.0f, 0.0f);
        m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);
    }
}

void SkeletonModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类设置基础动画，包括所有 ArmPose（BowAndArrow/CrossbowCharge/CrossbowHold/
    // ThrowSpear/Spyglass/Brush/Item/Block/Empty）的双手协调处理。
    // 基类 handleRightArmPose/handleLeftArmPose 会读取 m_rightArmPose/m_leftArmPose
    // 字段（由 setRightArmPose/setLeftArmPose 写入），无需子类重复实现。
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 空手攻击动画 - 对应 MC 1.21.11 SkeletonModel.setupAnim 中
    // if (state.isAggressive && !state.isHoldingBow) 分支。
    // 当骷髅攻击中且未持弓拉弓时，覆盖基类设置的手臂角度，呈现空手挥击动画。
    // 注意：当 ArmPose 为 BowAndArrow 时保留基类拉弓姿态，不进入此分支。
    // 当 ArmPose 为 CrossbowCharge/CrossbowHold 时也保留基类弩姿态。
    // TODO: isAggressive 状态当前未由渲染管线推送（EntityRendererManager 未调用
    //       setAggressive），因为骷髅攻击状态尚未通过 metadata 同步到 ClientEntity。
    //       待网络同步完成后，应在 EntityRendererManager 的 skeleton 分支调用
    //       setAggressive(entity.isAggressive())。
    if (m_isAggressive && m_rightArmPose != ArmPose::BowAndArrow && m_rightArmPose != ArmPose::CrossbowCharge &&
        m_rightArmPose != ArmPose::CrossbowHold) {
        const f32 swingProgress = m_swingProgress;
        const f32 f = static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE));
        const f32 f1 =
            static_cast<f32>(std::sin((1.0 - (1.0 - swingProgress) * (1.0 - swingProgress)) * mc::math::PI_DOUBLE));

        if (m_rightArm && m_leftArm) {
            m_rightArm->setRotateAngleZ(0.0f);
            m_leftArm->setRotateAngleZ(0.0f);
            m_rightArm->setRotateAngleY(-(0.1f - f * 0.6f));
            m_leftArm->setRotateAngleY(0.1f - f * 0.6f);
            m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));
            m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));

            // 手臂抖动效果 (AnimationUtils.bobArms)
            m_rightArm->setRotateAngleZ(
                m_rightArm->rotateAngleZ() + static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
            m_leftArm->setRotateAngleZ(
                m_leftArm->rotateAngleZ() - static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
            m_rightArm->setRotateAngleX(
                m_rightArm->rotateAngleX() + static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
            m_leftArm->setRotateAngleX(
                m_leftArm->rotateAngleX() - static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
        }
    }
}

} // namespace mc::client::renderer::entity::model::monster
