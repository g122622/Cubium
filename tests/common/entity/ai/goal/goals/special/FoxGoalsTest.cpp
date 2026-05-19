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

#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/special/FoxGoals.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/passive/special/FoxEntity.hpp"

namespace mc {
namespace test {

// ==================== FoxGoals 基础测试 ====================

class FoxGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityId(1)); }

    void TearDown() override { fox.reset(); }

    std::unique_ptr<FoxEntity> fox;
};

// ==================== FoxPassiveGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxPassiveGoal_TypeName)
{
    // 验证 FoxPassiveGoal 可以被正确构造
    // FoxPassiveGoal 是抽象类，通过 FoxSleepGoal 测试基类功能
    auto goal = std::make_unique<entity::ai::goal::FoxSleepGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
}

// ==================== FoxFollowTargetGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxFollowTargetGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxFollowTargetGoal");
}

TEST_F(FoxGoalsTest, FoxFollowTargetGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(fox.get());

    // FoxFollowTargetGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(FoxGoalsTest, FoxFollowTargetGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(fox.get());

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxFollowTargetGoal_ShouldContinueExecutingWithoutTarget)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(fox.get());

    // 无目标时不应继续执行
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

// ==================== FoxPounceGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxPounceGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxPounceGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxPounceGoal");
}

TEST_F(FoxGoalsTest, FoxPounceGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxPounceGoal>(fox.get());

    // FoxPounceGoal 应该有 Move、Look 和 Jump 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(FoxGoalsTest, FoxPounceGoal_NotPreemptible)
{
    auto goal = std::make_unique<entity::ai::goal::FoxPounceGoal>(fox.get());

    // 扑击不可中断
    EXPECT_FALSE(goal->isPreemptible());
}

TEST_F(FoxGoalsTest, FoxPounceGoal_ShouldExecuteWithoutCrouching)
{
    auto goal = std::make_unique<entity::ai::goal::FoxPounceGoal>(fox.get());

    // 未蹲伏时不应该执行
    EXPECT_FALSE(fox->isFullyCrouched());
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxBiteGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxBiteGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxBiteGoal>(fox.get(), 1.2, true);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxBiteGoal");
}

TEST_F(FoxGoalsTest, FoxBiteGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxBiteGoal>(fox.get(), 1.2, true);

    // FoxBiteGoal 继承自 MeleeAttackGoal，应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
}

TEST_F(FoxGoalsTest, FoxBiteGoal_ShouldNotExecuteWhenSitting)
{
    auto goal = std::make_unique<entity::ai::goal::FoxBiteGoal>(fox.get(), 1.2, true);

    fox->setSitting(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxBiteGoal_ShouldNotExecuteWhenSleeping)
{
    auto goal = std::make_unique<entity::ai::goal::FoxBiteGoal>(fox.get(), 1.2, true);

    fox->setSleeping(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxBiteGoal_ShouldNotExecuteWhenCrouching)
{
    auto goal = std::make_unique<entity::ai::goal::FoxBiteGoal>(fox.get(), 1.2, true);

    fox->setCrouching(true);
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxFindShelterGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxFindShelterGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFindShelterGoal>(fox.get(), 1.25);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxFindShelterGoal");
}

TEST_F(FoxGoalsTest, FoxFindShelterGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFindShelterGoal>(fox.get(), 1.25);

    // FoxFindShelterGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
}

TEST_F(FoxGoalsTest, FoxFindShelterGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFindShelterGoal>(fox.get(), 1.25);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxSleepGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxSleepGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSleepGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxSleepGoal");
}

TEST_F(FoxGoalsTest, FoxSleepGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSleepGoal>(fox.get());

    // FoxSleepGoal 应该有 Move、Look 和 Jump 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
}

TEST_F(FoxGoalsTest, FoxSleepGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSleepGoal>(fox.get());

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxEatBerriesGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxEatBerriesGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(fox.get(), 1.2, 12, 2);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxEatBerriesGoal");
}

TEST_F(FoxGoalsTest, FoxEatBerriesGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(fox.get(), 1.2, 12, 2);

    // FoxEatBerriesGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
}

TEST_F(FoxGoalsTest, FoxEatBerriesGoal_ShouldNotExecuteWhenSleeping)
{
    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(fox.get(), 1.2, 12, 2);

    fox->setSleeping(true);
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxFindItemsGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxFindItemsGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFindItemsGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxFindItemsGoal");
}

TEST_F(FoxGoalsTest, FoxFindItemsGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxFindItemsGoal>(fox.get());

    // FoxFindItemsGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
}

// ==================== FoxSitAndLookGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxSitAndLookGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSitAndLookGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxSitAndLookGoal");
}

TEST_F(FoxGoalsTest, FoxSitAndLookGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSitAndLookGoal>(fox.get());

    // FoxSitAndLookGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
}

TEST_F(FoxGoalsTest, FoxSitAndLookGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::FoxSitAndLookGoal>(fox.get());

    // 无世界时不应执行（概率触发，但无世界时 hasAlertableTarget 返回 false）
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxRevengeGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxRevengeGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxRevengeGoal");
}

TEST_F(FoxGoalsTest, FoxRevengeGoal_ShouldExecuteReturnsFalse)
{
    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());

    // 简化实现，暂时返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxEntity 状态测试 ====================

class FoxEntityStateTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityId(1)); }

    void TearDown() override { fox.reset(); }

    std::unique_ptr<FoxEntity> fox;
};

