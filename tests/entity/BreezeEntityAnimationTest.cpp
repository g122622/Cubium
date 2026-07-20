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

#include "common/entity/ai/goal/goals/special/BreezeGoals.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/entities/monster/breeze/BreezeEntity.hpp"

using namespace mc;
using mc::entity::EntityPose;
using mc::entity::ai::goal::BreezeLongJumpGoal;
using mc::entity::ai::goal::BreezeShootGoal;
using mc::entity::ai::goal::BreezeSlideGoal;

// ============================================================================
// 测试访问器：通过 friend 声明访问 BreezeEntity 的 protected 成员
// ============================================================================
//
// BreezeEntity 被声明为 final，无法通过继承子类暴露 protected 方法。
// 测试中通过 BreezeEntityTestAccessor 这个 friend 类以间接方式访问
// protected 成员，避免修改生产代码的可见性。
// BreezeEntity.hpp 中已声明 `friend class test::BreezeEntityTestAccessor;`。

namespace mc::test {

class BreezeEntityTestAccessor {
public:
    explicit BreezeEntityTestAccessor(BreezeEntity& breeze)
        : m_breeze(breeze)
    {}

    [[nodiscard]] i32 shootCooldown() const { return m_breeze.shootCooldown(); }
    void setShootCooldown(i32 ticks) { m_breeze.setShootCooldown(ticks); }

    [[nodiscard]] i32 jumpCooldown() const { return m_breeze.jumpCooldown(); }
    void setJumpCooldown(i32 ticks) { m_breeze.setJumpCooldown(ticks); }

    [[nodiscard]] bool hasShootPermit() const { return m_breeze.hasShootPermit(); }
    void setShootPermit(i32 ticks) { m_breeze.setShootPermit(ticks); }
    void clearShootPermit() { m_breeze.clearShootPermit(); }

    [[nodiscard]] bool isLongJumping() const { return m_breeze.isLongJumping(); }
    void setLongJumping(bool jumping) { m_breeze.setLongJumping(jumping); }

    [[nodiscard]] bool isSliding() const { return m_breeze.isSliding(); }
    void setSliding(bool sliding) { m_breeze.setSliding(sliding); }

private:
    BreezeEntity& m_breeze;
};

} // namespace mc::test

// ============================================================================
// BreezeEntity 动画状态机测试
// ============================================================================
//
// 本测试集聚焦于旋风人动画状态机的 Pose 转换逻辑，验证：
// - 默认 Pose 为 Standing
// - setPose 直接调用能正确切换姿态
// - BreezeShootGoal::resetTask 守卫逻辑（仅在 Shooting 时恢复 Standing）
// - BreezeLongJumpGoal::resetTask 守卫逻辑（仅在 LongJumping/Inhaling 时恢复 Standing）
// - BreezeSlideGoal::resetTask 恢复 Standing
// - AnimationState 字段的访问器
// - 静态常量值与 MC 1.21.11 一致
// - canAttackType 白名单逻辑
// - 多 Pose 顺序切换时 resetTask 守卫不互相干扰

class BreezeEntityTest : public ::testing::Test {
protected:
    BreezeEntity m_breeze{EntityInstanceId(1)};
    test::BreezeEntityTestAccessor m_accessor{m_breeze};

    void SetUp() override
    {
        // 默认 Pose 应为 Standing
        ASSERT_EQ(m_breeze.pose(), EntityPose::Standing);
    }
};

// ==================== 默认状态测试 ====================

TEST_F(BreezeEntityTest, DefaultPose_IsStanding)
{
    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, DefaultShootCooldown_IsZero)
{
    EXPECT_EQ(m_accessor.shootCooldown(), 0);
}

TEST_F(BreezeEntityTest, DefaultJumpCooldown_IsZero)
{
    EXPECT_EQ(m_accessor.jumpCooldown(), 0);
}

TEST_F(BreezeEntityTest, DefaultHasNoShootPermit)
{
    EXPECT_FALSE(m_accessor.hasShootPermit());
}

TEST_F(BreezeEntityTest, DefaultIsNotLongJumping)
{
    EXPECT_FALSE(m_accessor.isLongJumping());
}

TEST_F(BreezeEntityTest, DefaultIsNotSliding)
{
    EXPECT_FALSE(m_accessor.isSliding());
}

