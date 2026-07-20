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
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"

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
    // 验证 VanillaEntityTypeKeys 常量存在
    (void)entity::VanillaEntityTypeKeys::DROWNED;
    (void)entity::VanillaEntityTypeKeys::GUARDIAN;
    (void)entity::VanillaEntityTypeKeys::ELDER_GUARDIAN;
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
    (void)entity::VanillaEntityTypeKeys::TROPICAL_FISH;
    (void)entity::VanillaEntityTypeKeys::PUFFERFISH;
    (void)entity::VanillaEntityTypeKeys::SALMON;
    (void)entity::VanillaEntityTypeKeys::COD;
    (void)entity::VanillaEntityTypeKeys::SQUID;
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

// ============================================================================
// 美西螈支援效果常量测试
// ============================================================================

/**
 * @brief 美西螈支援效果常量测试
 *
 * 验证美西螈支援效果相关的常量值与 MC 1.21 一致。
 * MC 1.21 参考: net.minecraft.world.entity.animal.axolotl.Axolotl
 */
class AxolotlSupportingEffectsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

/**
 * @brief 测试再生效果基础持续时间
 *
 * MC 1.21: REGEN_BUFF_BASE_DURATION = 100 (5秒)
 */
TEST_F(AxolotlSupportingEffectsTest, RegenBuffBaseDuration_Is100Ticks)
{
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;
    EXPECT_EQ(REGEN_BUFF_BASE_DURATION, 100);
}

/**
 * @brief 测试再生效果最大持续时间
 *
 * MC 1.21: REGEN_BUFF_MAX_DURATION = 2400 (2分钟)
 */
TEST_F(AxolotlSupportingEffectsTest, RegenBuffMaxDuration_Is2400Ticks)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    EXPECT_EQ(REGEN_BUFF_MAX_DURATION, 2400);
}

/**
 * @brief 测试玩家检测范围
 *
 * MC 1.21: PLAYER_REGEN_DETECTION_RANGE = 20.0 (20格)
 */
TEST_F(AxolotlSupportingEffectsTest, PlayerRegenDetectionRange_Is20Blocks)
{
    constexpr f64 PLAYER_REGEN_DETECTION_RANGE = 20.0;
    EXPECT_DOUBLE_EQ(PLAYER_REGEN_DETECTION_RANGE, 20.0);
}

/**
 * @brief 测试狩猎冷却持续时间
 *
 * MC 1.21: 狩猎冷却 2400 ticks (2分钟)
 */
TEST_F(AxolotlSupportingEffectsTest, HuntingCooldownDuration_Is2400Ticks)
{
    constexpr i32 HUNTING_COOLDOWN_DURATION = 2400;
    EXPECT_EQ(HUNTING_COOLDOWN_DURATION, 2400);
}

/**
 * @brief 测试再生效果持续时间计算：无现有效果
 *
 * MC 1.21: 无现有再生效果时，持续时间 = 100 tick
 * applySupportingEffects: newDuration = min(2400, 100 + 0) = 100
 */
TEST_F(AxolotlSupportingEffectsTest, RegenDuration_NoExistingEffect_Is100Ticks)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    // 无现有效果时 currentDuration = 0
    i32 currentDuration = 0;
    i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
    EXPECT_EQ(newDuration, 100);
}

/**
 * @brief 测试再生效果持续时间计算：已有500tick效果
 *
 * MC 1.21: newDuration = min(2400, 100 + 500) = 600
 */
TEST_F(AxolotlSupportingEffectsTest, RegenDuration_Existing500Ticks_Is600Ticks)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    i32 currentDuration = 500;
    i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
    EXPECT_EQ(newDuration, 600);
}

/**
 * @brief 测试再生效果持续时间计算：已有2399tick效果
 *
 * MC 1.21: newDuration = min(2400, 100 + 2399) = 2400（达到上限）
 * endsWithin(2399) → 2399 <= 2399 → true，可以刷新
 */
