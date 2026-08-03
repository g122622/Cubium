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

#include "EndermanModel.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

namespace mc::client::renderer::entity::model::monster {

namespace {
// 末影人 Y 偏移：-14（比普通生物高）
constexpr f32 ENDERMAN_Y_OFFSET = -14.0f;
// 手臂/腿角度限制：±0.4 弧度
constexpr f32 ARM_LEG_ANGLE_LIMIT = 0.4f;
} // namespace

EndermanModel::EndermanModel()
    : BipedModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void EndermanModel::setupParts()
{
    // 末影人有独特的身体比例：手臂和腿非常长
    // 清除基类添加的盒子，重新设置末影人特有的尺寸

    // 头部内层
    m_head->clearBoxes();
    m_head->setTextureSize(64, 32);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.0f);
    m_head->setRotationPoint(0.0f, ENDERMAN_Y_OFFSET, 0.0f);

    // 头部外层（头套）
    m_headwear->clearBoxes();
    m_headwear->setTextureSize(64, 32);
    m_headwear->setTextureOffset(0, 16);
    m_headwear->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, -0.5f);
    m_headwear->setRotationPoint(0.0f, ENDERMAN_Y_OFFSET, 0.0f);

    // 身体
    m_body->clearBoxes();
    m_body->setTextureSize(64, 32);
    m_body->setTextureOffset(32, 16);
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 0.0f);
    m_body->setRotationPoint(0.0f, ENDERMAN_Y_OFFSET, 0.0f);

    // 右臂
    m_rightArm->clearBoxes();
    m_rightArm->setTextureSize(64, 32);
    m_rightArm->setTextureOffset(56, 0);
    m_rightArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
    m_rightArm->setRotationPoint(-3.0f, -12.0f, 0.0f);

    // 左臂（镜像）
    m_leftArm->clearBoxes();
    m_leftArm->setTextureSize(64, 32);
    m_leftArm->setTextureOffset(56, 0);
    m_leftArm->setMirror(true);
    m_leftArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
    m_leftArm->setRotationPoint(5.0f, -12.0f, 0.0f);

    // 右腿
    m_rightLeg->clearBoxes();
    m_rightLeg->setTextureSize(64, 32);
    m_rightLeg->setTextureOffset(56, 0);
    m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
    m_rightLeg->setRotationPoint(-2.0f, -2.0f, 0.0f);

    // 左腿（镜像）
    m_leftLeg->clearBoxes();
    m_leftLeg->setTextureSize(64, 32);
    m_leftLeg->setTextureOffset(56, 0);
    m_leftLeg->setMirror(true);
    m_leftLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
    m_leftLeg->setRotationPoint(2.0f, -2.0f, 0.0f);
}

void EndermanModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 头部始终显示
    m_head->setVisible(true);

    // 身体旋转
    m_body->setRotateAngleX(0.0f);
    m_body->setRotationPointY(ENDERMAN_Y_OFFSET);
    m_body->setRotationPointZ(0.0f);

    // 手臂和腿的角度限制：±0.4 弧度，并缩小到一半
    f32 rightArmX = m_rightArm->rotateAngleX() * 0.5f;
    rightArmX = std::clamp(rightArmX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
    m_rightArm->setRotateAngleX(rightArmX);

    f32 leftArmX = m_leftArm->rotateAngleX() * 0.5f;
    leftArmX = std::clamp(leftArmX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
    m_leftArm->setRotateAngleX(leftArmX);

    f32 rightLegX = m_rightLeg->rotateAngleX() * 0.5f;
    rightLegX = std::clamp(rightLegX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
    m_rightLeg->setRotateAngleX(rightLegX);

    f32 leftLegX = m_leftLeg->rotateAngleX() * 0.5f;
    leftLegX = std::clamp(leftLegX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
    m_leftLeg->setRotateAngleX(leftLegX);

    // 携带方块时手臂前伸
    if (m_carrying) {
        m_rightArm->setRotateAngleX(-0.5f);
        m_rightArm->setRotateAngleZ(0.05f);
        m_leftArm->setRotateAngleX(-0.5f);
        m_leftArm->setRotateAngleZ(-0.05f);
    }

    // 位置重置
    m_rightArm->setRotationPointZ(0.0f);
    m_leftArm->setRotationPointZ(0.0f);
    m_rightLeg->setRotationPointZ(0.0f);
    m_rightLeg->setRotationPointY(-5.0f);
    m_leftLeg->setRotationPointZ(0.0f);
    m_leftLeg->setRotationPointY(-5.0f);

    // 头部位置
    m_head->setRotationPointZ(0.0f);
    m_head->setRotationPointY(-13.0f);

    // 攻击/尖叫状态：头部下移
    if (m_attacking) {
        m_head->setRotationPointY(m_head->rotationPointY() - 5.0f);
    }

    // 同步头部外层位置和角度
    m_headwear->setRotationPoint(m_head->rotationPointX(), m_head->rotationPointY(), m_head->rotationPointZ());
    m_headwear->setRotateAngleX(m_head->rotateAngleX());
    m_headwear->setRotateAngleY(m_head->rotateAngleY());
    m_headwear->setRotateAngleZ(m_head->rotateAngleZ());

    // 最终手臂位置
    m_rightArm->setRotationPoint(-5.0f, -12.0f, 0.0f);
    m_leftArm->setRotationPoint(5.0f, -12.0f, 0.0f);

    (void)ageInTicks; // 末影人不使用 ageInTicks
    (void)scale;      // 已在 render() 中使用
}

} // namespace mc::client::renderer::entity::model::monster
