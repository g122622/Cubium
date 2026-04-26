#include "ZombieModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
    constexpr f64 DEG_TO_RAD = PI / 180.0;
}

ZombieModel::ZombieModel(bool slim)
    : BipedModel()
    , m_slim(slim)
{
    // 参考 MC 1.16.5 ZombieModel 构造函数
    // 普通僵尸使用 64x64 纹理，尸壳/溺尸使用 64x32
    setTextureSize(64, 64);
    setupParts();
}

void ZombieModel::setupParts() {
    // 僵尸的部件尺寸与玩家相同
    // 由 BipedModel 基类设置
}

void ZombieModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 参考 MC 1.16.5 AbstractZombieModel.setRotationAngles
    // ModelHelper.func_239105_a_(leftArm, rightArm, isAggressive, swingProgress, ageInTicks)
    // 僵尸攻击动画

    (void)ageInTicks;  // 在攻击动画中使用
}

void ZombieModel::setAttackAnimation(f64 swingProgress, f64 ageInTicks, bool isAggressive) {
    // 参考 MC 1.16.5 ModelHelper.func_239105_a_
    // float f = MathHelper.sin(swingProgress * PI);
    // float f1 = MathHelper.sin((1.0F - (1.0F - swingProgress) * (1.0F - swingProgress)) * PI);
    f32 f = static_cast<f32>(std::sin(swingProgress * PI));
    f32 f1 = static_cast<f32>(std::sin((1.0 - (1.0 - swingProgress) * (1.0 - swingProgress)) * PI));

    if (m_leftArm && m_rightArm) {
        m_rightArm->setRotateAngleZ(0.0f);
        m_leftArm->setRotateAngleZ(0.0f);

        m_rightArm->setRotateAngleY(-(0.1f - f * 0.6f));
        m_leftArm->setRotateAngleY(0.1f - f * 0.6f);

        // float f2 = -(float)Math.PI / (isAggressive ? 1.5F : 2.25F);
        f32 f2 = static_cast<f32>(-PI / (isAggressive ? 1.5 : 2.25));

        m_rightArm->setRotateAngleX(f2);
        m_leftArm->setRotateAngleX(f2);

        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() + f * 1.2f - f1 * 0.4f);
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() + f * 1.2f - f1 * 0.4f);

        // func_239101_a_: 添加手臂抖动
        // rightArm.rotateAngleZ += MathHelper.cos(ageInTicks * 0.09F) * 0.05F + 0.05F;
        // leftArm.rotateAngleZ -= MathHelper.cos(ageInTicks * 0.09F) * 0.05F + 0.05F;
        // rightArm.rotateAngleX += MathHelper.sin(ageInTicks * 0.067F) * 0.05F;
        // leftArm.rotateAngleX -= MathHelper.sin(ageInTicks * 0.067F) * 0.05F;
        m_rightArm->setRotateAngleZ(m_rightArm->rotateAngleZ() +
            static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_leftArm->setRotateAngleZ(m_leftArm->rotateAngleZ() -
            static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() +
            static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() -
            static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
    }
}

void ZombieModel::setTextureDimensions(bool useSlimTexture) {
    // 普通僵尸使用 64x64，尸壳/溺尸使用 64x32
    if (useSlimTexture) {
        setTextureSize(64, 32);
    } else {
        setTextureSize(64, 64);
    }
}

} // namespace mc::client::renderer::entity::model::monster
