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

/**
 * @file DrownedModelSwimAnimationTest.cpp
 * @brief DrownedModel::setAngles 游泳动画覆盖单元测试
 *
 * 验证 DrownedModel::setAngles 按 MC 1.21.11 DrownedModel.setupAnim 正确实现：
 *
 * 1. ThrowSpear 手臂姿态重新应用（super.setupAnim 中 animateZombieArms 会覆盖）：
 *    if (leftArmPose == THROW_TRIDENT) leftArm.xRot = leftArm.xRot * 0.5 - PI; leftArm.yRot = 0;
 *    if (rightArmPose == THROW_TRIDENT) rightArm.xRot = rightArm.xRot * 0.5 - PI; rightArm.yRot = 0;
 *
 * 2. swimAmount > 0 时的游泳覆盖（手臂/腿部/头部）：
 *    f = swimAmount
 *    rightArm.xRot = rotLerpRad(f, rightArm.xRot, -4π/5) + f * 0.35 * sin(0.1 * ageInTicks)
 *    leftArm.xRot  = rotLerpRad(f, leftArm.xRot,  -4π/5) - f * 0.35 * sin(0.1 * ageInTicks)
 *    rightArm.zRot = rotLerpRad(f, rightArm.zRot, -0.15)
 *    leftArm.zRot  = rotLerpRad(f, leftArm.zRot,   0.15)
 *    leftLeg.xRot  = leftLeg.xRot  - f * 0.55 * sin(0.1 * ageInTicks)
 *    rightLeg.xRot = rightLeg.xRot + f * 0.55 * sin(0.1 * ageInTicks)
 *    head.xRot = 0.0
 *
 * 测试在 swingProgress=0、ageInTicks=0、aggressive=false 条件下进行，使 ZombieModel
 * 设置的 rightArm.xRot 恰好为 -PI/2.25（bobArms 的 xRot 偏移 sin(0)*0.05=0），
 * 从而精确验证游泳覆盖的插值结果。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/monster/MonsterVariantModels.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::math;
namespace mc::client::renderer {
namespace {

/**
 * @brief DrownedModel 游泳动画单元测试夹具
 *
 * SetUp 中关闭 sneaking，清零 swimAnimation/swingProgress，设置主手为右手，
 * 保证 ZombieModel::setAngles 在 aggressive=false、swingProgress=0、ageInTicks=0
 * 条件下设置 rightArm.xRot = -PI/2.25（f1 = -PI/2.25，f2=f3=0，bobArms xRot 偏移=0），
 * rightArm.zRot = 0.1（bobArms zRot 偏移 cos(0)*0.05+0.05=0.1），
 * leftArm.zRot = -0.1（bobArms zRot 偏移 -(cos(0)*0.05+0.05)=-0.1）。
 */
class DrownedModelSwimAnimationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<monster::DrownedModel>();
        m_model->setSneaking(false);
        m_model->setSwimAnimation(0.0f);
        m_model->setSwingProgress(0.0f);
        m_model->setAggressive(false);
        using HS = mc::client::renderer::entity::model::HandSide;
        m_model->setMainHand(HS::Right);
        m_model->setSwingingHand(HS::Right);
    }

    /**
     * @brief 触发一次 setAngles 让模型应用当前 swimAnimation 状态
     *
     * 所有动画参数设为 0（headYaw/pitch/limbSwing），scale 取 1/16。
     * ageInTicks 默认 0，使 sin(0.1*0)=sin(0)=0，消除游泳摆动项干扰。
     */
    void applyAngles(f64 ageInTicks = 0.0) { m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<monster::DrownedModel> m_model;
};

// ============================================================================
// swimAmount = 0 时游泳覆盖不应生效（手臂保持 ZombieModel 设置）
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, SwimAmountZero_RightArmX_KeepsZombieBase)
{
    m_model->setSwimAnimation(0.0f);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // aggressive=false 时 f1 = -PI/2.25，bobArms xRot 偏移 sin(0)*0.05 = 0
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "swimAmount=0 时右臂 X 应保持 ZombieModel 的 -PI/2.25";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountZero_HeadX_NotZeroed)
{
    m_model->setSwimAnimation(0.0f);
    // 设置一个非零 headPitch（度），验证 swimAmount=0 时不会被覆盖为 0
    // BipedModel 将 headPitch（度）转为弧度：headPitch * PI_DOUBLE / 180.0
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.5, 1.0 / 16.0);

    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    const f32 expectedHeadPitchRad = static_cast<f32>(0.5 * PI_DOUBLE / 180.0);
    EXPECT_NEAR(head->rotateAngleX(), expectedHeadPitchRad, 1e-5f)
        << "swimAmount=0 时头部 X 应保持 headPitch 设置值，不被游泳覆盖归零";
}