// ==================== Pose 转换测试 ====================

TEST_F(BreezeEntityTest, SetPose_ToShooting)
{
    m_breeze.setPose(EntityPose::Shooting);
    EXPECT_EQ(m_breeze.pose(), EntityPose::Shooting);
}

TEST_F(BreezeEntityTest, SetPose_ToSliding)
{
    m_breeze.setPose(EntityPose::Sliding);
    EXPECT_EQ(m_breeze.pose(), EntityPose::Sliding);
}

TEST_F(BreezeEntityTest, SetPose_ToInhaling)
{
    m_breeze.setPose(EntityPose::Inhaling);
    EXPECT_EQ(m_breeze.pose(), EntityPose::Inhaling);
}

TEST_F(BreezeEntityTest, SetPose_ToLongJumping)
{
    m_breeze.setPose(EntityPose::LongJumping);
    EXPECT_EQ(m_breeze.pose(), EntityPose::LongJumping);
}

TEST_F(BreezeEntityTest, SetPose_BackToStanding)
{
    m_breeze.setPose(EntityPose::Shooting);
    m_breeze.setPose(EntityPose::Standing);
    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

// ==================== BreezeShootGoal resetTask 守卫测试 ====================

TEST_F(BreezeEntityTest, ShootGoalResetTask_WhenShooting_ResetsToStanding)
{
    // 模拟 Shoot.startExecuting 设置 Shooting 姿态
    m_breeze.setPose(EntityPose::Shooting);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Shooting);

    // 构造 Shoot Goal 并调用 resetTask
    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    // 应恢复 Standing
    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_WhenSliding_DoesNotChangePose)
{
    // 当前 Pose 为 Sliding（其他 Goal 设置的），不应被 ShootGoal.resetTask 覆盖
    m_breeze.setPose(EntityPose::Sliding);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Sliding);

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    // Pose 应仍为 Sliding（守卫逻辑生效）
    EXPECT_EQ(m_breeze.pose(), EntityPose::Sliding);
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_WhenStanding_DoesNotChangePose)
{
    // 当前已为 Standing，resetTask 守卫检查 Pose != Shooting，不切换
    ASSERT_EQ(m_breeze.pose(), EntityPose::Standing);

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_WhenLongJumping_DoesNotChangePose)
{
    // 当前为 LongJumping（其他 Goal 设置的），不应被 ShootGoal.resetTask 覆盖
    m_breeze.setPose(EntityPose::LongJumping);
    ASSERT_EQ(m_breeze.pose(), EntityPose::LongJumping);

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::LongJumping);
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_WhenInhaling_DoesNotChangePose)
{
    // 当前为 Inhaling（LongJumpGoal 设置的），不应被 ShootGoal.resetTask 覆盖
    m_breeze.setPose(EntityPose::Inhaling);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Inhaling);

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Inhaling);
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_ClearsShootPermit)
{
    m_accessor.setShootPermit(100);
    ASSERT_TRUE(m_accessor.hasShootPermit());

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    EXPECT_FALSE(m_accessor.hasShootPermit());
}

TEST_F(BreezeEntityTest, ShootGoalResetTask_SetsShootCooldown)
{
    ASSERT_EQ(m_accessor.shootCooldown(), 0);

    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();

    // MC 1.21.11: SHOOT_COOLDOWN_TICKS = 10
    EXPECT_EQ(m_accessor.shootCooldown(), 10);
}

// ==================== BreezeLongJumpGoal resetTask 守卫测试 ====================

TEST_F(BreezeEntityTest, LongJumpGoalResetTask_WhenLongJumping_ResetsToStanding)
{
    m_breeze.setPose(EntityPose::LongJumping);
    ASSERT_EQ(m_breeze.pose(), EntityPose::LongJumping);

    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, LongJumpGoalResetTask_WhenInhaling_ResetsToStanding)
{
    m_breeze.setPose(EntityPose::Inhaling);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Inhaling);

    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, LongJumpGoalResetTask_WhenSliding_DoesNotChangePose)
{
    // Sliding 由 SlideGoal 拥有，LongJumpGoal.resetTask 不应覆盖
    m_breeze.setPose(EntityPose::Sliding);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Sliding);

    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Sliding);
}

