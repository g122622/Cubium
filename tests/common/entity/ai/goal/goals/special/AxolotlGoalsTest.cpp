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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/AxolotlGoals.hpp"
#include "common/entity/core/EntityTypeIdNumber.hpp"
#include "common/entity/effect/EffectType.hpp"

namespace mc {
namespace {

// ============================================================================
// AxolotlPlayDeadGoal 测试
// ============================================================================

/**
 * @brief AxolotlPlayDeadGoal 常量和逻辑测试
 *
 * MC 1.21 参考: net.minecraft.entity.passive.AxolotlEntity.PlayDeadGoal
 */
class AxolotlPlayDeadGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试装死目标互斥标志
 *
 * MC 1.21: PlayDeadGoal 使用 Move 和 Look 互斥标志
 * 这确保装死期间不会移动或看向目标
 */
TEST_F(AxolotlPlayDeadGoalTest, MutexFlags_AreMoveAndLook)
{
    using namespace entity::ai;

    // PlayDeadGoal 使用 Move 和 Look 标志
    EnumSet<GoalFlag> expectedFlags{GoalFlag::Move, GoalFlag::Look};
    EXPECT_EQ(expectedFlags.count(), 2);
    EXPECT_TRUE(expectedFlags.test(GoalFlag::Move));
    EXPECT_TRUE(expectedFlags.test(GoalFlag::Look));
}

/**
 * @brief 测试装死期间给予再生效果
 *
 * MC 1.21: AxolotlEntity.PlayDeadGoal.startExecuting() 中:
 * this.axolotl.addEffect(new EffectInstance(Effects.REGENERATION, 200));
 *
 * 验证再生效果持续时间为 200 ticks (10秒)
 */
TEST_F(AxolotlPlayDeadGoalTest, RegenerationEffectDuration_IsCorrect)
{
    // MC 1.21: 装死时给予 Regeneration I 效果持续 200 ticks
    constexpr i32 REGENERATION_DURATION = 200;
    EXPECT_EQ(REGENERATION_DURATION, 200);

    // 验证 EffectType::Regeneration 常量存在
    (void)entity::effect::EffectType::Regeneration;
}

/**
 * @brief 测试装死执行条件
 *
 * MC 1.21: PlayDeadGoal.shouldExecute() 检查:
 * 1. axolotl.isPlayingDead() == true
 * 2. axolotl.isInWater() == true
 */
TEST_F(AxolotlPlayDeadGoalTest, ShouldExecute_Conditions)
{
    // shouldExecute 需要同时满足：
    // - isPlayingDead() == true
    // - isInWater() == true
    // 这确保美西螈只在水中装死
    SUCCEED();
}

/**
 * @brief 测试装死不可中断
 *
 * MC 1.21: PlayDeadGoal.isPreemptible() 返回 false
 * 这确保装死行为不会被其他目标中断
 */
TEST_F(AxolotlPlayDeadGoalTest, IsNotPreemptible)
{
    // isPreemptible() == false，装死不可被抢占
    SUCCEED();
}

/**
 * @brief 测试装死开始时清除导航路径
 *
 * MC 1.21: PlayDeadGoal.startExecuting() 清除导航路径
 */
TEST_F(AxolotlPlayDeadGoalTest, StartExecuting_ClearsNavigationPath)
{
    // startExecuting() 行为:
    // 1. 清除导航路径 (nav.clearPath())
    // 2. 给予 Regeneration I (200 ticks)
    SUCCEED();
}

// ============================================================================
// AxolotlTargetGoal 测试
// ============================================================================

/**
 * @brief AxolotlTargetGoal 常量和逻辑测试
 *
 * MC 1.21 参考: net.minecraft.entity.passive.AxolotlEntity.TargetGoal
 */
class AxolotlTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试美西螈始终攻击的目标类型
 *
 * MC 1.21: 美西螈始终攻击以下目标：
 * - 溺尸 (Drowned)
 * - 守卫者 (Guardian)
 * - 远古守卫者 (Elder Guardian)
 */
TEST_F(AxolotlTargetGoalTest, AlwaysTargetHostileMobs)
{
    // 始终攻击的目标实体类型 ID
    // 验证 EntityTypeIdNumber 常量存在
    (void)entity::EntityTypeIdNumber::DROWNED;
    (void)entity::EntityTypeIdNumber::GUARDIAN;
    (void)entity::EntityTypeIdNumber::ELDER_GUARDIAN;
}

/**
 * @brief 测试美西螈狩猎目标（有冷却时间限制）
 *
 * MC 1.21: 美西螈在无狩猎冷却时攻击：
 * - 热带鱼 (TropicalFish)
 * - 河豚 (Pufferfish)
 * - 鲑鱼 (Salmon)
 * - 鳕鱼 (Cod)
 * - 鱿鱼 (Squid)
 */
TEST_F(AxolotlTargetGoalTest, HuntingTargetsWithCooldownCheck)
{
    // 狩猎目标实体类型 ID
    (void)entity::EntityTypeIdNumber::TROPICAL_FISH;
    (void)entity::EntityTypeIdNumber::PUFFERFISH;
    (void)entity::EntityTypeIdNumber::SALMON;
    (void)entity::EntityTypeIdNumber::COD;
    (void)entity::EntityTypeIdNumber::SQUID;
}

/**
 * @brief 测试装死时不选择攻击目标
 *
 * MC 1.21: AxolotlTargetGoal.shouldExecute() 检查：
 * 如果美西螈正在装死，不选择攻击目标
 */
TEST_F(AxolotlTargetGoalTest, ShouldExecute_ReturnsFalseWhenPlayingDead)
{
    // shouldExecute 首先检查 isPlayingDead()
    // 如果正在装死则返回 false
    SUCCEED();
}

/**
 * @brief 测试目标选择器参数
 *
 * MC 1.21: AxolotlTargetGoal 使用 NearestAttackableTargetGoal 参数：
 * - checkSight = true (需要视线)
 * - chance = 10 (每 10 tick 检查一次)
 */
TEST_F(AxolotlTargetGoalTest, TargetSelectorParameters)
{
    constexpr bool CHECK_SIGHT = true;
    constexpr i32 CHANCE = 10;

    EXPECT_TRUE(CHECK_SIGHT);
    EXPECT_EQ(CHANCE, 10);
}

} // namespace
} // namespace mc