// ============================================================================
// swimAmount = 1.0 时游泳覆盖完全生效（ageInTicks=0 消除摆动）
// ============================================================================
//
// 在 aggressive=false、swingProgress=0、ageInTicks=0 条件下：
//   ZombieModel 设置：rightArm.xRot = -PI/2.25, rightArm.zRot = 0.1
//   sin(0.1 * 0) = sin(0) = 0
//   游泳覆盖后：
//     rightArm.xRot = rotLerpRad(1.0, -PI/2.25, -4π/5) + 1.0*0.35*0 = -4π/5
//     rightArm.zRot = rotLerpRad(1.0, 0.1, -0.15) = -0.15
//     leftArm.zRot  = rotLerpRad(1.0, -0.1, 0.15) = 0.15
//     head.xRot = 0.0
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_RightArmX_IsMinusFourPiOverFive)
{
    m_model->setSwimAnimation(1.0f);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 expected = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0);
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-4f)
        << "swimAmount=1.0、ageInTicks=0 时右臂 X 应为 -4π/5（游泳手臂前伸）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_LeftArmX_IsMinusFourPiOverFive)
{
    m_model->setSwimAnimation(1.0f);
    applyAngles(0.0);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    const f32 expected = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0);
    EXPECT_NEAR(leftArm->rotateAngleX(), expected, 1e-4f) << "swimAmount=1.0、ageInTicks=0 时左臂 X 应为 -4π/5";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_RightArmZ_IsMinusPointOneFive)
{
    m_model->setSwimAnimation(1.0f);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleZ(), -0.15f, 1e-5f) << "swimAmount=1.0 时右臂 Z 应为 -0.15（游泳手臂略微内收）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_LeftArmZ_IsPointOneFive)
{
    m_model->setSwimAnimation(1.0f);
    applyAngles(0.0);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleZ(), 0.15f, 1e-5f) << "swimAmount=1.0 时左臂 Z 应为 0.15（游泳手臂略微外展）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_HeadX_IsZero)
{
    m_model->setSwimAnimation(1.0f);
    // 设置一个非零 headPitch，验证 swimAmount=1.0 时会被覆盖为 0
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.5, 1.0 / 16.0);

    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_NEAR(head->rotateAngleX(), 0.0f, 1e-6f) << "swimAmount=1.0 时头部 X 应被覆盖为 0（游泳时平视前方）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_LegsX_HaveBipedSwimBase_WhenAgeInTicksZero)
{
    // ageInTicks=0 时 sin(0.1*0)=0，DrownedModel 腿部覆盖项为 0。
    // 但 BipedModel::setAngles 的游泳块（对应 MC 1.21.11 HumanoidModel.setupAnim
    // lines 260-263）在 swimAmount>0 时会将腿部 X 朝 0.3*cos(limbSwing/3 + {0,π})
    // 插值。limbSwing=0 时：leftLeg=-0.3，rightLeg=+0.3。DrownedModel 覆盖在此基础上
    // 叠加 ∓0.55*f*sin(0.1*age)，age=0 时为 0，故最终值为 (-0.3, +0.3)。
    m_model->setSwimAnimation(1.0f);
    applyAngles(0.0);

    auto leftLeg = m_model->getLeftLeg();
    auto rightLeg = m_model->getRightLeg();
    ASSERT_NE(leftLeg, nullptr);
    ASSERT_NE(rightLeg, nullptr);
    EXPECT_NEAR(leftLeg->rotateAngleX(), -0.3f, 1e-5f)
        << "swimAmount=1.0、ageInTicks=0 时左腿 X 应为 BipedModel 游泳基线 -0.3（cos(0+π)）";
    EXPECT_NEAR(rightLeg->rotateAngleX(), 0.3f, 1e-5f)
        << "swimAmount=1.0、ageInTicks=0 时右腿 X 应为 BipedModel 游泳基线 +0.3（cos(0)）";
}

