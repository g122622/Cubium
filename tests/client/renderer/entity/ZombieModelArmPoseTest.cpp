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
 * @file ZombieModelArmPoseTest.cpp
 * @brief ZombieModel::setAngles 激怒状态手臂动画单元测试
 *
 * 验证 ZombieModel::setAngles 按 MC 1.21.11 AnimationUtils.animateZombieArms 正确实现：
 *   f1 = -PI / (m_isAggressive ? 1.5 : 2.25)   // 手臂前伸基础角度
 *   f2 = sin(swingProgress * PI)
 *   f3 = sin((1 - (1-swingProgress)^2) * PI)
 *   rightArm: zRot=0, yRot=-(0.1 - f2*0.6), xRot=f1 + f2*1.2 - f3*0.4
 *   leftArm:  zRot=0, yRot= (0.1 - f2*0.6), xRot=f1 + f2*1.2 - f3*0.4
 *   bobArms(rightArm, leftArm, ageInTicks)  // 无条件执行
 *
 * 测试在 swingProgress=0、ageInTicks=0 条件下：
 *   f2 = sin(0) = 0
 *   f3 = sin(0) = 0
 *   bobArms: cos(0)*0.05+0.05 = 0.1 (zRot 偏移), sin(0)*0.05 = 0 (xRot 偏移)
 *   → rightArm.xRot = f1, rightArm.yRot = -0.1, rightArm.zRot = 0.1
 *   → leftArm.xRot  = f1, leftArm.yRot  =  0.1, leftArm.zRot = -0.1
 *
 * 因此 aggressive=true 时 rightArm.xRot ≈ -PI/1.5，aggressive=false 时 ≈ -PI/2.25。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/monster/MonsterVariantModels.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief ZombieModel 手臂姿态单元测试夹具
 *
 * SetUp 中关闭 sneaking/swimming 等状态干扰，保持 ArmPose=Empty（默认），
 * 使 BipedModel::setAngles 设置的手臂角度被 ZombieModel::setAngles 完全覆盖。
 * ageInTicks 与 swingProgress 均设为 0 以消除 bobArms 与攻击因子干扰，
 * 使 rightArm.xRot 恰好等于 f1 = -PI/(aggressive?1.5:2.25)。
 */
class ZombieModelArmPoseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<monster::ZombieModel>();
        m_model->setSneaking(false);
        m_model->setSwimAnimation(0.0f);
        m_model->setSwingProgress(0.0f);
        using HS = mc::client::renderer::entity::model::HandSide;
        m_model->setMainHand(HS::Right);
        m_model->setSwingingHand(HS::Right);
    }

    /**
     * @brief 触发一次 setAngles 让模型应用当前 aggressive 状态
     *
     * 所有参数设为 0（headYaw/pitch/limbSwing/ageInTicks），scale 取 1/16。
     * ageInTicks=0 使 bobArms 的 sin(0*0.067)*0.05=0，cos(0*0.09)*0.05+0.05=0.1。
     */
    void applyAngles() { m_model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0); }

    std::unique_ptr<monster::ZombieModel> m_model;
};

// ============================================================================
// aggressive 状态 getter/setter 测试
// ============================================================================

TEST_F(ZombieModelArmPoseTest, IsAggressive_DefaultFalse)
{
    EXPECT_FALSE(m_model->isAggressive());
}

TEST_F(ZombieModelArmPoseTest, SetAggressive_True_GetterReturnsTrue)
{
    m_model->setAggressive(true);
    EXPECT_TRUE(m_model->isAggressive());
}

TEST_F(ZombieModelArmPoseTest, SetAggressive_False_GetterReturnsFalse)
{
    m_model->setAggressive(true);
    m_model->setAggressive(false);
    EXPECT_FALSE(m_model->isAggressive());
}

