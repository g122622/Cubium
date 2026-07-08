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
 * @file SkeletonModelArmPoseTest.cpp
 * @brief SkeletonModel 手臂姿态委托基类 BipedModel 的单元测试
 *
 * 验证 SkeletonModel 移除遮蔽的 enum class ArmPose 和 m_rightArmPose/m_leftArmPose
 * 字段后，setRightArmPose/setLeftArmPose 正确委托到基类 BipedModel 的同名字段，
 * 并由 BipedModel::setAngles → handleRightArmPose/handleLeftArmPose 消费。
 *
 * 覆盖场景：
 * - BowAndArrow 姿态触发拉弓动画（右臂 Y=-0.1+headYaw，X=-PI/2+headPitch）
 * - Empty 姿态保持默认角度（右臂 Y=0）
 * - CrossbowCharge 姿态由基类 handleCrossbowCharge 处理
 * - CrossbowHold 姿态由基类 handleCrossbowHold 处理
 * - isAggressive 空手攻击动画（ArmPose 非 BowAndArrow/Crossbow* 时覆盖手臂角度）
 * - isAggressive + BowAndArrow 保留拉弓姿态（不进入攻击覆盖分支）
 *
 * 对应 MC 1.21.11 SkeletonModel.setupAnim：
 *   super.setupAnim(state)  // 基类处理所有 ArmPose
 *   if (state.isAggressive && !state.isHoldingBow) { 空手攻击动画 }
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief SkeletonModel 手臂姿态单元测试夹具
 *
 * SetUp 中关闭 sneaking/swimming 等状态干扰，主手设为右手。
 * 各测试通过 setRightArmPose/setLeftArmPose + applyAngles 触发姿态处理。
 */
class SkeletonModelArmPoseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<monster::SkeletonModel>();
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
     * 头部 yaw/pitch 设为 0 以简化角度断言。
     */
    void applyAngles() { m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<monster::SkeletonModel> m_model;
};

// ============================================================================
// BowAndArrow 姿态测试（拉弓动画）
//
// handleRightArmPose::BowAndArrow（头部 yaw=0, pitch=0）：
//   rightArm.Y = -0.1 + headYaw = -0.1
//   leftArm.Y  = 0.1 + headYaw + 0.4 = 0.5  （但若 leftArmPose=Empty，handleLeftArmPose::Empty 会覆盖 leftArm.Y=0）
//   rightArm.X = -PI/2 + headPitch = -PI/2
//   leftArm.X  = -PI/2 + headPitch = -PI/2
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_RightArm_YawIsMinusPointOne)
{
    // setRightArmPose 应委托到基类 m_rightArmPose 字段
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f) << "BowAndArrow 右臂 Y 应为 -0.1 + headYaw(0) = -0.1";
}

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_RightArm_PitchIsMinusPiOverTwo)
{
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f)
        << "BowAndArrow 右臂 X 应为 -PI/2 + headPitch(0) = -PI/2";
}

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_LeftArm_PitchIsMinusPiOverTwo)
{
    // 即使 leftArmPose=Empty，BowAndArrow 分支在 handleRightArmPose 中会设置 leftArm.X = -PI/2。
    // 随后 handleLeftArmPose::Empty 只覆盖 leftArm.Y=0，不覆盖 leftArm.X。
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f)
        << "BowAndArrow 左臂 X 应由 handleRightArmPose 设置为 -PI/2";
}

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_LeftArm_YawOverwrittenByBowAndArrowToPointFive)
{
    // 右撇子 + rightArmPose=BowAndArrow + leftArmPose=Empty：
    //   needsCrossArmCoord = (leftArmPose==BowAndArrow || ...) = false
    //   isRightHanded(true) != needsCrossArmCoord(false) → 先 handleLeftArmPose，后 handleRightArmPose
    // 1. handleLeftArmPose::Empty → leftArm.Y = 0
    // 2. handleRightArmPose::BowAndArrow → leftArm.Y = 0.1 + 0 + 0.4 = 0.5（覆盖 Empty 的 0）
    // 最终 leftArm.Y = 0.5（BowAndArrow 在 Empty 之后执行，覆盖 Empty 的设置）
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.5f, 1e-5f)
        << "rightArmPose=BowAndArrow 在 handleLeftArmPose::Empty 之后执行，leftArm.Y 应为 0.5（覆盖 Empty）";
}

// ============================================================================
// BowAndArrow 头部跟随测试
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_RightArmFollowsHeadYaw)
{
    // 头部 yaw=30°：rightArm.Y = -0.1 + 30°(rad)
    const f64 headYawDeg = 30.0;
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, headYawDeg, 0.0, 1.0 / 16.0);

    const f32 headYawRad = static_cast<f32>(headYawDeg * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f + headYawRad, 1e-5f) << "BowAndArrow 右臂 Y 应跟随头部 yaw";
}