// ============================================================================
// swimAmount = 0.5 时游泳覆盖部分生效（插值验证）
// ============================================================================
//
// rotLerpRad(0.5, a, b) = a + 0.5 * wrap(b - a)
// rightArm.xRot: a=-PI/2.25, b=-4π/5, b-a = -4π/5 + PI/2.25 ≈ -1.117（在 [-π,π) 内）
//   → -PI/2.25 + 0.5 * (-1.117) ≈ -1.396 - 0.5585 ≈ -1.955
// rightArm.zRot: a=0.1, b=-0.15, b-a=-0.25
//   → 0.1 + 0.5 * (-0.25) = -0.025
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, SwimAmountHalf_RightArmX_IsInterpolated)
{
    m_model->setSwimAnimation(0.5f);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 base = static_cast<f32>(-PI_DOUBLE / 2.25);
    const f32 target = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0);
    const f32 expected = base + 0.5f * (target - base);
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-4f) << "swimAmount=0.5 时右臂 X 应为 -PI/2.25 与 -4π/5 的中点";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountHalf_RightArmZ_IsInterpolated)
{
    m_model->setSwimAnimation(0.5f);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // base=0.1, target=-0.15, 中点 = 0.1 + 0.5*(-0.25) = -0.025
    EXPECT_NEAR(rightArm->rotateAngleZ(), -0.025f, 1e-5f) << "swimAmount=0.5 时右臂 Z 应为 0.1 与 -0.15 的中点 -0.025";
}

// ============================================================================
// ageInTicks 影响：手臂/腿部的正弦摆动项
// ============================================================================
//
// ageInTicks 使得 sin(0.1 * ageInTicks) ≠ 0，验证手臂 X 的 ±0.35*f*sin 项与腿部 X 的 ∓0.55*f*sin 项。
// 取 ageInTicks = 5π（使 0.1*5π = π/2，sin(π/2) = 1），swimAmount = 1.0：
//   rightArm.xRot = rotLerpRad(1.0, -PI/2.25, -4π/5) + 1.0*0.35*sin(π/2) = -4π/5 + 0.35
//   leftArm.xRot  = rotLerpRad(1.0, -PI/2.25, -4π/5) - 1.0*0.35*sin(π/2) = -4π/5 - 0.35
// 腿部需先经过 BipedModel::setAngles 游泳块（limbSwing=0 时基线 leftLeg=-0.3, rightLeg=+0.3），
// 再叠加 DrownedModel 覆盖 ∓0.55*f*sin(0.1*age)：
//   leftLeg.xRot  = -0.3 - 1.0*0.55*1 = -0.85
//   rightLeg.xRot = +0.3 + 1.0*0.55*1 = +0.85
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_AgeInTicks_RightArmX_HasPositiveSway)
{
    m_model->setSwimAnimation(1.0f);
    // ageInTicks = 5π → 0.1*5π = π/2 → sin(π/2) = 1
    const f64 ageInTicks = 5.0 * PI_DOUBLE;
    applyAngles(ageInTicks);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 expected = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0) + 1.0f * 0.35f * 1.0f;
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-3f)
        << "swimAmount=1.0、sin(0.1*age)=1 时右臂 X 应为 -4π/5 + 0.35（正摆动）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_AgeInTicks_LeftArmX_HasNegativeSway)
{
    m_model->setSwimAnimation(1.0f);
    const f64 ageInTicks = 5.0 * PI_DOUBLE;
    applyAngles(ageInTicks);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    const f32 expected = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0) - 1.0f * 0.35f * 1.0f;
    EXPECT_NEAR(leftArm->rotateAngleX(), expected, 1e-3f)
        << "swimAmount=1.0、sin(0.1*age)=1 时左臂 X 应为 -4π/5 - 0.35（负摆动）";
}

TEST_F(DrownedModelSwimAnimationTest, SwimAmountOne_AgeInTicks_LegsX_HaveAlternatingSway)
{
    m_model->setSwimAnimation(1.0f);
    const f64 ageInTicks = 5.0 * PI_DOUBLE;
    applyAngles(ageInTicks);

    auto leftLeg = m_model->getLeftLeg();
    auto rightLeg = m_model->getRightLeg();
    ASSERT_NE(leftLeg, nullptr);
    ASSERT_NE(rightLeg, nullptr);
    // sin(0.1 * 5π) = sin(π/2) = 1
    // BipedModel 游泳基线（limbSwing=0）：leftLeg=-0.3, rightLeg=+0.3
    // DrownedModel 覆盖：
    //   leftLeg.xRot  = -0.3 - 1.0 * 0.55 * 1 = -0.85
    //   rightLeg.xRot = +0.3 + 1.0 * 0.55 * 1 = +0.85
    EXPECT_NEAR(leftLeg->rotateAngleX(), -0.85f, 1e-3f)
        << "swimAmount=1.0、sin(0.1*age)=1 时左腿 X 应为 -0.85（-0.3 基线 -0.55 摆动）";
    EXPECT_NEAR(rightLeg->rotateAngleX(), 0.85f, 1e-3f)
        << "swimAmount=1.0、sin(0.1*age)=1 时右腿 X 应为 +0.85（+0.3 基线 +0.55 摆动）";
}

