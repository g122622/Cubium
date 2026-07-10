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

#include "MonsterVariantModels.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

// ==================== ZombieVillagerModel ====================

ZombieVillagerModel::ZombieVillagerModel()
    : ZombieModel(false)
{
    _setupParts(0.0f, false);
}

ZombieVillagerModel::ZombieVillagerModel(f32 scale, bool slim)
    : ZombieModel(slim)
{
    _setupParts(scale, slim);
}

void ZombieVillagerModel::_setupParts(f32 scale, bool slim)
{
    if (slim) {
        m_head->setTextureOffset(0, 0);
        m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
        m_body->setTextureOffset(16, 16);
        m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, scale + 0.1f);
    } else {
        m_head->setTextureOffset(0, 0);
        m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 10.0f, 8.0f, scale);
        m_head->setTextureOffset(24, 0);
        m_head->addBox(-1.0f, -3.0f, -6.0f, 2.0f, 4.0f, 2.0f, scale); // 鼻子

        m_villagerNose = std::make_shared<ModelRenderer>("villagerNose");
        m_villagerNose->setTextureOffset(30, 47);
        m_villagerNose->addBox(-8.0f, -8.0f, -6.0f, 16.0f, 16.0f, 1.0f, scale);
        m_villagerNose->setRotateAngleX(static_cast<f32>(-mc::math::PI / 2.0));

        m_body->setTextureOffset(16, 20);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 12.0f, 6.0f, scale);
        m_body->setTextureOffset(0, 38);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 18.0f, 6.0f, scale + 0.05f);
    }
}

void ZombieVillagerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用 ZombieModel::setAngles 以获得 animateZombieArms 手臂前伸/攻击动画
    ZombieModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

void ZombieVillagerModel::setHeadVisible(bool visible)
{
    m_head->setVisible(visible);
    m_headwear->setVisible(visible);
    if (m_villagerNose) {
        m_villagerNose->setVisible(visible);
    }
}

// ==================== DrownedModel ====================

DrownedModel::DrownedModel()
    : ZombieModel(false)
{
    _setupParts(0.0f, 0.0f, 64, 64);
}

DrownedModel::DrownedModel(f32 scale, bool slim)
    : ZombieModel(slim)
{
    MC_UNUSED(slim);
    _setupParts(scale, 0.0f, 64, 64);
}

void DrownedModel::_setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
{
    MC_UNUSED(textureWidth);
    MC_UNUSED(textureHeight);

    // 溺尸有特殊的手臂和腿
    m_rightArm->setTextureOffset(32, 48);
    m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_rightArm->setRotationPoint(-5.0f, 2.0f + yOffset, 0.0f);

    m_rightLeg->setTextureOffset(16, 48);
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_rightLeg->setRotationPoint(-1.9f, 12.0f + yOffset, 0.0f);
}

void DrownedModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用 ZombieModel::setAngles 以获得 animateZombieArms 手臂前伸/攻击动画
    // 注意：ZombieModel::setAngles 内部会调用 BipedModel::setAngles（其中
    // handleRightArmPose/handleLeftArmPose 已应用 ThrowSpear 手臂姿态），然后
    // animateZombieArms 会覆盖手臂角度，导致 ThrowSpear 姿态丢失。因此本方法
    // 在 super 调用返回后需要重新应用 ThrowSpear 姿态，与 MC 1.21.11
    // DrownedModel.setupAnim 的处理方式一致。
    ZombieModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 重新应用三叉戟投掷手臂姿态（对应 MC 1.21.11 DrownedModel.setupAnim 中的
    // THROW_TRIDENT 分支）。animateZombieArms 在 ZombieModel::setAngles 中已覆盖
    // 手臂角度，此处的 *0.5 - PI 会将手臂抬到头顶后方（投掷起始姿态）。
    // 注意：必须在游泳覆盖之前执行，因为游泳覆盖会用 rotLerpRad 在当前角度与
    // 目标游泳角度之间插值，而 ThrowSpear 调整后的角度是该插值的起点之一。
    if (m_leftArmPose == ArmPose::ThrowSpear) {
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
        m_leftArm->setRotateAngleY(0.0f);
    }
    if (m_rightArmPose == ArmPose::ThrowSpear) {
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
        m_rightArm->setRotateAngleY(0.0f);
    }

    // 游泳动画覆盖（对应 MC 1.21.11 DrownedModel.setupAnim 末尾的 swimAmount 分支）
    // m_swimAnimation 由 EntityRendererManager::_applyZombieState 在 setAngles 调用前
    // 通过 setSwimAnimation(context.swimAmount) 推送。当 swimAmount > 0 时，手臂向身体
    // 两侧张开并前后摆动（模拟划水），腿部交替上下摆动（模拟打水），头部归零（平视前方）。
    // 该覆盖与玩家爬行式游泳（BipedModel::handleSwimAnimation）完全不同，是溺尸专属的
    // 两栖游泳姿态。
    const f32 f = m_swimAnimation;
    if (f > 0.0f) {
        // 手臂 X 旋转：在当前角度与 -4π/5 之间插值，加上随 ageInTicks 的正弦摆动
        // 右臂 +摆动，左臂 -摆动，形成交替划水的视觉效果
        m_rightArm->setRotateAngleX(
            rotLerpRad(f, m_rightArm->rotateAngleX(), static_cast<f32>(-mc::math::PI_DOUBLE * 4.0 / 5.0)) +
            f * 0.35f * static_cast<f32>(std::sin(0.1f * ageInTicks)));
        m_leftArm->setRotateAngleX(
            rotLerpRad(f, m_leftArm->rotateAngleX(), static_cast<f32>(-mc::math::PI_DOUBLE * 4.0 / 5.0)) -
            f * 0.35f * static_cast<f32>(std::sin(0.1f * ageInTicks)));

        // 手臂 Z 旋转：右臂略微内收（-0.15），左臂略微外展（+0.15）
        m_rightArm->setRotateAngleZ(rotLerpRad(f, m_rightArm->rotateAngleZ(), -0.15f));
        m_leftArm->setRotateAngleZ(rotLerpRad(f, m_leftArm->rotateAngleZ(), 0.15f));

        // 腿部 X 旋转：交替上下摆动（模拟打水）
        m_leftLeg->setRotateAngleX(
            m_leftLeg->rotateAngleX() - f * 0.55f * static_cast<f32>(std::sin(0.1f * ageInTicks)));
        m_rightLeg->setRotateAngleX(
            m_rightLeg->rotateAngleX() + f * 0.55f * static_cast<f32>(std::sin(0.1f * ageInTicks)));

        // 头部归零：游泳时头部平视前方，不跟随 netHeadYaw/headPitch
        m_head->setRotateAngleX(0.0f);
    }
}

// ==================== StrayModel ====================

StrayModel::StrayModel()
    : BipedModel()
{
    // 流浪者与骷髅结构相同，只是纹理不同
}

// ==================== HuskModel ====================

HuskModel::HuskModel()
    : ZombieModel(false)
{
    // 尸壳与僵尸结构相同，只是纹理不同
}

// ==================== CaveSpiderModel ====================

CaveSpiderModel::CaveSpiderModel()
    : SpiderModel() // 继承 SpiderModel，模型结构完全相同
{
    // 洞穴蜘蛛模型与普通蜘蛛完全相同
    // 区别在于渲染时缩放 0.7 倍（在 render() 中处理）
}

void CaveSpiderModel::render(f64 scale)
{
    SpiderModel::render(scale * getScaleFactor());
}

// ==================== GiantModel ====================

GiantModel::GiantModel()
    : ZombieModel(false)
{
    // 巨人与僵尸模型相同，只是渲染时缩放更大
}

} // namespace mc::client::renderer::entity::model::monster
