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
 * @file ApplySkeletonArmPoseTest.cpp
 * @brief EntityRendererManager::_applySkeletonArmPose 契约单元测试
 *
 * _applySkeletonArmPose 是 EntityRendererManager 的私有方法，其完整契约为：
 * 1. 读取 ClientEntity::isAggressive() 与主手持弓状态（getMainHandItem()->getItem()==Items::BOW）
 * 2. 若 isAggressive=true 且持弓：skeletonModel.setRightArmPose(ArmPose::BowAndArrow)
 *    否则：skeletonModel.setRightArmPose(ArmPose::Empty)
 * 3. 重新调用 skeletonModel.setAngles(context.limbSwing, context.limbSwingAmount,
 *    context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0)
 *
 * 由于 EntityRendererManager 构造需要 Vulkan 管线（EntityPipeline、纹理图集、
 * 命令缓冲区等），在单元测试中无法实例化。此处采用与 ElytraSpeedValueTest 相同的
 * 策略：通过公开 API（ClientEntity::isAggressive + setMainHandItem +
 * SkeletonModel::setRightArmPose + SkeletonModel::setAngles）复制方法逻辑，验证
 * _applySkeletonArmPose 声称履行的契约。
 *
 * 数据流验证：
 *   ClientEntity::isAggressive() + getMainHandItem() → setRightArmPose(BowAndArrow/Empty)
 *   → setAngles → handleRightArmPose → 右臂 Y/X 旋转角度
 *
 * 对应 MC 1.21.11 AbstractSkeletonRenderer.getArmPose：
 *   isAggressive && mainHandItem.is(Items.BOW) → BOW_AND_ARROW
 *
 * 注：原项目曾用独立 DATA_CHARGING_BOW_PARAM(id16) 同步拉弓状态，但 vanilla
 * AbstractSkeleton/Stray/WitherSkeleton 客户端 SynchedEntityData 数组长度=16
 * （无 id16 字段），发送 id16 致真 Java 客户端 set_entity_data
 * "Index 16 out of bounds for length 16" 崩溃，故改回 vanilla 的
 * isAggressive + 持弓判定。isAggressive 经 MobEntity::DATA_MOB_FLAGS_PARAM 位 2
 * 同步，由 RangedBowAttackGoal::startExecuting/resetTask 经 setAggroed 写入。
 */

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/MathConstants.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::model;
using namespace mc::client::renderer::entity::core;
using namespace mc::math;

namespace mc::client::renderer {
namespace {

/**
 * @brief 复制 EntityRendererManager::_applySkeletonArmPose 的逻辑
 *
 * 此函数与 EntityRendererManager.cpp 中 _applySkeletonArmPose 实现保持一致，
 * 用于在不实例化 EntityRendererManager 的情况下测试方法契约。
 *
 * @param skeletonModel 已创建的骷髅模型
 * @param entity 客户端实体（提供 isAggressive + 主手持弓判定）
 * @param context 动画上下文（提供 limbSwing 等）
 */
void applySkeletonArmPoseContract(
    monster::SkeletonModel& skeletonModel, const mc::client::ClientEntity& entity, const AnimationContext& context)
{
    const bool holdingBow = [&] {
        const mc::ItemStack* mainHand = entity.getMainHandItem();
        return mainHand != nullptr && mainHand->getItem() == mc::Items::BOW;
    }();
    if (entity.isAggressive() && holdingBow) {
        skeletonModel.setRightArmPose(ArmPose::BowAndArrow);
    } else {
        skeletonModel.setRightArmPose(ArmPose::Empty);
    }
    // 重新调用 setAngles 让 ArmPose 通过 handleRightArmPose/handleLeftArmPose 生效
    skeletonModel.setAngles(context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale * 16.0);
}

/**
 * @brief _applySkeletonArmPose 契约测试夹具
 */
class ApplySkeletonArmPoseTest : public ::testing::Test {
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

        m_entity = std::make_unique<mc::client::ClientEntity>(mc::EntityInstanceId(1), "minecraft:skeleton");
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

    /// 给实体主手装备弓（对齐 vanilla 骷髅拉弓前置条件）
    void equipBow() { m_entity->setMainHandItem(mc::ItemStack(*mc::Items::BOW, 1)); }