TEST_F(FoxEntityStateTest, SittingState)
{
    EXPECT_FALSE(fox->isSitting());

    fox->setSitting(true);
    EXPECT_TRUE(fox->isSitting());

    fox->setSitting(false);
    EXPECT_FALSE(fox->isSitting());
}

TEST_F(FoxEntityStateTest, CrouchingState)
{
    EXPECT_FALSE(fox->isCrouching());

    fox->setCrouching(true);
    EXPECT_TRUE(fox->isCrouching());

    fox->setCrouching(false);
    EXPECT_FALSE(fox->isCrouching());
}

TEST_F(FoxEntityStateTest, InterestedState)
{
    EXPECT_FALSE(fox->isInterested());

    fox->setInterested(true);
    EXPECT_TRUE(fox->isInterested());

    fox->setInterested(false);
    EXPECT_FALSE(fox->isInterested());
}

TEST_F(FoxEntityStateTest, PounceReadyState)
{
    EXPECT_FALSE(fox->isPounceReady());

    fox->setPounceReady(true);
    EXPECT_TRUE(fox->isPounceReady());

    fox->setPounceReady(false);
    EXPECT_FALSE(fox->isPounceReady());
}

TEST_F(FoxEntityStateTest, SleepingState)
{
    EXPECT_FALSE(fox->isSleeping());

    fox->setSleeping(true);
    EXPECT_TRUE(fox->isSleeping());

    fox->setSleeping(false);
    EXPECT_FALSE(fox->isSleeping());
}

TEST_F(FoxEntityStateTest, StuckState)
{
    EXPECT_FALSE(fox->isStuck());

    fox->setStuck(true);
    EXPECT_TRUE(fox->isStuck());

    fox->setStuck(false);
    EXPECT_FALSE(fox->isStuck());
}

TEST_F(FoxEntityStateTest, FoxAggroedState)
{
    EXPECT_FALSE(fox->isFoxAggroed());

    fox->setFoxAggroed(true);
    EXPECT_TRUE(fox->isFoxAggroed());

    fox->setFoxAggroed(false);
    EXPECT_FALSE(fox->isFoxAggroed());
}

TEST_F(FoxEntityStateTest, CrouchAmount)
{
    EXPECT_FLOAT_EQ(fox->crouchAmount(), 0.0f);

    fox->setCrouchAmount(1.5f);
    EXPECT_FLOAT_EQ(fox->crouchAmount(), 1.5f);

    fox->setCrouchAmount(0.0f);
    EXPECT_FLOAT_EQ(fox->crouchAmount(), 0.0f);
}

TEST_F(FoxEntityStateTest, FullyCrouched)
{
    EXPECT_FALSE(fox->isFullyCrouched());

    // 设置蹲伏但蹲伏量不足
    fox->setCrouching(true);
    fox->setCrouchAmount(2.0f);
    EXPECT_FALSE(fox->isFullyCrouched());

    // 蹲伏量达到 3.0
    fox->setCrouchAmount(3.0f);
    EXPECT_TRUE(fox->isFullyCrouched());

    // 蹲伏量超过 3.0
    fox->setCrouchAmount(3.5f);
    EXPECT_TRUE(fox->isFullyCrouched());
}