// ============================================================================
// aggressive=true 手臂角度测试（基础角度 -PI/1.5）
// ============================================================================
//
// 在 swingProgress=0、ageInTicks=0 条件下：
//   f1 = -PI/1.5 ≈ -2.0943951
//   f2 = sin(0*PI) = 0
//   f3 = sin((1-(1-0)^2)*PI) = sin(0) = 0
//   rightArm.xRot = f1 + f2*1.2 - f3*0.4 = f1 = -PI/1.5
//   bobArms 右臂 xRot 偏移 = sin(0*0.067)*0.05 = 0
//   → rightArm.xRot = -PI/1.5
//   rightArm.yRot = -(0.1 - 0*0.6) = -0.1
//   rightArm.zRot = 0 + (cos(0*0.09)*0.05 + 0.05) = 0.1
// ============================================================================

TEST_F(ZombieModelArmPoseTest, Aggressive_True_RightArm_XIsMinusPiOver1Point5)
{
    m_model->setAggressive(true);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 1.5), 1e-5f)
        << "aggressive=true 时右臂 X 应为 -PI/1.5（抬臂更高）";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_True_LeftArm_XIsMinusPiOver1Point5)
{
    m_model->setAggressive(true);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 1.5), 1e-5f)
        << "aggressive=true 时左臂 X 应为 -PI/1.5";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_True_RightArm_YIsMinusPointOne)
{
    m_model->setAggressive(true);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f) << "右臂 Y 应为 -(0.1 - 0*0.6) = -0.1";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_True_LeftArm_YIsPointOne)
{
    m_model->setAggressive(true);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.1f, 1e-5f) << "左臂 Y 应为 (0.1 - 0*0.6) = 0.1";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_True_RightArm_ZIsPointOne_FromBobArms)
{
    // bobArms 在 ageInTicks=0 时：rightArm.zRot += cos(0)*0.05 + 0.05 = 0.1
    m_model->setAggressive(true);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleZ(), 0.1f, 1e-5f) << "右臂 Z 应为 0（重置）+ bobArms(cos(0)*0.05+0.05)=0.1";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_True_LeftArm_ZIsMinusPointOne_FromBobArms)
{
    // bobArms 在 ageInTicks=0 时：leftArm.zRot -= cos(0)*0.05 + 0.05 = -0.1
    m_model->setAggressive(true);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleZ(), -0.1f, 1e-5f) << "左臂 Z 应为 0 - bobArms(0.1) = -0.1";
}

// ============================================================================
// aggressive=false 手臂角度测试（基础角度 -PI/2.25）
// ============================================================================
//
// 在 swingProgress=0、ageInTicks=0 条件下：
//   f1 = -PI/2.25 ≈ -1.3962634
//   f2 = 0, f3 = 0
//   rightArm.xRot = f1 = -PI/2.25
// ============================================================================

TEST_F(ZombieModelArmPoseTest, Aggressive_False_RightArm_XIsMinusPiOver2Point25)
{
    m_model->setAggressive(false);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "aggressive=false 时右臂 X 应为 -PI/2.25（抬臂较低）";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_False_LeftArm_XIsMinusPiOver2Point25)
{
    m_model->setAggressive(false);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "aggressive=false 时左臂 X 应为 -PI/2.25";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_False_RightArm_YIsMinusPointOne)
{
    m_model->setAggressive(false);
    applyAngles();

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f)
        << "aggressive=false 时右臂 Y 仍为 -0.1（yRot 与 aggressive 无关）";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_False_LeftArm_YIsPointOne)
{
    m_model->setAggressive(false);
    applyAngles();

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleY(), 0.1f, 1e-5f) << "aggressive=false 时左臂 Y 仍为 0.1";
}

// ============================================================================
// aggressive 状态切换测试（验证 xRot 在两种基值间切换）
// ============================================================================