// ============================================================================
// ThrowSpear 手臂姿态重新应用测试
// ============================================================================
//
// ZombieModel::setAngles 中 animateZombieArms 会覆盖手臂角度，DrownedModel 需在
// super 调用后重新应用 ThrowSpear 姿态：
//   rightArm.xRot = rightArm.xRot * 0.5 - PI
//   rightArm.yRot = 0
// 在 aggressive=false、swingProgress=0、ageInTicks=0、swimAmount=0 条件下：
//   ZombieModel 设置 rightArm.xRot = -PI/2.25
//   ThrowSpear 重新应用后：rightArm.xRot = -PI/2.25 * 0.5 - PI
//   rightArm.yRot = 0（animateZombieArms 设置为 -0.1，ThrowSpear 覆盖为 0）
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, ThrowSpear_RightArmX_IsHalfBaseMinusPi)
{
    m_model->setSwimAnimation(0.0f);
    m_model->setRightArmPose(ArmPose::ThrowSpear);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 base = static_cast<f32>(-PI_DOUBLE / 2.25);
    const f32 expected = base * 0.5f - static_cast<f32>(PI_DOUBLE);
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-4f) << "ThrowSpear 右臂 X 应为 ZombieModel 基础值 *0.5 - PI";
}

TEST_F(DrownedModelSwimAnimationTest, ThrowSpear_RightArmY_IsZero)
{
    m_model->setSwimAnimation(0.0f);
    m_model->setRightArmPose(ArmPose::ThrowSpear);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-6f)
        << "ThrowSpear 右臂 Y 应被覆盖为 0（animateZombieArms 设置的 -0.1 被清除）";
}

TEST_F(DrownedModelSwimAnimationTest, ThrowSpear_LeftArmX_IsHalfBaseMinusPi)
{
    m_model->setSwimAnimation(0.0f);
    m_model->setLeftArmPose(ArmPose::ThrowSpear);
    applyAngles(0.0);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    const f32 base = static_cast<f32>(-PI_DOUBLE / 2.25);
    const f32 expected = base * 0.5f - static_cast<f32>(PI_DOUBLE);
    EXPECT_NEAR(leftArm->rotateAngleX(), expected, 1e-4f) << "ThrowSpear 左臂 X 应为 ZombieModel 基础值 *0.5 - PI";
}

TEST_F(DrownedModelSwimAnimationTest, ThrowSpear_LeftArmY_IsZero)
{
    m_model->setSwimAnimation(0.0f);
    m_model->setLeftArmPose(ArmPose::ThrowSpear);
    applyAngles(0.0);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.0f, 1e-6f) << "ThrowSpear 左臂 Y 应被覆盖为 0";
}

// ============================================================================
// ThrowSpear + 游泳覆盖组合测试
// ============================================================================
//
// ThrowSpear 先于游泳覆盖执行，游泳覆盖的 rotLerpRad 以 ThrowSpear 调整后的角度为起点。
// 在 aggressive=false、swingProgress=0、ageInTicks=0、swimAmount=1.0 条件下：
//   ZombieModel rightArm.xRot = -PI/2.25
//   ThrowSpear: rightArm.xRot = -PI/2.25 * 0.5 - PI
//   游泳覆盖: rightArm.xRot = rotLerpRad(1.0, (-PI/2.25*0.5-PI), -4π/5) + 0
//                   = -4π/5（f=1.0 时 rotLerpRad 直接返回 target）
// 即 swimAmount=1.0 时游泳覆盖完全压过 ThrowSpear，最终角度为 -4π/5。
// 验证两者组合不冲突、游泳覆盖优先级正确。
// ============================================================================

TEST_F(DrownedModelSwimAnimationTest, ThrowSpear_And_SwimAmountOne_RightArmX_IsSwimTarget)
{
    m_model->setSwimAnimation(1.0f);
    m_model->setRightArmPose(ArmPose::ThrowSpear);
    applyAngles(0.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 expected = static_cast<f32>(-PI_DOUBLE * 4.0 / 5.0);
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-4f)
        << "ThrowSpear + swimAmount=1.0 时，游泳覆盖应完全压过 ThrowSpear，右臂 X 为 -4π/5";
}

} // namespace
} // namespace mc::client::renderer