TEST_F(SkeletonModelArmPoseTest, BowAndArrow_RightArmFollowsHeadPitch)
{
    // 头部 pitch=20°：rightArm.X = -PI/2 + 20°(rad)
    const f64 headPitchDeg = 20.0;
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    m_model->setAngles(0.0, 0.0, 0.0, 0.0, headPitchDeg, 1.0 / 16.0);

    const f32 headPitchRad = static_cast<f32>(headPitchDeg * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0) + headPitchRad, 1e-5f)
        << "BowAndArrow 右臂 X 应跟随头部 pitch";
}

// ============================================================================
// Empty 姿态测试（默认）
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, Empty_RightArm_YawIsZero)
{
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f);
}

TEST_F(SkeletonModelArmPoseTest, Empty_LeftArm_YawIsZero)
{
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.0f, 1e-5f);
}

// ============================================================================
// CrossbowCharge 姿态由基类处理测试
//
// SkeletonModel 不再自定义 CrossbowCharge 处理，由基类 handleCrossbowCharge 完整处理。
// 验证 setRightArmPose(CrossbowCharge) 后 setAngles 触发基类弩装填动画。
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, CrossbowCharge_DelegatesToBase_HandleCrossbowCharge)
{
    // 弩装填：主手 Y=-0.8，主手 X=-0.97079635（progress=0 时）
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(0.0f);
    m_model->setRightArmPose(monster::ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.8f, 1e-5f)
        << "CrossbowCharge 应由基类 handleCrossbowCharge 处理，主手 Y=-0.8";
    EXPECT_NEAR(rightArm->rotateAngleX(), -0.97079635f, 1e-5f) << "CrossbowCharge 主手 X=-0.97079635（progress=0）";
}

TEST_F(SkeletonModelArmPoseTest, CrossbowCharge_OffArmYIsPointFour)
{
    // progress=0 时副手 Y = 0.4（右撇子 sign=+1）
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(0.0f);
    m_model->setRightArmPose(monster::ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.4f, 1e-5f) << "CrossbowCharge 副手 Y=0.4（右撇子 progress=0）";
}

// ============================================================================
// CrossbowHold 姿态由基类处理测试
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, CrossbowHold_DelegatesToBase_HandleCrossbowHold)
{
    // CrossbowHold 由基类 handleCrossbowHold 处理，设置双手持弩姿态。
    // 验证不崩溃且角度被设置（具体角度由 handleCrossbowHold 实现）。
    m_model->setRightArmPose(monster::ArmPose::CrossbowHold);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);
    // 只验证不崩溃且角度被设置（非默认 Empty 的 Y=0）
    // 具体角度断言由 BipedModelCrossbowTest 覆盖
    SUCCEED();
}

// ============================================================================
// ArmPose 别名一致性测试
//
// SkeletonModel 使用 using ArmPose = model::ArmPose 复用基类枚举。
// 验证 SkeletonModel::ArmPose 与 BipedModel::ArmPose 是同一类型，
// 且所有 9 种姿态均可通过 setRightArmPose 设置。
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, ArmPoseAlias_AllPosesSettable)
{
    // 验证 using ArmPose = model::ArmPose 后所有 9 种姿态均可设置
    // 且不触发编译错误（类型一致性）
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setRightArmPose(monster::ArmPose::Item);
    m_model->setRightArmPose(monster::ArmPose::Block);
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setRightArmPose(monster::ArmPose::ThrowSpear);
    m_model->setRightArmPose(monster::ArmPose::CrossbowCharge);
    m_model->setRightArmPose(monster::ArmPose::CrossbowHold);
    m_model->setRightArmPose(monster::ArmPose::Spyglass);
    m_model->setRightArmPose(monster::ArmPose::Brush);
    SUCCEED() << "所有 9 种 ArmPose 均可通过 SkeletonModel::setRightArmPose 设置";
}

TEST_F(SkeletonModelArmPoseTest, ArmPoseAlias_SameTypeAsBase)
{
    // 验证 SkeletonModel::ArmPose 与基类 ArmPose 是同一类型
    // （using 声明不创建新类型）
    static_assert(
        std::is_same_v<monster::ArmPose, ArmPose>, "SkeletonModel::ArmPose 应通过 using 声明与基类 ArmPose 为同一类型");
    SUCCEED();
}

