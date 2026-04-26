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
    // 调用 ModelHelper.func_239105_a_() 处理攻击动画
    // 参数: leftArm, rightArm, isAggressive, swingProgress, ageInTicks

    // 使用基类的 swingProgress 字段
    f32 swingProgress = m_swingProgress;

    // 计算 f 和 f1 用于攻击动画
    f32 f = static_cast<f32>(std::sin(swingProgress * PI));
    f32 f1 = static_cast<f32>(std::sin((1.0 - (1.0 - swingProgress) * (1.0 - swingProgress)) * PI));

    if (m_leftArm && m_rightArm) {
        // 重置 Z 轴旋转
        m_leftArm->setRotateAngleZ(0.0f);
        m_rightArm->setRotateAngleZ(0.0f);

        // Y 轴旋转
        m_leftArm->setRotateAngleY(-(0.1f - f * 0.6f));
        m_rightArm->setRotateAngleY(0.1f - f * 0.6f);

        // X 轴旋转基值 - 参考 MC 1.16.5 AbstractZombieModel
        // 基础角度始终为 -PI/2，然后根据攻击动画调整
        m_leftArm->setRotateAngleX(static_cast<f32>(-PI / 2.0));
        m_rightArm->setRotateAngleX(static_cast<f32>(-PI / 2.0));

        // 添加攻击动画 (f * 1.2F - f1 * 0.4F)
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));

        // 手臂抖动效果 (ModelHelper.func_239101_a_)
        m_leftArm->setRotateAngleZ(m_leftArm->rotateAngleZ() +
            static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_rightArm->setRotateAngleZ(m_rightArm->rotateAngleZ() -
            static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() +
            static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() -
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
