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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file BipedModelSpeedValueTest.cpp
 * @brief BipedModel 鞘翅飞行速度因子（speedValue）单元测试
 *
 * 验证 BipedModel::setAngles 中速度因子 f 的计算与使用，对应 MC 1.21.11
 * HumanoidMobRenderer.extractHumanoidRenderState 中 speedValue 的填充逻辑：
 *
 *   speedValue = 1.0F;
 *   if (isFallFlying) {
 *       speedValue = (float)deltaMovement.lengthSqr();
 *       speedValue /= 0.2F;
 *       speedValue = speedValue * (speedValue * speedValue);  // 立方
 *   }
 *   if (speedValue < 1.0F) speedValue = 1.0F;
 *
 * BipedModel 中 f 用作手臂/腿部摆动振幅的除数：
 *   rightArm.xRot = cos(limbSwing * 0.6662 + PI) * 2.0 * limbSwingAmount * 0.5 / f
 *   leftArm.xRot  = cos(limbSwing * 0.6662)        * 2.0 * limbSwingAmount * 0.5 / f
 *   rightLeg.xRot = cos(limbSwing * 0.6662)        * 1.4 * limbSwingAmount / f
 *   leftLeg.xRot  = cos(limbSwing * 0.6662 + PI)   * 1.4 * limbSwingAmount / f
 *
 * 测试通过 setFallFlying / setSpeedValue + setAngles 触发动画计算，断言手臂/腿部角度
 * 反映 1/f 缩放。同时验证 isFallFlying 时头部角度为 -π/4。
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
 * @brief BipedModel 鞘翅飞行速度因子单元测试夹具
 *
 * SetUp 中关闭 sneaking/swimming 等其他状态对角度的干扰，并将主手设为右手。
 */
class BipedModelSpeedValueTest : public ::testing::Test {
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
     * @brief 触发一次 setAngles 让模型应用当前状态
     *
     * 头部 yaw/pitch 设为 0 以简化头部角度断言。
     * limbSwing=0.5 / limbSwingAmount=0.4 提供非零步态参数便于观察缩放效果。
     */
    void applyAngles()
    {
        constexpr f64 limbSwing = 0.5;
        constexpr f64 limbSwingAmount = 0.4;
        m_model->setAngles(limbSwing, limbSwingAmount, 0.0, 0.0, 0.0, 1.0 / 16.0);
    }

    /**
     * @brief 计算给定 limbSwing/limbSwingAmount/f 下右臂 X 角度的期望值
     *
     * 对应 BipedModel::setAngles 中的公式：
     *   rightArm.xRot = cos(limbSwing * 0.6662 + PI) * 2.0 * limbSwingAmount * 0.5 / f
     */
    static f32 expectedRightArmX(f64 limbSwing, f64 limbSwingAmount, f32 f)
    {
        return static_cast<f32>(std::cos(limbSwing * 0.6662 + PI_DOUBLE) * 2.0 * limbSwingAmount * 0.5 / f);
    }

    /**
     * @brief 计算给定 limbSwing/limbSwingAmount/f 下左臂 X 角度的期望值
     */
    static f32 expectedLeftArmX(f64 limbSwing, f64 limbSwingAmount, f32 f)
    {
        return static_cast<f32>(std::cos(limbSwing * 0.6662) * 2.0 * limbSwingAmount * 0.5 / f);
    }

    /**
     * @brief 计算给定 limbSwing/limbSwingAmount/f 下右腿 X 角度的期望值
     */
    static f32 expectedRightLegX(f64 limbSwing, f64 limbSwingAmount, f32 f)
    {
        return static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount / f);
    }

    /**
     * @brief 计算给定 limbSwing/limbSwingAmount/f 下左腿 X 角度的期望值
     */
    static f32 expectedLeftLegX(f64 limbSwing, f64 limbSwingAmount, f32 f)
    {
        return static_cast<f32>(std::cos(limbSwing * 0.6662 + PI_DOUBLE) * 1.4 * limbSwingAmount / f);
    }

    std::unique_ptr<BipedModel> m_model;
};

// ========== 默认值：非鞘翅飞行时 f 应为 1.0 ==========