TEST_F(BreezeEntityTest, LongJumpGoalResetTask_WhenShooting_DoesNotChangePose)
{
    // Shooting 由 ShootGoal 拥有，LongJumpGoal.resetTask 不应覆盖
    m_breeze.setPose(EntityPose::Shooting);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Shooting);

    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Shooting);
}

TEST_F(BreezeEntityTest, LongJumpGoalResetTask_WhenStanding_DoesNotChangePose)
{
    ASSERT_EQ(m_breeze.pose(), EntityPose::Standing);

    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

// ==================== BreezeSlideGoal resetTask 测试 ====================

TEST_F(BreezeEntityTest, SlideGoalResetTask_WhenSliding_ResetsToStanding)
{
    m_breeze.setPose(EntityPose::Sliding);
    m_accessor.setSliding(true);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Sliding);
    ASSERT_TRUE(m_accessor.isSliding());

    BreezeSlideGoal slideGoal(&m_breeze);
    slideGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
    EXPECT_FALSE(m_accessor.isSliding());
}

TEST_F(BreezeEntityTest, SlideGoalResetTask_WhenStanding_RemainsStanding)
{
    ASSERT_EQ(m_breeze.pose(), EntityPose::Standing);

    BreezeSlideGoal slideGoal(&m_breeze);
    slideGoal.resetTask();

    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

// ==================== AnimationState 访问器测试 ====================

TEST_F(BreezeEntityTest, IdleAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.idleAnimation().isStarted());
}

TEST_F(BreezeEntityTest, SlideAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.slideAnimation().isStarted());
}

TEST_F(BreezeEntityTest, SlideBackAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.slideBackAnimation().isStarted());
}

TEST_F(BreezeEntityTest, LongJumpAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.longJumpAnimation().isStarted());
}

TEST_F(BreezeEntityTest, ShootAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.shootAnimation().isStarted());
}

TEST_F(BreezeEntityTest, InhaleAnimation_DefaultNotStarted)
{
    EXPECT_FALSE(m_breeze.inhaleAnimation().isStarted());
}

// ==================== 静态常量测试 ====================

TEST_F(BreezeEntityTest, MaxHealthConstant_Is30)
{
    // MC 1.21.11: Breeze.MAX_HEALTH = 30
    EXPECT_FLOAT_EQ(BreezeEntity::MAX_HEALTH, 30.0f);
}

TEST_F(BreezeEntityTest, MovementSpeedConstant_Is06)
{
    // MC 1.21.11: Breeze.MOVEMENT_SPEED = 0.6
    EXPECT_FLOAT_EQ(BreezeEntity::MOVEMENT_SPEED, 0.6f);
}

TEST_F(BreezeEntityTest, FollowRangeConstant_Is24)
{
    // MC 1.21.11: Breeze.FOLLOW_RANGE = 24
    EXPECT_FLOAT_EQ(BreezeEntity::FOLLOW_RANGE, 24.0f);
}

TEST_F(BreezeEntityTest, AttackDamageConstant_Is3)
{
    // MC 1.21.11: Breeze.ATTACK_DAMAGE = 3
    EXPECT_FLOAT_EQ(BreezeEntity::ATTACK_DAMAGE, 3.0f);
}

// ==================== 射击许可与冷却测试 ====================

TEST_F(BreezeEntityTest, SetShootPermit_SetsPermit)
{
    m_accessor.setShootPermit(50);
    EXPECT_TRUE(m_accessor.hasShootPermit());
}

TEST_F(BreezeEntityTest, ClearShootPermit_ClearsPermit)
{
    m_accessor.setShootPermit(50);
    m_accessor.clearShootPermit();
    EXPECT_FALSE(m_accessor.hasShootPermit());
}

TEST_F(BreezeEntityTest, SetShootCooldown_SetsCooldown)
{
    m_accessor.setShootCooldown(20);
    EXPECT_EQ(m_accessor.shootCooldown(), 20);
}

TEST_F(BreezeEntityTest, SetJumpCooldown_SetsCooldown)
{
    m_accessor.setJumpCooldown(15);
    EXPECT_EQ(m_accessor.jumpCooldown(), 15);
}