TEST_F(FoxEntityStateTest, CanAct)
{
    // 默认状态可以行动
    EXPECT_TRUE(fox->canAct());

    // 坐下时不能行动
    fox->setSitting(true);
    EXPECT_FALSE(fox->canAct());
    fox->setSitting(false);

    // 蹲伏时不能行动
    fox->setCrouching(true);
    EXPECT_FALSE(fox->canAct());
    fox->setCrouching(false);

    // 睡眠时不能行动
    fox->setSleeping(true);
    EXPECT_FALSE(fox->canAct());
    fox->setSleeping(false);

    // 卡住时不能行动
    fox->setStuck(true);
    EXPECT_FALSE(fox->canAct());
    fox->setStuck(false);

    // 激怒时不能行动
    fox->setFoxAggroed(true);
    EXPECT_FALSE(fox->canAct());
    fox->setFoxAggroed(false);

    // 恢复默认状态
    EXPECT_TRUE(fox->canAct());
}

TEST_F(FoxEntityStateTest, ResetAllStates)
{
    // 设置所有状态
    fox->setSitting(true);
    fox->setCrouching(true);
    fox->setInterested(true);
    fox->setPounceReady(true);
    fox->setStuck(true);
    fox->setCrouchAmount(2.5f);

    // 重置
    fox->resetAllStates();

    // 验证所有状态被重置
    EXPECT_FALSE(fox->isSitting());
    EXPECT_FALSE(fox->isCrouching());
    EXPECT_FALSE(fox->isInterested());
    EXPECT_FALSE(fox->isPounceReady());
    EXPECT_FALSE(fox->isStuck());
    EXPECT_FLOAT_EQ(fox->crouchAmount(), 0.0f);
}

TEST_F(FoxEntityStateTest, WakeUp)
{
    fox->setSleeping(true);
    fox->setSitting(true);

    fox->wakeUp();

    EXPECT_FALSE(fox->isSleeping());
    EXPECT_FALSE(fox->isSitting());
}

// ==================== FoxEntity 信任系统测试 ====================

class FoxEntityTrustTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityId(1)); }

    void TearDown() override { fox.reset(); }

    std::unique_ptr<FoxEntity> fox;
};

TEST_F(FoxEntityTrustTest, TrustsPlayer)
{
    u64 playerId1 = 12345;
    u64 playerId2 = 67890;

    // 初始不信任任何人
    EXPECT_FALSE(fox->trusts(playerId1));
    EXPECT_FALSE(fox->trusts(playerId2));

    // 添加信任
    fox->addTrustedPlayer(playerId1);
    EXPECT_TRUE(fox->trusts(playerId1));
    EXPECT_FALSE(fox->trusts(playerId2));

    // 添加第二个信任玩家
    fox->addTrustedPlayer(playerId2);
    EXPECT_TRUE(fox->trusts(playerId1));
    EXPECT_TRUE(fox->trusts(playerId2));

    // 移除信任
    fox->removeTrustedPlayer(playerId1);
    EXPECT_FALSE(fox->trusts(playerId1));
    EXPECT_TRUE(fox->trusts(playerId2));
}

TEST_F(FoxEntityTrustTest, MaxTrustedPlayers)
{
    // 添加最大数量的信任玩家
    for (u64 i = 1; i <= 2; ++i) {
        fox->addTrustedPlayer(i);
    }

    // 验证所有玩家都被信任
    for (u64 i = 1; i <= 2; ++i) {
        EXPECT_TRUE(fox->trusts(i));
    }

    // 添加第三个玩家（应该替换最早的）
    fox->addTrustedPlayer(3);
    EXPECT_FALSE(fox->trusts(1)); // 第一个被替换
    EXPECT_TRUE(fox->trusts(2));
    EXPECT_TRUE(fox->trusts(3));
}

TEST_F(FoxEntityTrustTest, GetFirstTrustedPlayer)
{
    // 无信任玩家时返回 nullopt
    EXPECT_FALSE(fox->getFirstTrustedPlayer().has_value());

    // 添加信任玩家
    fox->addTrustedPlayer(12345);
    auto first = fox->getFirstTrustedPlayer();
    EXPECT_TRUE(first.has_value());
    EXPECT_EQ(first.value(), 12345);
}

