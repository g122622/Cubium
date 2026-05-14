#include "SkeletonModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
constexpr f64 DEG_TO_RAD = mc::math::PI_DOUBLE / 180.0;
}

SkeletonModel::SkeletonModel()
    : BipedModel()
{
    // 骷髅使用 64x32 纹理
    setTextureSize(64, 32);
    setupParts();
}

void SkeletonModel::setupParts()
{
    // 参考 MC 1.16.5 SkeletonModel
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
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 参考 MC 1.16.5 SkeletonModel.setRotationAngles 第 64-76 行
    // 空手攻击动画 - 当攻击中且主手不是弓时
    // 注意：isAggressive 和 swingProgress 需要从实体获取
    // 这里使用 m_isAggressive 和 m_swingProgress 成员变量

    if (m_isAggressive && m_rightArmPose != ArmPose::BowAndArrow) {
        f32 swingProgress = m_swingProgress;
        f32 f = static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE));
        f32 f1 =
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

            // 手臂抖动效果 (ModelHelper.func_239101_a_)
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

    // 弓箭姿态处理
    switch (m_rightArmPose) {
        case ArmPose::BowAndArrow:
            // 拉弓姿态 - 参考 MC 1.16.5 BipedModel 弓箭姿态
            // 使用头部的Y旋转角度
            if (m_rightArm && m_leftArm && m_head) {
                m_rightArm->setRotateAngleY(-0.1f + m_head->rotateAngleY());
                m_leftArm->setRotateAngleY(0.1f + m_head->rotateAngleY() + 0.4f);
                m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0 + headPitch * DEG_TO_RAD));
                m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0 + headPitch * DEG_TO_RAD));
            }
            break;
        case ArmPose::ThrowSpear:
            // 投掷三叉戟姿态
            if (m_rightArm) {
                m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
            }
            break;
        case ArmPose::CrossbowCharge:
        case ArmPose::CrossbowHold:
            // 弩姿态需要额外实现
            break;
        default:
            break;
    }
}

} // namespace mc::client::renderer::entity::model::monster
