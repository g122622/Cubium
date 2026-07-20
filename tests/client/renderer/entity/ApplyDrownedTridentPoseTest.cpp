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
 * @file ApplyDrownedTridentPoseTest.cpp
 * @brief EntityRendererManager::_applyDrownedTridentPose 契约单元测试
 *
 * _applyDrownedTridentPose 是 EntityRendererManager 的私有方法，其完整契约为：
 * 1. 读取 ClientEntity::isAggressive()
 * 2. 若 isAggressive=true：drownedModel.setRightArmPose(ArmPose::ThrowSpear)
 *    若 isAggressive=false：drownedModel.setRightArmPose(ArmPose::Empty)
 *
 * 注意：_applyDrownedTridentPose 本身不调用 setAngles（与 _applySkeletonArmPose 不同）。
 * setAngles 的重新调用由紧随其后的 _applyZombieState 完成。但为验证 ThrowSpear 姿态
 * 能正确驱动 DrownedModel::setAngles 的重应用逻辑（xRot = xRot*0.5 - PI, yRot = 0），
 * 本测试在设置 ArmPose 后手动调用 setAngles，模拟 _applyZombieState 的行为。
 *
 * 由于 EntityRendererManager 构造需要 Vulkan 管线，在单元测试中无法实例化。此处采用
 * 与 ApplySkeletonArmPoseTest 相同的策略：通过公开 API 复制方法逻辑，验证契约。
 *
 * 对应 MC 1.21.11 DrownedRenderer.getArmPose：
 *   isAggressive && mainHandItem.is(Items.TRIDENT) → THROW_TRIDENT
 * 本项目以 isAggressive 作为唯一触发信号（详见 _applyDrownedTridentPose 实现注释）。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/monster/MonsterVariantModels.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::core;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief 复制 EntityRendererManager::_applyDrownedTridentPose 的逻辑
 *
 * 此函数与 EntityRendererManager.cpp 中 _applyDrownedTridentPose 实现保持一致，
 * 用于在不实例化 EntityRendererManager 的情况下测试方法契约。设置 ArmPose 后
 * 重新调用 setAngles，模拟 _applyZombieState 在 _applyDrownedTridentPose 之后
 * 的重新调用行为。
 */
void applyDrownedTridentPoseContract(
    monster::DrownedModel& drownedModel, const mc::client::ClientEntity& entity, const AnimationContext& context)
{
    if (entity.isAggressive()) {
        drownedModel.setRightArmPose(ArmPose::ThrowSpear);
    } else {
        drownedModel.setRightArmPose(ArmPose::Empty);
    }
    // 模拟 _applyZombieState 的重新 setAngles 调用
    drownedModel.setAngles(context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale * 16.0);
}

/**
 * @brief _applyDrownedTridentPose 契约测试夹具
 *
 * SetUp 中关闭 sneaking，清零 swimAnimation/swingProgress，设置主手为右手，
 * 保证 aggressive=false、swingProgress=0、ageInTicks=0 条件下 ZombieModel 设置的
 * rightArm.xRot = -PI/2.25（bobArms 的 xRot 偏移 sin(0)*0.05=0），从而精确验证
 * ThrowSpear 重应用后的角度 = -PI/2.25 * 0.5 - PI。
 */
class ApplyDrownedTridentPoseTest : public ::testing::Test {
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

        m_entity = std::make_unique<mc::client::ClientEntity>(mc::EntityInstanceId(1), "minecraft:drowned");
        m_context = std::make_unique<AnimationContext>();
        m_context->limbSwing = 0.0;
        m_context->limbSwingAmount = 0.0;
        m_context->ageInTicks = 0.0;
        m_context->netHeadYaw = 0.0;
        m_context->headPitch = 0.0;
        m_context->scale = 1.0 / 16.0;
    }

    void TearDown() override
    {
        m_model.reset();
        m_entity.reset();
        m_context.reset();
    }

    std::unique_ptr<monster::DrownedModel> m_model;
    std::unique_ptr<mc::client::ClientEntity> m_entity;
    std::unique_ptr<AnimationContext> m_context;
};