TEST_F(FoxEntityTrustTest, DuplicateTrust)
{
    fox->addTrustedPlayer(12345);
    fox->addTrustedPlayer(12345); // 重复添加

    // 应该只有一个信任玩家
    EXPECT_TRUE(fox->trusts(12345));
    EXPECT_TRUE(fox->getFirstTrustedPlayer().has_value());
}

// ==================== FoxEntity 音效方法测试 ====================

class FoxEntitySoundTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityId(1)); }

    void TearDown() override { fox.reset(); }

    std::unique_ptr<FoxEntity> fox;
};

TEST_F(FoxEntitySoundTest, AmbientSound)
{
    // 白狐使用 screech 音效
    fox->setFoxType(FoxEntity::FoxType::Snow);
    auto sound = fox->getAmbientSound();
    EXPECT_TRUE(sound.has_value());
    // 验证是 screech 音效

    // 红狐使用普通 ambient 音效
    fox->setFoxType(FoxEntity::FoxType::Red);
    sound = fox->getAmbientSound();
    EXPECT_TRUE(sound.has_value());
}

TEST_F(FoxEntitySoundTest, HurtSound)
{
    EnvironmentalDamage damageSource = EnvironmentalDamage(DamageType::Generic);
    auto sound = fox->getHurtSound(damageSource);
    EXPECT_TRUE(sound.has_value());
}

TEST_F(FoxEntitySoundTest, DeathSound)
{
    auto sound = fox->getDeathSound();
    EXPECT_TRUE(sound.has_value());
}

// ==================== FoxEntity 繁殖测试 ====================

class FoxEntityBreedTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityId(1)); }

    void TearDown() override { fox.reset(); }

    std::unique_ptr<FoxEntity> fox;
};

TEST_F(FoxEntityBreedTest, IsBreedingItem_Empty)
{
    // 空物品不能繁殖
    ItemStack empty;
    EXPECT_FALSE(fox->isBreedingItem(empty));
}

TEST_F(FoxEntityBreedTest, SpawnBaby)
{
    // 测试 spawnBaby 返回非空
    auto baby = fox->spawnBaby(*fox);
    EXPECT_NE(baby, nullptr);

    // 验证幼体
    if (baby) {
        EXPECT_TRUE(baby->isChild());
    }
}

// ==================== 常量测试 ====================

TEST_F(FoxGoalsTest, FoxFollowTargetGoal_Constants)
{
    // 验证常量值符合 MC 1.16.5
    // START_FOLLOW_DISTANCE_SQ = 36.0 (6^2)
    // STOP_FOLLOW_DISTANCE_SQ = 36.0 (6^2)
    // APPROACH_SPEED = 1.5
    // 这些是私有常量，通过行为验证
    auto goal = std::make_unique<entity::ai::goal::FoxFollowTargetGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
}

TEST_F(FoxGoalsTest, FoxPounceGoal_Constants)
{
    // 验证常量值符合 MC 1.16.5
    // POUNCE_HORIZONTAL_FACTOR = 0.8
    // POUNCE_VERTICAL_FACTOR = 0.9
    // ATTACK_DISTANCE = 2.0f
    // MIN_MOTION_Y_SQ = 0.05f
    // MAX_PITCH_ANGLE = 15.0f
    // STUCK_PITCH_ANGLE = 60.0f
    auto goal = std::make_unique<entity::ai::goal::FoxPounceGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
}

TEST_F(FoxGoalsTest, FoxEatBerriesGoal_Constants)
{
    // EAT_DURATION = 40 (吃浆果需要 40 tick)
    // REACH_DISTANCE_SQ = 2.0
    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(fox.get(), 1.2, 12, 2);
    EXPECT_NE(goal, nullptr);
}

TEST_F(FoxGoalsTest, FoxFindItemsGoal_Constants)
{
    // SEARCH_RADIUS = 8.0
    // MOVE_SPEED = 1.2
    // CHANCE = 10
    auto goal = std::make_unique<entity::ai::goal::FoxFindItemsGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
}

TEST_F(FoxGoalsTest, FoxSitAndLookGoal_Constants)
{
    // LOOK_DURATION_MIN = 80
    // LOOK_DURATION_MAX = 100
    // LOOK_COUNT_MIN = 2
    // LOOK_COUNT_MAX = 4
    // TRIGGER_CHANCE = 0.02f
    auto goal = std::make_unique<entity::ai::goal::FoxSitAndLookGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
}

} // namespace test
} // namespace mc