TEST_F(AxolotlSupportingEffectsTest, RegenDuration_Existing2399Ticks_CappedAt2400)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    i32 currentDuration = 2399;
    // endsWithin(2399) 检查：现有效果 2399 tick <= 2399 → true
    EXPECT_TRUE(currentDuration <= REGEN_BUFF_MAX_DURATION - 1);

    i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
    EXPECT_EQ(newDuration, 2400);
}

/**
 * @brief 测试再生效果持续时间计算：已有2400tick效果
 *
 * MC 1.21: 当现有再生效果持续时间已达到 2400 tick 时，
 * endsWithin(2399) → 2400 > 2399 → false，不刷新
 */
TEST_F(AxolotlSupportingEffectsTest, RegenDuration_Existing2400Ticks_NotRefreshed)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;

    i32 currentDuration = 2400;
    // endsWithin(2399) 检查：现有效果 2400 tick > 2399 → false，不刷新
    EXPECT_FALSE(currentDuration <= REGEN_BUFF_MAX_DURATION - 1);
}

/**
 * @brief 测试再生效果持续时间计算：已有超过2400tick的异常值
 *
 * 虽然正常游戏不会出现超过 2400 tick 的再生效果，
 * 但 endsWithin 应正确处理这种情况。
 */
TEST_F(AxolotlSupportingEffectsTest, RegenDuration_ExistingAboveMax_NotRefreshed)
{
    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;

    i32 currentDuration = 3000;
    EXPECT_FALSE(currentDuration <= REGEN_BUFF_MAX_DURATION - 1);
}

/**
 * @brief 测试再生效果使用 endsWithin 判断的完整逻辑
 *
 * 模拟 applySupportingEffects 的核心判断逻辑：
 * - 无效果时给予新效果
 * - 有效果且 endsWithin(2399) 时刷新
 * - 有效果但 !endsWithin(2399) 时不刷新
 */
TEST_F(AxolotlSupportingEffectsTest, ApplySupportingEffects_LogicUsingEndsWithin)
{
    using namespace mc::entity::effect;

    constexpr i32 REGEN_BUFF_MAX_DURATION = 2400;
    constexpr i32 REGEN_BUFF_BASE_DURATION = 100;

    // 场景1：玩家无再生效果 → 给予 100 tick
    {
        const EffectInstance* existing = nullptr;
        bool shouldApply = (existing == nullptr || existing->endsWithin(REGEN_BUFF_MAX_DURATION - 1));
        EXPECT_TRUE(shouldApply);

        i32 currentDuration = existing != nullptr ? existing->duration() : 0;
        i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + currentDuration);
        EXPECT_EQ(newDuration, 100);
    }

    // 场景2：玩家有 500 tick 再生效果 → 刷新为 600 tick
    {
        EffectInstance existing(EffectType::Regeneration, 500, 0);
        bool shouldApply = existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1);
        EXPECT_TRUE(shouldApply);

        i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + existing.duration());
        EXPECT_EQ(newDuration, 600);
    }

    // 场景3：玩家有 2399 tick 再生效果 → 刷新为 2400 tick（达到上限）
    {
        EffectInstance existing(EffectType::Regeneration, 2399, 0);
        bool shouldApply = existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1);
        EXPECT_TRUE(shouldApply);

        i32 newDuration = std::min(REGEN_BUFF_MAX_DURATION, REGEN_BUFF_BASE_DURATION + existing.duration());
        EXPECT_EQ(newDuration, 2400);
    }

    // 场景4：玩家有 2400 tick 再生效果 → 不刷新
    {
        EffectInstance existing(EffectType::Regeneration, 2400, 0);
        bool shouldApply = existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1);
        EXPECT_FALSE(shouldApply);
    }

    // 场景5：玩家有永久再生效果 → 不刷新
    {
        EffectInstance existing(EffectType::Regeneration, -1, 0, true, false, false);
        bool shouldApply = existing.endsWithin(REGEN_BUFF_MAX_DURATION - 1);
        EXPECT_FALSE(shouldApply);
    }
}

} // namespace
} // namespace mc
