#include "CatModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

CatModel::CatModel(f32 scale)
    : OcelotModel(scale)
{
}

void CatModel::setCatAnimState(f32 lieDownAmount, f32 relaxStateAmount, f32 sleepPoseAmount) {
    m_lieDownAmount = lieDownAmount;
    m_relaxStateAmount = relaxStateAmount;
    m_sleepPoseAmount = sleepPoseAmount;
}

void CatModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 CatModel.setRotationAngles
    // 使用角度插值函数处理躺下动画

    // 如果躺下动画 > 0，执行躺下动画
    if (m_lieDownAmount > 0.0f) {
        // 头部倾斜 - 使用角度插值
        // Java: this.ocelotHead.rotateAngleZ = ModelUtils.func_228283_a_(this.ocelotHead.rotateAngleZ, -1.2707963F, this.field_217155_m);
        f32 currentHeadZ = m_head->rotateAngleZ();
        f32 currentHeadY = m_head->rotateAngleY();
        m_head->setRotateAngleZ(math::lerpAngleRadians(currentHeadZ, -1.2707963f, m_lieDownAmount));
        m_head->setRotateAngleY(math::lerpAngleRadians(currentHeadY, 1.2707963f, m_lieDownAmount));

        // 前腿姿势
        m_frontLeftLeg->setRotateAngleX(-1.2707963f);
        m_frontRightLeg->setRotateAngleX(-0.47079635f);
        m_frontRightLeg->setRotateAngleZ(-0.2f);
        m_frontRightLeg->setRotationPointX(-0.2f);

        // 后腿姿势
        m_backLeftLeg->setRotateAngleX(-0.4f);
        m_backRightLeg->setRotateAngleX(0.5f);
        m_backRightLeg->setRotateAngleZ(-0.5f);
        m_backRightLeg->setRotationPointX(-0.3f);
        m_backRightLeg->setRotationPointY(20.0f);

        // 尾巴动画 - 使用角度插值
        // Java: this.ocelotTail.rotateAngleX = ModelUtils.func_228283_a_(this.ocelotTail.rotateAngleX, 0.8F, this.field_217156_n);
        f32 currentTailX = m_tail->rotateAngleX();
        f32 currentTail2X = m_tail2->rotateAngleX();
        m_tail->setRotateAngleX(math::lerpAngleRadians(currentTailX, 0.8f, m_relaxStateAmount));
        m_tail2->setRotateAngleX(math::lerpAngleRadians(currentTail2X, -0.4f, m_relaxStateAmount));
    } else {
        // 重置为默认姿势
        m_head->setRotateAngleZ(0.0f);
        m_frontRightLeg->setRotateAngleZ(0.0f);
        m_frontRightLeg->setRotationPointX(-1.2f);
        m_backRightLeg->setRotateAngleZ(0.0f);
        m_backRightLeg->setRotationPointX(-1.1f);
        m_backRightLeg->setRotationPointY(18.0f);
    }

    // 睡眠姿势 - 使用角度插值
    if (m_sleepPoseAmount > 0.0f) {
        // Java: this.ocelotHead.rotateAngleX = ModelUtils.func_228283_a_(this.ocelotHead.rotateAngleX, -0.58177644F, this.field_217157_o);
        f32 currentHeadX = m_head->rotateAngleX();
        m_head->setRotateAngleX(math::lerpAngleRadians(currentHeadX, -0.58177644f, m_sleepPoseAmount));
    }

    // 坐下状态
    if (m_isSitting) {
        m_body->setRotateAngleX(static_cast<f32>(PI / 4.0));
        m_body->setRotationPointY(8.0f);  // 12 - 4
        m_body->setRotationPointZ(-5.0f); // -10 + 5
        m_head->setRotationPointY(11.7f); // 15 - 3.3
        m_head->setRotationPointZ(-8.0f); // -9 + 1
        m_tail->setRotationPointY(23.0f); // 15 + 8
        m_tail->setRotationPointZ(6.0f);  // 8 - 2
        m_tail2->setRotationPointY(22.0f); // 20 + 2
        m_tail2->setRotationPointZ(13.2f); // 14 - 0.8
        m_tail->setRotateAngleX(1.7278761f);
        m_tail2->setRotateAngleX(2.670354f);
        m_frontLeftLeg->setRotateAngleX(-0.15707964f);
        m_frontLeftLeg->setRotationPointY(16.1f);
        m_frontLeftLeg->setRotationPointZ(-7.0f);
        m_frontRightLeg->setRotateAngleX(-0.15707964f);
        m_frontRightLeg->setRotationPointY(16.1f);
        m_frontRightLeg->setRotationPointZ(-7.0f);
        m_backLeftLeg->setRotateAngleX(static_cast<f32>(-PI / 2.0));
        m_backLeftLeg->setRotationPointY(21.0f);
        m_backLeftLeg->setRotationPointZ(1.0f);
        m_backRightLeg->setRotateAngleX(static_cast<f32>(-PI / 2.0));
        m_backRightLeg->setRotationPointY(21.0f);
        m_backRightLeg->setRotationPointZ(1.0f);
        m_state = 3;
    }

    // 调用父类动画
    OcelotModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

} // namespace mc::client::renderer::entity::model::animal