TEST_F(BreezeEntityTest, SetSliding_SetsSlidingState)
{
    m_accessor.setSliding(true);
    EXPECT_TRUE(m_accessor.isSliding());

    m_accessor.setSliding(false);
    EXPECT_FALSE(m_accessor.isSliding());
}

TEST_F(BreezeEntityTest, SetLongJumping_SetsLongJumpingState)
{
    m_accessor.setLongJumping(true);
    EXPECT_TRUE(m_accessor.isLongJumping());

    m_accessor.setLongJumping(false);
    EXPECT_FALSE(m_accessor.isLongJumping());
}

// ==================== 实体尺寸测试 ====================

TEST_F(BreezeEntityTest, Width_Is06)
{
    // MC 1.21.11: Breeze 宽度 0.6
    EXPECT_FLOAT_EQ(m_breeze.width(), 0.6f);
}

TEST_F(BreezeEntityTest, Height_Is177)
{
    // MC 1.21.11: Breeze 高度 1.77
    EXPECT_FLOAT_EQ(m_breeze.height(), 1.77f);
}

TEST_F(BreezeEntityTest, EyeHeight_Is152)
{
    // MC 1.21.11: Breeze 眼睛高度 1.52
    EXPECT_FLOAT_EQ(m_breeze.eyeHeight(), 1.52f);
}

// ==================== canAttackType 白名单测试 ====================
//
// 注意：canAttackType 的完整测试在 tests/entity/CanAttackTypeTest.cpp 中，
// 该测试套件通过 SetUpTestSuite 调用 VanillaEntities::registerAll() 初始化
// 实体类型 ID。本测试集不初始化注册表（VanillaEntityTypeKeys::* 默认为 0），
// 因此 canAttackType 的白名单比较在此会失效（所有类型 ID 都为 0），
// 完整的 canAttackType 行为验证由 CanAttackTypeTest 负责。
//
// 如需查阅 Breeze canAttackType 的测试用例，参见：
// - CanAttackTypeTest.Breeze_CanAttackPlayer
// - CanAttackTypeTest.Breeze_CanAttackIronGolem
// - CanAttackTypeTest.Breeze_CannotAttackOtherTypes

// ==================== 多 Pose 顺序切换不互相干扰测试 ====================

TEST_F(BreezeEntityTest, PoseTransitions_ShootThenSlide_IndependentResets)
{
    // 验证 Pose 切换顺序：Shooting → Sliding → Standing
    // ShootGoal.resetTask 在 Sliding Pose 下不应切换回 Standing
    m_breeze.setPose(EntityPose::Shooting);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Shooting);

    // 模拟 SlideGoal.startExecuting 切换到 Sliding
    m_breeze.setPose(EntityPose::Sliding);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Sliding);

    // 此时 ShootGoal.resetTask 被调用（射击目标结束），不应覆盖 Sliding
    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();
    EXPECT_EQ(m_breeze.pose(), EntityPose::Sliding);

    // SlideGoal.resetTask 被调用（滑行结束），应恢复 Standing
    BreezeSlideGoal slideGoal(&m_breeze);
    slideGoal.resetTask();
    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}

TEST_F(BreezeEntityTest, PoseTransitions_LongJumpThenShoot_IndependentResets)
{
    // 验证 Pose 切换顺序：Inhaling → LongJumping → Shooting
    // LongJumpGoal.resetTask 在 Shooting Pose 下不应切换回 Standing
    m_breeze.setPose(EntityPose::Inhaling);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Inhaling);

    m_breeze.setPose(EntityPose::LongJumping);
    ASSERT_EQ(m_breeze.pose(), EntityPose::LongJumping);

    // 着陆后 ShootGoal 启动，切换到 Shooting
    m_breeze.setPose(EntityPose::Shooting);
    ASSERT_EQ(m_breeze.pose(), EntityPose::Shooting);

    // 此时 LongJumpGoal.resetTask 被调用，不应覆盖 Shooting
    BreezeLongJumpGoal longJumpGoal(&m_breeze);
    longJumpGoal.resetTask();
    EXPECT_EQ(m_breeze.pose(), EntityPose::Shooting);

    // ShootGoal.resetTask 被调用，应恢复 Standing
    BreezeShootGoal shootGoal(&m_breeze);
    shootGoal.resetTask();
    EXPECT_EQ(m_breeze.pose(), EntityPose::Standing);
}