// ============================================================================
// isAggressive=true → ThrowSpear 姿态测试
// ============================================================================
//
// aggressive=false、swingProgress=0、ageInTicks=0 条件下：
//   ZombieModel::setAngles 设置 rightArm.xRot = -PI/2.25
//   DrownedModel::setAngles 重应用 ThrowSpear：rightArm.xRot = -PI/2.25 * 0.5 - PI
//   rightArm.yRot = 0（animateZombieArms 设置 -0.1，ThrowSpear 覆盖为 0）
// ============================================================================

TEST_F(ApplyDrownedTridentPoseTest, AggressiveTrue_SetsRightArmThrowSpear_XRot)
{
    m_entity->setIsAggressive(true);
    ASSERT_TRUE(m_entity->isAggressive());

    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 base = static_cast<f32>(-PI_DOUBLE / 2.25);
    const f32 expected = base * 0.5f - static_cast<f32>(PI_DOUBLE);
    EXPECT_NEAR(rightArm->rotateAngleX(), expected, 1e-4f)
        << "isAggressive=true 时 _applyDrownedTridentPose 应设置右臂 ThrowSpear（X=base*0.5-PI）";
}

TEST_F(ApplyDrownedTridentPoseTest, AggressiveTrue_SetsRightArmThrowSpear_YRotZero)
{
    m_entity->setIsAggressive(true);
    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-6f)
        << "isAggressive=true 时 ThrowSpear 应将右臂 Y 覆盖为 0（animateZombieArms 的 -0.1 被清除）";
}

// ============================================================================
// isAggressive=false → Empty 姿态测试
// ============================================================================

TEST_F(ApplyDrownedTridentPoseTest, AggressiveFalse_SetsRightArmEmpty_XRotKeepsZombieBase)
{
    m_entity->setIsAggressive(false);
    ASSERT_FALSE(m_entity->isAggressive());

    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // Empty 姿态不触发 ThrowSpear 重应用，rightArm.xRot 保持 ZombieModel 的 -PI/2.25
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "isAggressive=false 时 _applyDrownedTridentPose 应设置右臂 Empty（X 保持 -PI/2.25）";
}

TEST_F(ApplyDrownedTridentPoseTest, AggressiveFalse_RightArmYRotKeepsZombieBase)
{
    m_entity->setIsAggressive(false);
    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // Empty 姿态不触发 ThrowSpear 重应用，rightArm.yRot 保持 animateZombieArms 的 -0.1
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f)
        << "isAggressive=false 时右臂 Y 应保持 animateZombieArms 的 -0.1";
}

// ============================================================================
// 切换 isAggressive 状态测试
// ============================================================================

TEST_F(ApplyDrownedTridentPoseTest, ToggleAggressive_TrueThenFalse_UpdatesArmPose)
{
    // 第一次：isAggressive=true → ThrowSpear
    m_entity->setIsAggressive(true);
    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    const f32 throwSpearX = static_cast<f32>(-PI_DOUBLE / 2.25) * 0.5f - static_cast<f32>(PI_DOUBLE);
    EXPECT_NEAR(rightArm->rotateAngleX(), throwSpearX, 1e-4f);

    // 第二次：isAggressive=false → Empty
    m_entity->setIsAggressive(false);
    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);
    rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "切换 isAggressive=false 后右臂 X 应回到 -PI/2.25（Empty 姿态）";
}

// ============================================================================
// 左臂不受 ThrowSpear 影响测试（仅设置右臂）
// ============================================================================

TEST_F(ApplyDrownedTridentPoseTest, AggressiveTrue_LeftArmNotAffected)
{
    m_entity->setIsAggressive(true);
    applyDrownedTridentPoseContract(*m_model, *m_entity, *m_context);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    // 左臂 ArmPose 保持 Empty，不触发 ThrowSpear 重应用
    // leftArm.xRot 保持 ZombieModel 的 -PI/2.25
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.25), 1e-5f)
        << "_applyDrownedTridentPose 仅设置右臂 ThrowSpear，左臂 X 应保持 -PI/2.25";
}

} // namespace
} // namespace mc::client::renderer
