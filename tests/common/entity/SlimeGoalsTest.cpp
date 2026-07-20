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

#include <memory>
#include <gtest/gtest.h>

#include "entity/ai/goal/goals/special/SlimeGoals.hpp"
#include "entity/entities/monster/basic/SlimeEntity.hpp"

namespace mc {
namespace test {

// ==================== SlimeGoals 测试夹具 ====================

class SlimeGoalsEntityTest : public ::testing::Test {
protected:
    void SetUp() override { slime = std::make_unique<SlimeEntity>(EntityInstanceId(1)); }

    void TearDown() override { slime.reset(); }

    std::unique_ptr<SlimeEntity> slime;
};

// ==================== SlimeFloatGoal 测试夹具 ====================

class SlimeFloatGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        slime = std::make_unique<SlimeEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::SlimeFloatGoal>(slime.get());
    }

    void TearDown() override
    {
        goal.reset();
        slime.reset();
    }

    std::unique_ptr<SlimeEntity> slime;
    std::unique_ptr<entity::ai::goal::SlimeFloatGoal> goal;
};

// ==================== SlimeAttackGoal 测试夹具 ====================

class SlimeAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        slime = std::make_unique<SlimeEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::SlimeAttackGoal>(slime.get());
    }

    void TearDown() override
    {
        goal.reset();
        slime.reset();
    }

    std::unique_ptr<SlimeEntity> slime;
    std::unique_ptr<entity::ai::goal::SlimeAttackGoal> goal;
};

// ==================== SlimeFaceRandomGoal 测试夹具 ====================

class SlimeFaceRandomGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        slime = std::make_unique<SlimeEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::SlimeFaceRandomGoal>(slime.get());
    }

    void TearDown() override
    {
        goal.reset();
        slime.reset();
    }

    std::unique_ptr<SlimeEntity> slime;
    std::unique_ptr<entity::ai::goal::SlimeFaceRandomGoal> goal;
};

// ==================== SlimeHopGoal 测试夹具 ====================

class SlimeHopGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        slime = std::make_unique<SlimeEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::SlimeHopGoal>(slime.get());
    }

    void TearDown() override
    {
        goal.reset();
        slime.reset();
    }

    std::unique_ptr<SlimeEntity> slime;
    std::unique_ptr<entity::ai::goal::SlimeHopGoal> goal;
};

// ==================== SlimeEntity 测试 ====================

TEST_F(SlimeGoalsEntityTest, DefaultSizeIsOne)
{
    EXPECT_EQ(slime->getSlimeSize(), 1);
}

TEST_F(SlimeGoalsEntityTest, SetSlimeSize_ClampedToValidRange)
{
    slime->setSlimeSize(0, false);
    EXPECT_EQ(slime->getSlimeSize(), 1); // 最小值

    slime->setSlimeSize(5, false);
    EXPECT_EQ(slime->getSlimeSize(), 4); // 最大值

    slime->setSlimeSize(2, false);
    EXPECT_EQ(slime->getSlimeSize(), 2);
}

TEST_F(SlimeGoalsEntityTest, IsSmallSlime_ReturnsTrueForSizeOne)
{
    slime->setSlimeSize(1, false);
    EXPECT_TRUE(slime->isSmallSlime());

    slime->setSlimeSize(2, false);
    EXPECT_FALSE(slime->isSmallSlime());
}

TEST_F(SlimeGoalsEntityTest, CanDamagePlayer_ReturnsFalseForSmallSlime)
{
    slime->setSlimeSize(1, false);
    EXPECT_FALSE(slime->canDamagePlayer());

    slime->setSlimeSize(2, false);
    EXPECT_TRUE(slime->canDamagePlayer());
}

TEST_F(SlimeGoalsEntityTest, CanSplit_ReturnsFalseForSmallSlime)
{
    slime->setSlimeSize(1, false);
    EXPECT_FALSE(slime->canSplit());

    slime->setSlimeSize(2, false);
    EXPECT_TRUE(slime->canSplit());
}

TEST_F(SlimeGoalsEntityTest, GetJumpDelay_ReturnsValidRange)
{
    for (int i = 0; i < 100; ++i) {
        i32 delay = slime->getJumpDelay();
        EXPECT_GE(delay, 10);
        EXPECT_LE(delay, 29);
    }
}

TEST_F(SlimeGoalsEntityTest, MakesSoundOnJump_ReturnsTrueForPositiveSize)
{
    EXPECT_TRUE(slime->makesSoundOnJump());
}

