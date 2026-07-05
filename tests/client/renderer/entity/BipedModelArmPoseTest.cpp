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
 * @file BipedModelArmPoseTest.cpp
 * @brief BipedModel Spyglass/Brush 手臂姿态角度单元测试
 *
 * 验证 handleRightArmPose/handleLeftArmPose 中 Spyglass 与 Brush 分支的角度计算，
 * 对应 MC 1.21.11 HumanoidModel.poseRightArm/poseLeftArm 中的 SPYGLASS/BRANCH 分支：
 *
 * - Spyglass 右臂：xRot = clamp(headX - 1.9198622 - (crouching ? π/12 : 0), -2.4, 3.3)
 *                  yRot = headY - π/12
 * - Spyglass 左臂：xRot 同右臂，yRot = headY + π/12
 * - Brush 右臂：xRot = rightArm.xRot * 0.5 - π/5, yRot = 0
 * - Brush 左臂：xRot = leftArm.xRot * 0.5 - π/5,  yRot = 0
 *
 * 测试通过 setRightArmPose/setLeftArmPose + setAngles 触发 handleRightArmPose/handleLeftArmPose，
 * 然后断言手臂 rotateAngleX/Y 与期望值的偏差在 1e-5 以内。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief BipedModel Spyglass/Brush 手臂姿态角度单元测试夹具
 *
 * SetUp 中关闭 sneaking/swimming 等其他状态对角度的干扰，并将主手设为右手。
 * 各测试用例通过 setRightArmPose/setLeftArmPose + applyAngles 触发姿态处理。
 */
class BipedModelArmPoseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<BipedModel>();
        m_model->setSneaking(false);
        m_model->setSwimAnimation(0.0f);
        m_model->setSwingProgress(0.0f);
        using HS = mc::client::renderer::entity::model::HandSide;
        m_model->setMainHand(HS::Right);
        m_model->setSwingingHand(HS::Right);
    }

    /**
     * @brief 触发一次 setAngles 让模型应用当前 ArmPose
     *
     * 头部 yaw/pitch 设为 0 以简化 Spyglass 角度断言。
     */
    void applyAngles() { m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<BipedModel> m_model;
};

// ========== Spyglass 右臂 ==========

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_HeadZeroYawPitch_NoCrouch)
{
    // 头部 yaw=0, pitch=0, 未蹲伏：右臂 X = clamp(0 - 1.9198622 - 0, -2.4, 3.3) = -1.9198622
    //                                  右臂 Y = 0 - π/12
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), -1.9198622f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleY(), static_cast<f32>(-PI_DOUBLE / 12.0), 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_HeadNonZeroPitch_NoCrouch)
{
    // 头部 pitch=-0.5（向上看）：右臂 X = clamp(-0.5 - 1.9198622 - 0, -2.4, 3.3) = -2.4198622 → clamp 到 -2.4
    const f64 headPitchDeg = -30.0;
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, headPitchDeg, 1.0 / 16.0);

    const f32 headPitchRad = static_cast<f32>(headPitchDeg * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 expectedX = std::clamp(headPitchRad - 1.9198622f, -2.4f, 3.3f);
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedX, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleY(), static_cast<f32>(-PI_DOUBLE / 12.0), 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_PitchClampUpperBound)
{
    // 头部 pitch=300°（5.236 rad）：右臂 X = clamp(5.236 - 1.9198622, -2.4, 3.3) = 3.3（被 clamp 到上限）
    const f64 headPitchDeg = 300.0;
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, headPitchDeg, 1.0 / 16.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), 3.3f, 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_PitchClampLowerBound)
{
    // 头部 pitch=-π：右臂 X = clamp(-π - 1.9198622, -2.4, 3.3) = -2.4（被 clamp 到下限）
    const f64 headPitchDeg = -180.0;
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, headPitchDeg, 1.0 / 16.0);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), -2.4f, 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_Crouching_AddsCrouchOffset)
{
    // 蹲伏时 handleRightArmPose 应用 -π/12 偏移：X = clamp(0 - 1.9198622 - π/12, -2.4, 3.3) = -2.1815
    // 随后 setAngles 末尾的 sneaking 块对右臂 X 再加 0.4：最终 X = -2.1815 + 0.4 = -1.7815
    m_model->setSneaking(true);
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 poseX = std::clamp(-1.9198622f - static_cast<f32>(PI_DOUBLE / 12.0), -2.4f, 3.3f);
    const f32 expectedX = poseX + 0.4f; // sneaking 块追加 0.4
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedX, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleY(), static_cast<f32>(-PI_DOUBLE / 12.0), 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_RightArm_NonZeroHeadYaw)
{
    // 头部 yaw=30°：右臂 Y = headYaw - π/12
    const f64 headYawDeg = 30.0;
    m_model->setRightArmPose(ArmPose::Spyglass);
    m_model->setLeftArmPose(ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, headYawDeg, 0.0, 1.0 / 16.0);

    const f32 headYawRad = static_cast<f32>(headYawDeg * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), headYawRad - static_cast<f32>(PI_DOUBLE / 12.0), 1e-4f);
}

// ========== Spyglass 左臂 ==========

TEST_F(BipedModelArmPoseTest, Spyglass_LeftArm_HeadZeroYawPitch_NoCrouch)
{
    // 头部 yaw=0, pitch=0：左臂 X = -1.9198622，左臂 Y = +π/12
    m_model->setRightArmPose(ArmPose::Empty);
    m_model->setLeftArmPose(ArmPose::Spyglass);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), -1.9198622f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), static_cast<f32>(PI_DOUBLE / 12.0), 1e-5f);
}

TEST_F(BipedModelArmPoseTest, Spyglass_LeftArm_NonZeroHeadYaw)
{
    // 头部 yaw=30°：左臂 Y = headYaw + π/12
    const f64 headYawDeg = 30.0;
    m_model->setRightArmPose(ArmPose::Empty);
    m_model->setLeftArmPose(ArmPose::Spyglass);
    m_model->setAngles(0.0, 0.0, 0.0, headYawDeg, 0.0, 1.0 / 16.0);

    const f32 headYawRad = static_cast<f32>(headYawDeg * PI_DOUBLE / 180.0);
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), headYawRad + static_cast<f32>(PI_DOUBLE / 12.0), 1e-4f);
}

// ========== Brush 右臂 ==========

TEST_F(BipedModelArmPoseTest, Brush_RightArm_ZeroInitialX_HalvesAndOffsets)
{
    // 头部 pitch=0 时 setAngles 走完前置流程后右臂 X 为 0，Brush 分支应用 0*0.5 - π/5 = -π/5
    m_model->setRightArmPose(ArmPose::Brush);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // setAngles 中 m_bipedRightArm 初始 X 为 0，handleRightArmPose::Brush 应用 0*0.5 - π/5
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 5.0), 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f);
}

// ========== Brush 左臂 ==========

TEST_F(BipedModelArmPoseTest, Brush_LeftArm_ZeroInitialX_HalvesAndOffsets)
{
    m_model->setRightArmPose(ArmPose::Empty);
    m_model->setLeftArmPose(ArmPose::Brush);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 5.0), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.0f, 1e-5f);
}

} // namespace
} // namespace mc::client::renderer