TEST_F(ZombieModelArmPoseTest, Aggressive_ToggleTrueFalse_XRotChanges)
{
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);

    m_model->setAggressive(true);
    applyAngles();
    const f32 xRotAggressive = rightArm->rotateAngleX();
    EXPECT_NEAR(xRotAggressive, static_cast<f32>(-PI_DOUBLE / 1.5), 1e-5f);

    m_model->setAggressive(false);
    applyAngles();
    const f32 xRotCalm = rightArm->rotateAngleX();
    EXPECT_NEAR(xRotCalm, static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f);

    // aggressive 时手臂抬得更高（更接近垂直），故 |xRotAggressive| > |xRotCalm|
    EXPECT_LT(xRotAggressive, xRotCalm) << "aggressive=true 时手臂 X 应更负（抬臂更高）";
}

TEST_F(ZombieModelArmPoseTest, Aggressive_ToggleFalseTrue_XRotChanges)
{
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);

    m_model->setAggressive(false);
    applyAngles();
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f);

    m_model->setAggressive(true);
    applyAngles();
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 1.5), 1e-5f);
}

// ============================================================================
// swingProgress 影响测试（验证 f2/f3 攻击因子叠加）
// ============================================================================
//
// 当 swingProgress=0.5、ageInTicks=0、aggressive=false：
//   f1 = -PI/2.25
//   f2 = sin(0.5*PI) = 1.0
//   f3 = sin((1-(1-0.5)^2)*PI) = sin((1-0.25)*PI) = sin(0.75*PI) ≈ 0.70710678
//   rightArm.xRot = f1 + f2*1.2 - f3*0.4 = -PI/2.25 + 1.2 - 0.4*0.70710678
//   bobArms 右臂 xRot 偏移 = sin(0*0.067)*0.05 = 0
// ============================================================================

TEST_F(ZombieModelArmPoseTest, SwingProgress_Half_AggressiveFalse_AppliesAttackFactors)
{
    m_model->setAggressive(false);
    m_model->setSwingProgress(0.5f);
    applyAngles();

    const f32 f1 = static_cast<f32>(-PI_DOUBLE / 2.25);
    const f32 f2 = static_cast<f32>(std::sin(0.5 * PI_DOUBLE));                               // = 1.0
    const f32 f3 = static_cast<f32>(std::sin((1.0 - (1.0 - 0.5) * (1.0 - 0.5)) * PI_DOUBLE)); // sin(0.75*PI)
    const f32 expectedX = f1 + f2 * 1.2f - f3 * 0.4f;

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedX, 1e-4f)
        << "swingProgress=0.5 时应叠加 f2*1.2 - f3*0.4 攻击因子到右臂 X";
}

TEST_F(ZombieModelArmPoseTest, SwingProgress_Half_AggressiveTrue_AppliesAttackFactorsWithHigherBase)
{
    m_model->setAggressive(true);
    m_model->setSwingProgress(0.5f);
    applyAngles();

    const f32 f1 = static_cast<f32>(-PI_DOUBLE / 1.5);
    const f32 f2 = static_cast<f32>(std::sin(0.5 * PI_DOUBLE));
    const f32 f3 = static_cast<f32>(std::sin((1.0 - (1.0 - 0.5) * (1.0 - 0.5)) * PI_DOUBLE));
    const f32 expectedX = f1 + f2 * 1.2f - f3 * 0.4f;

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedX, 1e-4f)
        << "aggressive=true + swingProgress=0.5 应使用 -PI/1.5 基值并叠加攻击因子";
}

// ============================================================================
// bobArms 无条件执行测试（ageInTicks 影响手臂抖动）
// ============================================================================
//
// bobArms 在 ageInTicks 非零时附加抖动：
//   rightArm.zRot += cos(age*0.09)*0.05 + 0.05
//   rightArm.xRot += sin(age*0.067)*0.05
// 验证 ageInTicks=10 时抖动偏移被应用（aggressive 状态不影响 bobArms）。
// ============================================================================