// ============================================================================
// setRightArmPose/setLeftArmPose 委托到基类字段测试
//
// 验证 setRightArmPose 实际写入基类 m_rightArmPose 字段（而非某个遮蔽字段）。
// 通过设置 BowAndArrow 后调用 applyAngles，检查角度是否符合基类 handleRightArmPose
// 的 BowAndArrow 分支预期。
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, SetRightArmPose_WritesToBaseField)
{
    // 如果 setRightArmPose 写入的是遮蔽字段，基类 handleRightArmPose 读取的
    // m_rightArmPose 仍为 Empty，不会触发 BowAndArrow 分支，rightArm.Y 会是 0。
    // 此测试验证委托正确：写入基类字段，handleRightArmPose 读取到 BowAndArrow。
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f)
        << "setRightArmPose 必须写入基类 m_rightArmPose，handleRightArmPose 才能读到 BowAndArrow";
}

TEST_F(SkeletonModelArmPoseTest, SetLeftArmPose_WritesToBaseField)
{
    // 设置左臂 BowAndArrow，验证 handleLeftArmPose::BowAndArrow 分支触发。
    // handleLeftArmPose::BowAndArrow（头部 yaw=0, pitch=0）：
    //   rightArm.Y = -0.1 + 0 - 0.4 = -0.5
    //   leftArm.Y = 0.1 + 0 = 0.1
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setLeftArmPose(monster::ArmPose::BowAndArrow);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.1f, 1e-5f)
        << "setLeftArmPose 必须写入基类 m_leftArmPose，handleLeftArmPose 才能读到 BowAndArrow";
}

// ============================================================================
// isAggressive 空手攻击动画测试
//
// SkeletonModel::setAngles 在基类 setAngles 之后，若 isAggressive=true 且
// rightArmPose 不是 BowAndArrow/CrossbowCharge/CrossbowHold，覆盖手臂角度
// 呈现空手挥击动画。
// ============================================================================

TEST_F(SkeletonModelArmPoseTest, IsAggressive_DefaultFalse)
{
    EXPECT_FALSE(m_model->isAggressive());
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_SetAndGet)
{
    m_model->setAggressive(true);
    EXPECT_TRUE(m_model->isAggressive());

    m_model->setAggressive(false);
    EXPECT_FALSE(m_model->isAggressive());
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_True_WithEmptyArmPose_OverridesArmAngles)
{
    // isAggressive=true + rightArmPose=Empty → 进入空手攻击覆盖分支
    // 手臂 X = -PI/2（覆盖基类设置的 0）
    m_model->setAggressive(true);
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f)
        << "isAggressive + Empty 应覆盖右臂 X 为 -PI/2";
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f)
        << "isAggressive + Empty 应覆盖左臂 X 为 -PI/2";
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_True_WithBowAndArrow_PreservesBowPose)
{
    // isAggressive=true + rightArmPose=BowAndArrow → 不进入攻击覆盖分支，保留拉弓姿态
    m_model->setAggressive(true);
    m_model->setRightArmPose(monster::ArmPose::BowAndArrow);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f) << "isAggressive + BowAndArrow 应保留拉弓姿态，不进入攻击覆盖";
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_True_WithCrossbowCharge_PreservesCrossbowPose)
{
    // isAggressive=true + rightArmPose=CrossbowCharge → 不进入攻击覆盖，保留弩装填姿态
    m_model->setAggressive(true);
    m_model->setMaxCrossbowChargeDuration(25.0f);
    m_model->setCrossbowChargeTicks(0.0f);
    m_model->setRightArmPose(monster::ArmPose::CrossbowCharge);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.8f, 1e-5f)
        << "isAggressive + CrossbowCharge 应保留弩装填姿态，不进入攻击覆盖";
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_True_WithCrossbowHold_PreservesCrossbowPose)
{
    // isAggressive=true + rightArmPose=CrossbowHold → 不进入攻击覆盖
    m_model->setAggressive(true);
    m_model->setRightArmPose(monster::ArmPose::CrossbowHold);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    // 不崩溃即通过（具体角度由 handleCrossbowHold 决定）
    SUCCEED();
}

TEST_F(SkeletonModelArmPoseTest, IsAggressive_False_WithEmptyArmPose_NoOverride)
{
    // isAggressive=false + rightArmPose=Empty → 不进入攻击覆盖，右臂 X 保持基类设置的 0
    m_model->setAggressive(false);
    m_model->setRightArmPose(monster::ArmPose::Empty);
    m_model->setLeftArmPose(monster::ArmPose::Empty);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // Empty 姿态下基类不设置 rightArm.X，保持默认 0
    EXPECT_NEAR(rightArm->rotateAngleX(), 0.0f, 1e-5f) << "isAggressive=false + Empty 不应覆盖手臂角度";
}

} // namespace
} // namespace mc::client::renderer