TEST_F(BipedModelSpeedValueTest, Default_SpeedValueIsOne_ArmLegAnglesUnscaled)
{
    // 未设置 setSpeedValue 时，默认 m_speedValue = 1.0
    // 手臂/腿部角度应按 f=1.0 计算
    applyAngles();

    constexpr f64 limbSwing = 0.5;
    constexpr f64 limbSwingAmount = 0.4;
    constexpr f32 f = 1.0f;

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    auto rightLeg = m_model->getRightLeg();
    auto leftLeg = m_model->getLeftLeg();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);
    ASSERT_NE(rightLeg, nullptr);
    ASSERT_NE(leftLeg, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleX(), expectedRightArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleX(), expectedLeftArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(rightLeg->rotateAngleX(), expectedRightLegX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftLeg->rotateAngleX(), expectedLeftLegX(limbSwing, limbSwingAmount, f), 1e-5f);
}

// ========== 鞘翅飞行时头部角度应为 -π/4 ==========

TEST_F(BipedModelSpeedValueTest, FallFlying_HeadAngleIsNegativePiOver4)
{
    // setFallFlying(true) 后，setAngles 应将头部 X 角度设为 -π/4
    m_model->setFallFlying(true);
    applyAngles();

    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_NEAR(head->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 4.0), 1e-5f);
}

// ========== setFallFlying(false) 时头部角度应保持 headPitch ==========

TEST_F(BipedModelSpeedValueTest, NotFallFlying_HeadAngleIsHeadPitch)
{
    // 非鞘翅飞行时，头部 X 角度应等于 headPitch（弧度），此处 headPitch=0
    m_model->setFallFlying(false);
    applyAngles();

    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_NEAR(head->rotateAngleX(), 0.0f, 1e-5f);
}

// ========== speedValue > 1 应缩放手臂/腿部角度 ==========

TEST_F(BipedModelSpeedValueTest, FallFlying_SpeedValueTwo_ArmLegAnglesHalved)
{
    // setSpeedValue(2.0) 应使手臂/腿部角度按 1/2 缩放
    constexpr f32 f = 2.0f;
    m_model->setSpeedValue(f);
    m_model->setFallFlying(true);
    applyAngles();

    constexpr f64 limbSwing = 0.5;
    constexpr f64 limbSwingAmount = 0.4;

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    auto rightLeg = m_model->getRightLeg();
    auto leftLeg = m_model->getLeftLeg();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);
    ASSERT_NE(rightLeg, nullptr);
    ASSERT_NE(leftLeg, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleX(), expectedRightArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleX(), expectedLeftArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(rightLeg->rotateAngleX(), expectedRightLegX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftLeg->rotateAngleX(), expectedLeftLegX(limbSwing, limbSwingAmount, f), 1e-5f);
}

// ========== speedValue < 1 应被钳制到 1.0（f >= 1.0 不变量） ==========