TEST_F(ZombieModelArmPoseTest, BobArms_AgeInTicksNonZero_AppliesWobbleToZRot)
{
    m_model->setAggressive(false);
    m_model->setSwingProgress(0.0f);
    const f64 ageInTicks = 10.0;
    m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0 / 16.0);

    const f32 expectedZ = static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleZ(), expectedZ, 1e-5f)
        << "bobArms 应无条件执行，rightArm.zRot = cos(age*0.09)*0.05+0.05";
}

TEST_F(ZombieModelArmPoseTest, BobArms_AgeInTicksNonZero_AppliesWobbleToXRot)
{
    m_model->setAggressive(true);
    m_model->setSwingProgress(0.0f);
    const f64 ageInTicks = 10.0;
    m_model->setAngles(0.0, 0.0, ageInTicks, 0.0, 0.0, 1.0 / 16.0);

    const f32 f1 = static_cast<f32>(-PI_DOUBLE / 1.5);
    const f32 bobX = static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05);
    const f32 expectedX = f1 + bobX;

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedX, 1e-5f)
        << "bobArms 应无条件叠加 sin(age*0.067)*0.05 到 rightArm.xRot";
}

// ============================================================================
// 变体模型继承测试（HuskModel/DrownedModel/ZombieVillagerModel/GiantModel 继承 ZombieModel）
// ============================================================================

TEST(ZombieModelVariantInheritanceTest, HuskModel_IsZombieModelSubclass)
{
    auto husk = std::make_unique<monster::HuskModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(husk.get());
    EXPECT_NE(zombieBase, nullptr) << "HuskModel 应继承自 ZombieModel，可被 dynamic_cast 命中";
}

TEST(ZombieModelVariantInheritanceTest, DrownedModel_IsZombieModelSubclass)
{
    auto drowned = std::make_unique<monster::DrownedModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(drowned.get());
    EXPECT_NE(zombieBase, nullptr) << "DrownedModel 应继承自 ZombieModel，可被 dynamic_cast 命中";
}

TEST(ZombieModelVariantInheritanceTest, ZombieVillagerModel_IsZombieModelSubclass)
{
    auto zv = std::make_unique<monster::ZombieVillagerModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(zv.get());
    EXPECT_NE(zombieBase, nullptr) << "ZombieVillagerModel 应继承自 ZombieModel，可被 dynamic_cast 命中";
}

TEST(ZombieModelVariantInheritanceTest, GiantModel_IsZombieModelSubclass)
{
    auto giant = std::make_unique<monster::GiantModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(giant.get());
    EXPECT_NE(zombieBase, nullptr) << "GiantModel 应继承自 ZombieModel，可被 dynamic_cast 命中";
}

TEST(ZombieModelVariantInheritanceTest, HuskModel_InheritsSetAggressive)
{
    // HuskModel 继承 ZombieModel，应拥有 setAggressive/isAggressive 方法
    auto husk = std::make_unique<monster::HuskModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(husk.get());
    ASSERT_NE(zombieBase, nullptr);
    EXPECT_FALSE(zombieBase->isAggressive());
    zombieBase->setAggressive(true);
    EXPECT_TRUE(zombieBase->isAggressive());
}

TEST(ZombieModelVariantInheritanceTest, HuskModel_AggressiveTrue_AppliesZombieArmAnimation)
{
    // HuskModel 继承 ZombieModel::setAngles，aggressive=true 时手臂角度应为 -PI/1.5
    auto husk = std::make_unique<monster::HuskModel>();
    auto* zombieBase = dynamic_cast<monster::ZombieModel*>(husk.get());
    ASSERT_NE(zombieBase, nullptr);

    zombieBase->setAggressive(true);
    zombieBase->setSwingProgress(0.0f);
    zombieBase->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);

    auto rightArm = zombieBase->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 1.5), 1e-5f)
        << "HuskModel 应通过继承 ZombieModel::setAngles 获得 aggressive=true 攻击抬臂动画";
}

} // namespace
} // namespace mc::client::renderer
