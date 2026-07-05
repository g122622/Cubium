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

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief BipedModel 弩装填/持握动画单元测试
 *
 * 覆盖 handleCrossbowCharge / handleCrossbowHold 的边界场景：
 * - progress = 0 / 0.5 / 1（弩装填进度归一化）
 * - progress > 1 越界（应被 clamp 到 1）
 * - m_maxCrossbowChargeDuration = 0 除零保护
 * - 右撇子 / 左撇子两种 isRightHanded 路径
 * - CrossbowHold 的头部角度跟随
 *
 * 由于 handleCrossbowCharge/Hold 是 protected 方法，测试通过 setRightArmPose /
 * setLeftArmPose + setAngles 触发 handleRightArmPose/handleLeftArmPose 间接调用。
 */
class BipedModelCrossbowTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<BipedModel>();
        // 关闭 sneaking/swimming 等其他状态对角度的干扰
        m_model->setSneaking(false);
        m_model->setSwimAnimation(0.0f);
        m_model->setSwingProgress(0.0f);
        // 主手为右手（默认）。注意：BipedModel::HandSide 与 mc::HandSide 是两个不同枚举，
        // 此处使用 model 命名空间下的 HandSide，需完整限定避免与 mc::HandSide 歧义
        using HS = mc::client::renderer::entity::model::HandSide;
        m_model->setMainHand(HS::Right);
        m_model->setSwingingHand(HS::Right);
    }

    /**
     * @brief 触发一次 setAngles 让模型应用当前 ArmPose
     *
     * 头部 yaw/pitch 设为 0 以简化 CrossbowHold 的角度断言。
     */
    void applyAngles() { m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<BipedModel> m_model;
};

// ========== handleCrossbowCharge ==========

TEST_F(BipedModelCrossbowTest, Charge_RightHanded_ProgressZero_OffArmAtInitialPosition)
{
    // progress = 0：副手 X 应等于主手 X (-0.97079635)，副手 Y 应为 ±0.4
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(0.0f);
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);

    // 主手角度
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.8f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleX(), -0.97079635f, 1e-5f);

    // 副手 X = 主手 X（初始位置）
    EXPECT_NEAR(leftArm->rotateAngleX(), -0.97079635f, 1e-5f);
    // 副手 Y = 0.4（右撇子 sign=+1, progress=0）
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.4f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_RightHanded_ProgressHalf_OffArmInterpolated)
{
    // progress = 0.5：副手 X/Y 应在初始与最终值的中点
    m_model->setMaxCrossbowChargeDuration(20.0f);
    m_model->setCrossbowChargeTicks(10.0f); // progress = 0.5
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);

    // 副手 X: lerp(-0.97079635, -PI/2, 0.5)
    const f32 expectedX = -0.97079635f + static_cast<f32>(-PI_DOUBLE / 2.0 - (-0.97079635)) * 0.5f;
    EXPECT_NEAR(leftArm->rotateAngleX(), expectedX, 1e-5f);

    // 副手 Y: lerp(0.4, 0.85, 0.5) = 0.625
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.625f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_RightHanded_ProgressFull_OffArmAtFinalPosition)
{
    // progress = 1：副手 X = -PI/2，副手 Y = 0.85
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(25.0f); // progress = 1
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.85f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_RightHanded_ProgressOverflow_ClampedToFull)
{
    // progress > 1（越界）：应被 clamp 到 1，结果与 progress=1 一致
    m_model->setMaxCrossbowChargeDuration(10.0f);
    m_model->setCrossbowChargeTicks(100.0f); // progress = 10
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.85f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_RightHanded_ProgressNegative_ClampedToZero)
{
    // progress < 0（负值）：应被 clamp 到 0，结果与 progress=0 一致
    m_model->setMaxCrossbowChargeDuration(10.0f);
    m_model->setCrossbowChargeTicks(-5.0f);
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(leftArm->rotateAngleX(), -0.97079635f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.4f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_MaxChargeDurationZero_NoDivideByZero)
{
    // m_maxCrossbowChargeDuration = 0：progress 应为 0（除零保护），副手保持初始位置
    m_model->setMaxCrossbowChargeDuration(0.0f);
    m_model->setCrossbowChargeTicks(50.0f); // 任意值
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);

    // progress=0 时副手 X = 主手 X
    EXPECT_NEAR(leftArm->rotateAngleX(), -0.97079635f, 1e-5f);
    // 副手 Y = 0.4
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.4f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_LeftHanded_MainArmOnLeft_OffArmOnRight)
{
    // 左撇子：主手为左臂，副手为右臂，sign=-1
    using HS = mc::client::renderer::entity::model::HandSide;
    m_model->setMainHand(HS::Left);
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(25.0f); // progress = 1
    m_model->setLeftArmPose(ArmPose::CrossbowCharge);
    m_model->setRightArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(leftArm, nullptr);
    ASSERT_NE(rightArm, nullptr);

    // 主手（左臂）：Y = +0.8（左撇子符号取反）
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.8f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleX(), -0.97079635f, 1e-5f);

    // 副手（右臂）：Y = -0.85（sign=-1, progress=1）
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.85f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Charge_BothPosesCrossbowCharge_NotDuplicated)
{
    // 主副手都为 CrossbowCharge：右手分支主导，左手分支跳过（避免重复设置）
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(25.0f);
    m_model->setRightArmPose(ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(ArmPose::CrossbowCharge);
    applyAngles();

    // 验证角度被正确设置（与仅右手为 CrossbowCharge 一致）
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.85f, 1e-5f);
}