TEST_F(BipedModelSpeedValueTest, SpeedValueBelowOne_ClampedToOne)
{
    // setSpeedValue(0.5) 应被钳制到 1.0，与未设置 speedValue 等价
    m_model->setSpeedValue(0.5f);
    applyAngles();

    constexpr f64 limbSwing = 0.5;
    constexpr f64 limbSwingAmount = 0.4;
    constexpr f32 f = 1.0f; // 钳制后

    auto rightArm = m_model->getRightArm();
    auto leftArm = m_model->getLeftArm();
    auto rightLeg = m_model->getRightLeg();
    auto leftLeg = m_model->getLeftLeg();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(leftArm, nullptr);
    ASSERT_NE(rightLeg, nullptr);
    ASSERT_NE(leftLeg, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleX(), expectedRightArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftArm->rotateAngleX(), expectedLeftArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(rightLeg->rotateAngleX(), expectedRightLegX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(leftLeg->rotateAngleX(), expectedLeftLegX(limbSwing, limbSwingAmount, f), 1e-5f);
}

// ========== 速度因子计算公式验证 ==========
//
// 对应 MC 1.21.11 HumanoidMobRenderer.extractHumanoidRenderState:
//   speedValue = 1.0F;
//   if (isFallFlying) {
//       speedValue = (float)deltaMovement.lengthSqr();
//       speedValue /= 0.2F;
//       speedValue = speedValue * (speedValue * speedValue);  // 立方
//   }
//   if (speedValue < 1.0F) speedValue = 1.0F;
//
// 此处验证渲染器（PlayerRenderer::_applyBipedElytraState 或 PlayerRenderer::setModelVisibilities）
// 应推送的 speedValue 公式。测试通过模拟渲染器计算后调用 setSpeedValue 验证。

TEST_F(BipedModelSpeedValueTest, SpeedValueFormula_VelocityZero_FallFlying_ClampedToOne)
{
    // 速度为 0 时：lengthSq=0, speedValue = 0/0.2 = 0, 立方=0, 钳制到 1.0
    constexpr f32 lengthSq = 0.0f;
    constexpr f32 speedDivisor = 0.2f;
    f32 speedValue = lengthSq / speedDivisor;
    speedValue = speedValue * speedValue * speedValue;
    if (speedValue < 1.0f) {
        speedValue = 1.0f;
    }
    EXPECT_NEAR(speedValue, 1.0f, 1e-5f);

    m_model->setSpeedValue(speedValue);
    m_model->setFallFlying(true);
    applyAngles();

    // 头部角度应反映鞘翅飞行（-π/4）
    auto head = m_model->getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_NEAR(head->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 4.0), 1e-5f);
}

TEST_F(BipedModelSpeedValueTest, SpeedValueFormula_VelocityModerate_ProducesLargeDivisor)
{
    // 速度长度平方 = 1.0（约 1 m/tick），speedValue = (1.0/0.2)^3 = 125
    // 手臂/腿部摆动应被显著缩放（除以 125）
    constexpr f32 lengthSq = 1.0f;
    constexpr f32 speedDivisor = 0.2f;
    f32 speedValue = lengthSq / speedDivisor;
    speedValue = speedValue * speedValue * speedValue;
    EXPECT_NEAR(speedValue, 125.0f, 1e-3f);

    m_model->setSpeedValue(speedValue);
    m_model->setFallFlying(true);
    applyAngles();

    constexpr f64 limbSwing = 0.5;
    constexpr f64 limbSwingAmount = 0.4;
    const f32 f = speedValue; // 125.0

    auto rightArm = m_model->getRightArm();
    auto rightLeg = m_model->getRightLeg();
    ASSERT_NE(rightArm, nullptr);
    ASSERT_NE(rightLeg, nullptr);

    EXPECT_NEAR(rightArm->rotateAngleX(), expectedRightArmX(limbSwing, limbSwingAmount, f), 1e-5f);
    EXPECT_NEAR(rightLeg->rotateAngleX(), expectedRightLegX(limbSwing, limbSwingAmount, f), 1e-5f);
}

// ========== copyModelAttributesTo 应复制 speedValue/fallFlying 字段 ==========

TEST_F(BipedModelSpeedValueTest, CopyModelAttributes_CopiesSpeedValueAndFallFlying)
{
    m_model->setSpeedValue(2.5f);
    m_model->setFallFlying(true);

    BipedModel target;
    m_model->copyModelAttributesTo(target);

    // 由于 setter 是 private 字段直接访问，需要通过 setAngles 后的行为验证
    // 简单验证：target 在 setAngles 后头部角度应为 -π/4（说明 m_isFallFlying 已复制）
    target.setSneaking(false);
    target.setSwimAnimation(0.0f);
    target.setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0 / 16.0);

    auto head = target.getModelHead();
    ASSERT_NE(head, nullptr);
    EXPECT_NEAR(head->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 4.0), 1e-5f);

    // 同时验证 speedValue 被复制：target 在 setAngles 后手臂角度应按 1/2.5 缩放
    constexpr f64 limbSwing = 0.5;
    constexpr f64 limbSwingAmount = 0.4;
    target.setAngles(limbSwing, limbSwingAmount, 0.0, 0.0, 0.0, 1.0 / 16.0);
    auto rightArm = target.getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), expectedRightArmX(limbSwing, limbSwingAmount, 2.5f), 1e-5f);
}

} // namespace
} // namespace mc::client::renderer
