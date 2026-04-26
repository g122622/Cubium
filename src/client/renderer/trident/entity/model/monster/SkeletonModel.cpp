#include "SkeletonModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
    constexpr f64 DEG_TO_RAD = PI / 180.0;
}

SkeletonModel::SkeletonModel()
    : BipedModel()
{
    // 骷髅使用 64x32 纹理
    setTextureSize(64, 32);
    setupParts();
}

void SkeletonModel::setupParts() {
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

void SkeletonModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                               f64 ageInTicks, f64 netHeadYaw,
                               f64 headPitch, f64 scale) {
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 骷髅手臂姿态处理
    // 参考 MC 1.16.5 SkeletonModel.setRotationAngles

    switch (m_rightArmPose) {
        case ArmPose::BowAndArrow:
            // 拉弓姿态 - 参考 MC 1.16.5 SkeletonModel.setRotationAngles
            // 使用头部的Y旋转角度，而不是netHeadYaw（度数）
            m_rightArm->setRotateAngleY(-0.1f + m_head->rotateAngleY());
            m_leftArm->setRotateAngleY(0.1f + m_head->rotateAngleY() + 0.4f);
            m_rightArm->setRotateAngleX(static_cast<f32>(-PI / 2.0 + headPitch * DEG_TO_RAD));
            m_leftArm->setRotateAngleX(static_cast<f32>(-PI / 2.0 + headPitch * DEG_TO_RAD));
            break;
        case ArmPose::ThrowSpear:
            // 投掷三叉戟姿态
            m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() * 0.5f - static_cast<f32>(PI));
            break;
        default:
            // 空手或持有物品时，手臂向前伸
            m_rightArm->setRotateAngleX(static_cast<f32>(-PI / 2.0));
            m_leftArm->setRotateAngleX(static_cast<f32>(-PI / 2.0));
            break;
    }

    (void)ageInTicks;  // 暂时未使用
}

} // namespace mc::client::renderer::entity::model::monster