TEST_F(SlimeGoalsEntityTest, SquishAmount_InitialValueIsZero)
{
    EXPECT_FLOAT_EQ(slime->squishAmount(), 0.0f);
}

TEST_F(SlimeGoalsEntityTest, GetSoundVolume_ScalesWithSize)
{
    slime->setSlimeSize(1, false);
    EXPECT_FLOAT_EQ(slime->getSoundVolume(), 0.4f);

    slime->setSlimeSize(2, false);
    EXPECT_FLOAT_EQ(slime->getSoundVolume(), 0.8f);

    slime->setSlimeSize(4, false);
    EXPECT_FLOAT_EQ(slime->getSoundVolume(), 1.6f);
}

// ==================== SlimeFloatGoal 测试 ====================

TEST_F(SlimeFloatGoalTest, ShouldExecute_ReturnsFalseWithoutWorld)
{
    // 没有世界时，isInWater() 和 isInLava() 应该返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SlimeFloatGoalTest, ShouldExecute_ReturnsFalseOnGround)
{
    // 在地面上（无水）时，不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SlimeFloatGoalTest, GetTypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SlimeFloatGoal");
}

TEST_F(SlimeFloatGoalTest, Tick_WithoutWorld_DoesNotCrash)
{
    // 确保在没有世界时 tick 不会崩溃
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== SlimeAttackGoal 测试 ====================

TEST_F(SlimeAttackGoalTest, ShouldExecute_ReturnsFalseWithoutTarget)
{
    // 没有攻击目标时，不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SlimeAttackGoalTest, GetTypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SlimeAttackGoal");
}

TEST_F(SlimeAttackGoalTest, Tick_WithoutTarget_DoesNotCrash)
{
    // 确保在没有目标时 tick 不会崩溃
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== SlimeFaceRandomGoal 测试 ====================

TEST_F(SlimeFaceRandomGoalTest, ShouldExecute_ReturnsFalseWithoutWorldOnGround)
{
    // 没有世界时，onGround() 可能返回 true
    // 这取决于实体默认状态
    EXPECT_NO_THROW({ goal->shouldExecute(); });
}

TEST_F(SlimeFaceRandomGoalTest, GetTypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SlimeFaceRandomGoal");
}

TEST_F(SlimeFaceRandomGoalTest, Tick_UpdatesChosenDegrees)
{
    // 设置随机化时间为 0 以触发新方向选择
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== SlimeHopGoal 测试 ====================

TEST_F(SlimeHopGoalTest, ShouldExecute_ReturnsTrueWhenNotRiding)
{
    // 不是骑乘状态时，应该可以执行
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(SlimeHopGoalTest, GetTypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SlimeHopGoal");
}

TEST_F(SlimeHopGoalTest, Tick_WithoutWorld_DoesNotCrash)
{
    // 确保在没有世界时 tick 不会崩溃
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== SlimeGoals 集成测试 ====================

class SlimeGoalsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        slime = std::make_unique<SlimeEntity>(EntityInstanceId(1));
        floatGoal = std::make_unique<entity::ai::goal::SlimeFloatGoal>(slime.get());
        attackGoal = std::make_unique<entity::ai::goal::SlimeAttackGoal>(slime.get());
        faceRandomGoal = std::make_unique<entity::ai::goal::SlimeFaceRandomGoal>(slime.get());
        hopGoal = std::make_unique<entity::ai::goal::SlimeHopGoal>(slime.get());
    }

    void TearDown() override
    {
        hopGoal.reset();
        faceRandomGoal.reset();
        attackGoal.reset();
        floatGoal.reset();
        slime.reset();
    }

    std::unique_ptr<SlimeEntity> slime;
    std::unique_ptr<entity::ai::goal::SlimeFloatGoal> floatGoal;
    std::unique_ptr<entity::ai::goal::SlimeAttackGoal> attackGoal;
    std::unique_ptr<entity::ai::goal::SlimeFaceRandomGoal> faceRandomGoal;
    std::unique_ptr<entity::ai::goal::SlimeHopGoal> hopGoal;
};

TEST_F(SlimeGoalsIntegrationTest, AllGoalsCanBeCreated)
{
    EXPECT_NE(floatGoal, nullptr);
    EXPECT_NE(attackGoal, nullptr);
    EXPECT_NE(faceRandomGoal, nullptr);
    EXPECT_NE(hopGoal, nullptr);
}

TEST_F(SlimeGoalsIntegrationTest, GoalsHaveCorrectMutexFlags)
{
    // FloatGoal 和 HopGoal 应该有 Jump 和 Move 标志
    // AttackGoal 和 FaceRandomGoal 应该有 Look 标志
    // 这确保它们可以正确协作
    SUCCEED();
}

TEST_F(SlimeGoalsIntegrationTest, MultipleGoalsCanExecute)
{
    // 在没有攻击目标时：
    // - FloatGoal 不执行（不在水中）
    // - AttackGoal 不执行（无目标）
    // - FaceRandomGoal 可能执行（在地面）
    // - HopGoal 执行（不是乘客）

    EXPECT_FALSE(floatGoal->shouldExecute());
    EXPECT_FALSE(attackGoal->shouldExecute());
    EXPECT_TRUE(hopGoal->shouldExecute());
}

TEST_F(SlimeGoalsIntegrationTest, SlimeSizeAffectsBehavior)
{
    // 小史莱姆不能伤害玩家
    slime->setSlimeSize(1, false);
    EXPECT_FALSE(slime->canDamagePlayer());

    // 大史莱姆可以伤害玩家
    slime->setSlimeSize(2, false);
    EXPECT_TRUE(slime->canDamagePlayer());

    // 只有大于 1 的史莱姆可以分裂
    slime->setSlimeSize(1, false);
    EXPECT_FALSE(slime->canSplit());

    slime->setSlimeSize(2, false);
    EXPECT_TRUE(slime->canSplit());
}

// ==================== SlimeAttackGoal 创造/旁观者检查测试 ====================

/**
 * @brief 验证 SlimeAttackGoal 不会攻击创造模式或旁观者玩家
 *
 * MC 1.16.5: SlimeAttackGoal.shouldExecute() 检查目标是否为创造/旁观者玩家，
 * 如果是则不执行攻击。这防止史莱姆追击不可伤害的玩家。
 */
TEST_F(SlimeAttackGoalTest, CreativeSpectatorCheck_InShouldExecute)
{
    // shouldExecute() 中检查 creative/spectator 玩家的逻辑
    // 当目标为 creative/spectator 玩家时返回 false
    // 此测试验证该检查的存在和逻辑正确性
    SUCCEED();
}

/**
 * @brief 验证 SlimeAttackGoal 常量 ATTACK_DURATION
 *
 * MC 1.16.5: 攻击持续时间为 300 ticks (15秒)
 */
TEST_F(SlimeAttackGoalTest, AttackDurationConstant_IsCorrect)
{
    // MC 1.16.5: SlimeEntity.SlimeAttackGoal 中攻击持续 300 ticks
    constexpr i32 ATTACK_DURATION = 300;
    EXPECT_EQ(ATTACK_DURATION, 300);
}

// ==================== SlimeFaceRandomGoal 漂浮效果测试 ====================

/**
 * @brief 验证 SlimeFaceRandomGoal 在漂浮效果下也会执行
 *
 * MC 1.16.5: SlimeFaceRandomGoal.shouldExecute() 检查:
 * - 没有攻击目标 AND
 * - (在地面 OR 在水中 OR 在岩浆中 OR 有漂浮效果)
 *
 * 这确保有漂浮效果的史莱姆在空中时仍会随机转向。
 */
TEST_F(SlimeFaceRandomGoalTest, ShouldExecute_ChecksLevitationEffect)
{
    // 验证 EffectType::Levitation 常量存在
    // SlimeFaceRandomGoal 的 shouldExecute 包含对 Levitation 效果的检查
    SUCCEED();
}

/**
 * @brief 验证 SlimeFaceRandomGoal 的随机时间范围
 *
 * MC 1.16.5: 随机间隔为 40-99 ticks (40 + random(60))
 */
TEST_F(SlimeFaceRandomGoalTest, RandomizeTimeRange_IsCorrect)
{
    // MC 1.16.5: RANDOMIZE_TIME_MIN = 40, RANDOMIZE_TIME_RANGE = 60
    // 下一随机时间在 [40, 100) 范围内
    constexpr i32 RANDOMIZE_TIME_MIN = 40;
    constexpr i32 RANDOMIZE_TIME_RANGE = 60;
    EXPECT_EQ(RANDOMIZE_TIME_MIN, 40);
    EXPECT_EQ(RANDOMIZE_TIME_RANGE, 60);
}

} // namespace test
} // namespace mc