    std::unique_ptr<monster::SkeletonModel> m_model;
    std::unique_ptr<mc::client::ClientEntity> m_entity;
    std::unique_ptr<AnimationContext> m_context;
};

// ============================================================================
// isAggressive=true + 持弓 → BowAndArrow 姿态测试
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithBow_SetsRightArmBowAndArrow)
{
    // 设置 ClientEntity::isAggressive=true + 主手持弓
    equipBow();
    m_entity->setIsAggressive(true);
    ASSERT_TRUE(m_entity->isAggressive());

    // 执行 _applySkeletonArmPose 契约
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    // 验证右臂 Y = -0.1（BowAndArrow 分支，headYaw=0）
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f)
        << "isAggressive=true 且持弓时 _applySkeletonArmPose 应设置右臂 BowAndArrow（Y=-0.1）";
}

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithBow_SetsRightArmPitchToMinusPiOverTwo)
{
    equipBow();
    m_entity->setIsAggressive(true);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f) << "BowAndArrow 右臂 X 应为 -PI/2";
}

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithBow_LeftArmPitchAlsoSetByBowAndArrow)
{
    // BowAndArrow 分支在 handleRightArmPose 中同时设置左臂 X = -PI/2
    equipBow();
    m_entity->setIsAggressive(true);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    EXPECT_NEAR(leftArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0), 1e-5f)
        << "BowAndArrow 应通过 handleRightArmPose 同时设置左臂 X=-PI/2";
}

// ============================================================================
// 非拉弓状态 → Empty 姿态测试
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, NotAggressiveWithBow_SetsRightArmEmpty)
{
    // 持弓但未激怒（RangedBowAttackGoal 未启动）→ 不拉弓
    equipBow();
    m_entity->setIsAggressive(false);
    ASSERT_FALSE(m_entity->isAggressive());

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "isAggressive=false 时即使持弓也应设置右臂 Empty（Y=0）";
}

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithoutBow_SetsRightArmEmpty)
{
    // 激怒但未持弓（如近战/换武器）→ 不进入 BowAndArrow
    m_entity->setIsAggressive(true);
    // 不装备弓，主手为空
    ASSERT_EQ(m_entity->getMainHandItem(), nullptr);

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "未持弓时即使 isAggressive=true 也应设置右臂 Empty（Y=0）";
}

TEST_F(ApplySkeletonArmPoseTest, NotAggressiveWithoutBow_SetsRightArmEmpty)
{
    // 既未激怒也未持弓 → Empty
    m_entity->setIsAggressive(false);
    ASSERT_EQ(m_entity->getMainHandItem(), nullptr);

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // Empty 姿态下基类不设置 rightArm.X，保持默认 0
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "Empty 姿态右臂 Y 应为 0";
    EXPECT_NEAR(rightArm->rotateAngleX(), 0.0f, 1e-5f) << "Empty 姿态右臂 X 应保持默认 0";
}

// ============================================================================
// 切换 isAggressive/持弓 状态测试
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, ToggleAggressive_TrueThenFalse_UpdatesArmPose)
{
    equipBow();
    // 第一次：isAggressive=true → BowAndArrow
    m_entity->setIsAggressive(true);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f);

    // 第二次：isAggressive=false → Empty（对齐 RangedBowAttackGoal::resetTask 清 aggressive）
    m_entity->setIsAggressive(false);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "切换到 isAggressive=false 后右臂应回到 Empty（Y=0）";
}

TEST_F(ApplySkeletonArmPoseTest, ToggleAggressive_FalseThenTrue_UpdatesArmPose)
{
    equipBow();
    // 第一次：isAggressive=false → Empty
    m_entity->setIsAggressive(false);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f);

    // 第二次：isAggressive=true → BowAndArrow（对齐 RangedBowAttackGoal::startExecuting 置 aggressive）
    m_entity->setIsAggressive(true);
    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f) << "切换到 isAggressive=true 后右臂应为 BowAndArrow（Y=-0.1）";
}