// ========== handleCrossbowHold ==========

TEST_F(BipedModelCrossbowTest, Hold_RightHanded_FollowsHeadZeroYawPitch)
{
    // 头部 yaw=0, pitch=0：主手 Y=-0.3, X=-PI/2+0.1；副手 Y=0.6, X=-1.5
    m_model->setRightArmPose(ArmPose::CrossbowHold);
    m_model->setLeftArmPose(ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleY(), -0.3f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.6f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0) + 0.1f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleX(), -1.5f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Hold_RightHanded_FollowsHeadNonZeroYawPitch)
{
    // 头部 yaw=30°, pitch=-10°：手臂角度应叠加头部角度
    const f64 headYawDeg = 30.0;
    const f64 headPitchDeg = -10.0;
    m_model->setRightArmPose(ArmPose::CrossbowHold);
    m_model->setLeftArmPose(ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, headYawDeg, headPitchDeg, 1.0 / 16.0);

    const f32 headYawRad = static_cast<f32>(headYawDeg * PI_DOUBLE / 180.0);
    const f32 headPitchRad = static_cast<f32>(headPitchDeg * PI_DOUBLE / 180.0);

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleY(), -0.3f + headYawRad, 1e-4f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.6f + headYawRad, 1e-4f);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0) + headPitchRad + 0.1f, 1e-4f);
    EXPECT_NEAR(leftArm->rotateAngleX(), -1.5f + headPitchRad, 1e-4f);
}

TEST_F(BipedModelCrossbowTest, Hold_LeftHanded_ArmSignFlipped)
{
    // 左撇子：主手为左臂，sign=-1；主手 Y=+0.3, 副手 Y=-0.6
    using HS = mc::client::renderer::entity::model::HandSide;
    m_model->setMainHand(HS::Left);
    m_model->setLeftArmPose(ArmPose::CrossbowHold);
    m_model->setRightArmPose(ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(leftArm, nullptr);
    ASSERT_NE(rightArm, nullptr);

    // 左撇子 sign=-1: 主手 Y = -(-0.3) = +0.3; 副手 Y = -(0.6) = -0.6
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.3f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.6f, 1e-5f);
    // X 角度与头部无关，左撇子不翻转
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0) + 0.1f, 1e-5f);
    EXPECT_NEAR(rightArm->rotateAngleX(), -1.5f, 1e-5f);
}

TEST_F(BipedModelCrossbowTest, Hold_BothPosesCrossbowHold_NotDuplicated)
{
    // 主副手都为 CrossbowHold：右手分支主导，左手分支跳过
    m_model->setRightArmPose(ArmPose::CrossbowHold);
    m_model->setLeftArmPose(ArmPose::CrossbowHold);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleY(), -0.3f, 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.6f, 1e-5f);
}

} // namespace
} // namespace mc::client::renderer
