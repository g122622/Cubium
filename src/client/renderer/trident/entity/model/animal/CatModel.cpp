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

#include "CatModel.hpp"
#include "client/renderer/trident/entity/model/animal/OcelotModel.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"

namespace mc::client::renderer::entity::model::animal {

CatModel::CatModel(f32 scale)
    : OcelotModel(scale)
{}

void CatModel::setCatAnimState(f32 lieDownAmount, f32 relaxStateAmount, f32 sleepPoseAmount)
{
    m_lieDownAmount = lieDownAmount;
    m_relaxStateAmount = relaxStateAmount;
    m_sleepPoseAmount = sleepPoseAmount;
}

void CatModel::setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick)
{
    // 首先检查躺下动画进度
    if (m_lieDownAmount <= 0.0f) {
        // 重置头部角度
        m_head->setRotateAngleX(0.0f);
        m_head->setRotateAngleZ(0.0f);
        // 重置前腿
        m_frontLeftLeg->setRotateAngleX(0.0f);
        m_frontLeftLeg->setRotateAngleZ(0.0f);
        m_frontRightLeg->setRotateAngleX(0.0f);
        m_frontRightLeg->setRotateAngleZ(0.0f);
        m_frontRightLeg->setRotationPointX(-1.2f);
        // 重置后腿
        m_backLeftLeg->setRotateAngleX(0.0f);
        m_backRightLeg->setRotateAngleX(0.0f);
        m_backRightLeg->setRotateAngleZ(0.0f);
        m_backRightLeg->setRotationPointX(-1.1f);
        m_backRightLeg->setRotationPointY(18.0f);
    }

    // 调用父类的 setLivingAnimations
    OcelotModel::setLivingAnimations(limbSwing, limbSwingAmount, partialTick);

    // 坐下状态的处理
    if (m_isSitting) {
        // 调整身体位置
        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));
        m_body->setRotationPointY(8.0f);  // 12 - 4
        m_body->setRotationPointZ(-5.0f); // -10 + 5
        // 调整头部位置
        m_head->setRotationPointY(11.7f); // 15 - 3.3
        m_head->setRotationPointZ(-8.0f); // -9 + 1
        // 调整尾巴位置
        m_tail->setRotationPointY(23.0f);  // 15 + 8
        m_tail->setRotationPointZ(6.0f);   // 8 - 2
        m_tail2->setRotationPointY(22.0f); // 20 + 2
        m_tail2->setRotationPointZ(13.2f); // 14 - 0.8
        m_tail->setRotateAngleX(1.7278761f);
        m_tail2->setRotateAngleX(2.670354f);
        // 调整前腿
        m_frontLeftLeg->setRotateAngleX(-0.15707964f);
        m_frontLeftLeg->setRotationPointY(16.1f);
        m_frontLeftLeg->setRotationPointZ(-7.0f);
        m_frontRightLeg->setRotateAngleX(-0.15707964f);
        m_frontRightLeg->setRotationPointY(16.1f);
        m_frontRightLeg->setRotationPointZ(-7.0f);
        // 调整后腿
        m_backLeftLeg->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        m_backLeftLeg->setRotationPointY(21.0f);
        m_backLeftLeg->setRotationPointZ(1.0f);
        m_backRightLeg->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        m_backRightLeg->setRotationPointY(21.0f);
        m_backRightLeg->setRotationPointZ(1.0f);
        // 设置状态为坐下
        m_state = 3;
    }
}

void CatModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用父类动画
    OcelotModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 如果躺下动画 > 0，执行躺下动画
    if (m_lieDownAmount > 0.0f) {
        // 头部倾斜 - 使用角度插值
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
        f32 currentTailX = m_tail->rotateAngleX();
        f32 currentTail2X = m_tail2->rotateAngleX();
        m_tail->setRotateAngleX(math::lerpAngleRadians(currentTailX, 0.8f, m_relaxStateAmount));
        m_tail2->setRotateAngleX(math::lerpAngleRadians(currentTail2X, -0.4f, m_relaxStateAmount));
    }

    // 睡眠姿势 - 使用角度插值
    if (m_sleepPoseAmount > 0.0f) {
        f32 currentHeadX = m_head->rotateAngleX();
        m_head->setRotateAngleX(math::lerpAngleRadians(currentHeadX, -0.58177644f, m_sleepPoseAmount));
    }

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