// ============================================================================
// AnimationContext 参数传递测试
//
// _applySkeletonArmPose 重新调用 setAngles 时传递 context 的参数。
// 验证 context.netHeadYaw 正确传递到 setAngles（影响 BowAndArrow 的右臂 Y）。
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithBow_HeadYawPropagatedToRightArm)
{
    // 头部 yaw=30°：rightArm.Y = -0.1 + 30°(rad)
    equipBow();
    m_entity->setIsAggressive(true);
    m_context->netHeadYaw = 30.0;

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    const f32 headYawRad = static_cast<f32>(m_context->netHeadYaw * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f + headYawRad, 1e-5f)
        << "context.netHeadYaw 应传递到 setAngles，影响 BowAndArrow 右臂 Y";
}

TEST_F(ApplySkeletonArmPoseTest, AggressiveWithBow_HeadPitchPropagatedToRightArm)
{
    // 头部 pitch=20°：rightArm.X = -PI/2 + 20°(rad)
    equipBow();
    m_entity->setIsAggressive(true);
    m_context->headPitch = 20.0;

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    const f32 headPitchRad = static_cast<f32>(m_context->headPitch * PI_DOUBLE / 180.0);
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleX(), static_cast<f32>(-PI_DOUBLE / 2.0) + headPitchRad, 1e-5f)
        << "context.headPitch 应传递到 setAngles，影响 BowAndArrow 右臂 X";
}

TEST_F(ApplySkeletonArmPoseTest, Empty_HeadYawDoesNotAffectRightArm)
{
    // Empty 姿态下右臂 Y 固定为 0，不受 headYaw 影响
    m_entity->setIsAggressive(false);
    m_context->netHeadYaw = 45.0;

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "Empty 姿态右臂 Y 固定为 0，不受 headYaw 影响";
}

// ============================================================================
// scale * 16.0 转换测试
//
// _applySkeletonArmPose 调用 setAngles 时传入 context.scale * 16.0，
// 而非 context.scale 本身。验证此转换正确（context.scale 默认 1/16 → 传入 1.0）。
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, ScaleConversion_SixteenthsToOnePassedToSetAngles)
{
    // context.scale = 1/16 → setAngles 收到 1.0
    // 验证不崩溃且角度正确（BowAndArrow 分支）
    equipBow();
    m_entity->setIsAggressive(true);
    m_context->scale = 1.0 / 16.0; // 默认值

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f) << "context.scale * 16.0 转换应正确（1/16 * 16 = 1.0）";
}

TEST_F(ApplySkeletonArmPoseTest, ScaleConversion_DifferentScaleDoesNotCrash)
{
    // 幼体骷髅 scale 更小，验证不同 scale 不崩溃
    equipBow();
    m_entity->setIsAggressive(true);
    m_context->scale = 0.5 / 16.0; // 幼体缩放

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    // BowAndArrow 角度不受 scale 影响（只影响部件尺寸）
    EXPECT_NEAR(rightArm->rotateAngleY(), -0.1f, 1e-5f);
}

// ============================================================================
// 重复调用幂等性测试
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, RepeatedCall_Idempotent_WhenStateUnchanged)
{
    equipBow();
    m_entity->setIsAggressive(true);

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    auto rightArm1 = m_model->getRightArm();
    ASSERT_NE(rightArm1, nullptr);
    const f32 yawAfterFirst = rightArm1->rotateAngleY();

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);
    auto rightArm2 = m_model->getRightArm();
    ASSERT_NE(rightArm2, nullptr);
    EXPECT_NEAR(rightArm2->rotateAngleY(), yawAfterFirst, 1e-6f) << "状态不变时重复调用 _applySkeletonArmPose 应幂等";
}

// ============================================================================
// 默认 ClientEntity 状态测试
// ============================================================================

TEST_F(ApplySkeletonArmPoseTest, DefaultClientEntity_NotAggressive_SetsEmpty)
{
    // 新创建的 ClientEntity 默认 isAggressive=false、主手无物品
    EXPECT_FALSE(m_entity->isAggressive());
    EXPECT_EQ(m_entity->getMainHandItem(), nullptr);

    applySkeletonArmPoseContract(*m_model, *m_entity, *m_context);

    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    EXPECT_NEAR(rightArm->rotateAngleY(), 0.0f, 1e-5f) << "默认 ClientEntity（非激怒、无物品）应设置右臂 Empty";
}

} // namespace
} // namespace mc::client::renderer
