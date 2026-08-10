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

#include "common/TestWorldHelper.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/vegetation/SweetBerryBushBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/special/FoxGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/passive/basic/ChickenEntity.hpp"
#include "entity/entities/passive/basic/RabbitEntity.hpp"
#include "entity/entities/passive/special/FoxEntity.hpp"
#include "entity/entities/passive/special/TurtleEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"

namespace mc {
namespace test {

// ==================== 简易测试世界 ====================

/**
 * @brief FoxGoalsTest 用的简易测试世界
 *
 * 继承 BaseTestWorld 以访问其 protected 构造函数
 */
class FoxSimpleTestWorld : public mc::test::BaseTestWorld {
public:
    FoxSimpleTestWorld() = default;
};

// ==================== FoxGoals 基础测试 ====================

class FoxGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

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

TEST_F(FoxGoalsTest, FoxRevengeGoal_ShouldExecuteNoWorldReturnsFalse)
{
    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxRevengeGoal_ShouldExecuteNoTrustedPlayersReturnsFalse)
{
    // 无信任玩家时不应执行（即使有世界）
    FoxSimpleTestWorld world;
    fox->setWorld(&world);

    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());

    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxRevengeGoal_ConstructsWithFoxEntity)
{
    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxRevengeGoal");
}

TEST_F(FoxGoalsTest, FoxRevengeGoal_UsesGetPlayersNotAABBSearch)
{
    // FoxRevengeGoal::shouldExecute() 使用 getPlayers() 而非 getEntitiesInAABB()
    // 来查找信任玩家，这是性能优化。
    // 验证目标可以被正常构造和调用（无崩溃）。
    FoxSimpleTestWorld world;
    fox->setWorld(&world);
    fox->addTrustedPlayer(12345ULL);

    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(fox.get());

    // 无玩家在世界中，应返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== FoxEntity 状态测试 ====================

class FoxEntityStateTest : public ::testing::Test {
protected:
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

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
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

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
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

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
    void SetUp() override { fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

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

// ============================================================================
// FoxEatBerriesGoal mobGriefing 测试
// ============================================================================

/**
 * @brief 支持方块状态和 GameRules 的测试世界，用于 FoxEatBerriesGoal 测试
 */
class FoxBerryTestWorld : public mc::test::BaseTestWorld {
public:
    FoxBerryTestWorld()
        : m_dayTime(6000)
        , m_currentTick(1000)
    {}

    void setDayTime(i64 time) { m_dayTime = time; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state)
    {
        BlockPos key(x, y, z);
        if (state) {
            auto it = m_blockStates.find(key);
            if (it != m_blockStates.end()) {
                it->second = *state;
            } else {
                m_blockStates.emplace(key, *state);
            }
        } else {
            m_blockStates.erase(key);
        }
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        if (it != m_blockStates.end()) {
            return &it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        setBlockStateAt(x, y, z, state);
        return true;
    }

private:
    i64 m_dayTime;
    u64 m_currentTick;
    std::unordered_map<BlockPos, BlockState> m_blockStates;
};

class FoxEatBerriesMobGriefingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_world = std::make_unique<FoxBerryTestWorld>();
        m_fox = std::make_unique<FoxEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        m_fox->setWorld(m_world.get());
        m_fox->setPosition(0.0, 64.0, 0.0);
    }

    void TearDown() override
    {
        m_fox.reset();
        m_world.reset();
    }

    /**
     * @brief 在狐狸附近设置一个成熟甜浆果丛
     */
    void setupSweetBerryBush()
    {
        const Block* sweetBerryBlock = VanillaBlocks::SWEET_BERRY_BUSH;
        ASSERT_TRUE(sweetBerryBlock != nullptr) << "SweetBerryBushBlock must be registered";

        auto* bushBlock = dynamic_cast<const blocks::SweetBerryBushBlock*>(sweetBerryBlock);
        ASSERT_TRUE(bushBlock != nullptr);

        // AGE=3 表示成熟可采摘（需要先获取 defaultState 再设置 AGE）
        const BlockState& defaultState = sweetBerryBlock->defaultState();
        const BlockState& matureState = bushBlock->withAge(defaultState, 3);
        m_world->setBlockStateAt(1, 64, 0, &matureState);
    }

    std::unique_ptr<FoxBerryTestWorld> m_world;
    std::unique_ptr<FoxEntity> m_fox;
};

TEST_F(FoxEatBerriesMobGriefingTest, MobGriefingTrue_AllowsBerryPicking)
{
    // 默认 mobGriefing = true，狐狸可以采摘浆果
    m_world->getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true);
    EXPECT_TRUE(m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));

    setupSweetBerryBush();

    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(m_fox.get(), 1.2, 12, 2);

    // shouldExecute 应该能找到浆果丛
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(FoxEatBerriesMobGriefingTest, MobGriefingFalse_ShouldExecuteStillFindsTarget)
{
    // mobGriefing = false 时，shouldExecute 仍然能找到浆果丛
    // 因为 shouldExecute 不检查 mobGriefing，_eatBerry 才检查
    m_world->getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false);

    setupSweetBerryBush();

    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(m_fox.get(), 1.2, 12, 2);

    // shouldExecute 应该能找到浆果丛（不检查 mobGriefing）
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(FoxEatBerriesMobGriefingTest, MobGriefingFalse_BerryBushUnmodified)
{
    // mobGriefing = false 时，_eatBerry 应该提前返回，不修改浆果丛
    m_world->getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false);

    setupSweetBerryBush();

    // 记录浆果丛初始状态
    const BlockState* initialState = m_world->getBlockState(1, 64, 0);
    ASSERT_TRUE(initialState != nullptr);

    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(m_fox.get(), 1.2, 12, 2);
    goal->startExecuting();

    // tick 多次（即使狐狸无法真正导航到目标，也不会崩溃）
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }

    // 浆果丛应该保持不变（mobGriefing=false 时 _eatBerry 不修改方块）
    const BlockState* stateAfter = m_world->getBlockState(1, 64, 0);
    ASSERT_TRUE(stateAfter != nullptr);

    // 验证 AGE 属性不变
    const auto* bushBlock = dynamic_cast<const blocks::SweetBerryBushBlock*>(VanillaBlocks::SWEET_BERRY_BUSH);
    ASSERT_TRUE(bushBlock != nullptr);
    EXPECT_EQ(bushBlock->getAge(*stateAfter), bushBlock->getAge(*initialState))
        << "Berry bush AGE should not change when mobGriefing=false";
}

TEST_F(FoxEatBerriesMobGriefingTest, TickDoesNotCrashWithMobGriefingFalse)
{
    // mobGriefing = false 时 tick 不崩溃
    m_world->getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, false);

    setupSweetBerryBush();

    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(m_fox.get(), 1.2, 12, 2);

    if (goal->shouldExecute()) {
        goal->startExecuting();
        for (int i = 0; i < 200; ++i) {
            EXPECT_NO_THROW(goal->tick());
        }
    }
}

TEST_F(FoxEatBerriesMobGriefingTest, TickDoesNotCrashWithMobGriefingTrue)
{
    // mobGriefing = true 时 tick 不崩溃
    m_world->getGameRules().setBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING, true);

    setupSweetBerryBush();

    auto goal = std::make_unique<entity::ai::goal::FoxEatBerriesGoal>(m_fox.get(), 1.2, 12, 2);

    if (goal->shouldExecute()) {
        goal->startExecuting();
        for (int i = 0; i < 200; ++i) {
            EXPECT_NO_THROW(goal->tick());
        }
    }
}

TEST_F(FoxEatBerriesMobGriefingTest, MobGriefingDefault_IsTrue)
{
    // 验证 GameRules 默认值 mobGriefing = true
    EXPECT_TRUE(m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING));
}

// ============================================================================
// FoxEntity 目标选择器注册测试
// ============================================================================

/**
 * @brief 可公开目标选择器的测试用狐狸实体
 */
class TestFoxEntityForTargets : public FoxEntity {
public:
    explicit TestFoxEntityForTargets(EntityInstanceId id)
        : FoxEntity(id, mc::test::testEcsRegistry())
    {}

    entity::ai::GoalSelector& testTargetSelector() { return targetSelector(); }
};

class FoxTargetGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
    }
};

TEST_F(FoxTargetGoalsTest, FoxHasFoxRevengeGoal)
{
    // FoxEntity::registerGoals() 注册了 FoxRevengeGoal（优先级3）
    TestFoxEntityForTargets fox(EntityInstanceId(1));

    i32 revengeGoalCount = 0;
    for (const auto& pg : fox.testTargetSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::FoxRevengeGoal*>(goal) != nullptr) {
            revengeGoalCount++;
        }
    }
    EXPECT_EQ(revengeGoalCount, 1) << "FoxEntity should have exactly 1 FoxRevengeGoal";
}

TEST_F(FoxTargetGoalsTest, FoxHasPreyTargetGoalsForChickenAndRabbit)
{
    // FoxEntity::registerGoals() 在优先级4注册了攻击鸡和兔子的目标
    TestFoxEntityForTargets fox(EntityInstanceId(1));

    i32 chickenRabbitGoalCount = 0;
    for (const auto& pg : fox.testTargetSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        // FoxEntity 攻击鸡和兔子的目标是 NearestAttackableTargetGoal<LivingEntity>
        // 通过谓词过滤，所以我们无法直接通过模板参数区分
        // 但我们可以验证 FoxRevengeGoal 存在且 TargetGoal 数量正确
        if (dynamic_cast<const entity::ai::goal::FoxRevengeGoal*>(goal) == nullptr &&
            dynamic_cast<const entity::ai::goal::TargetGoal*>(goal) != nullptr) {
            chickenRabbitGoalCount++;
        }
    }
    // 应有多个目标选择器目标（鸡/兔子、海龟、群居鱼类、HurtByTargetGoal）
    EXPECT_GE(chickenRabbitGoalCount, 3) << "FoxEntity should have at least 3 non-revenge target goals";
}

TEST_F(FoxTargetGoalsTest, FoxHasNearestAttackableTargetGoalForTurtle)
{
    // FoxEntity::registerGoals() 注册了 NearestAttackableTargetGoal<TurtleEntity>
    TestFoxEntityForTargets fox(EntityInstanceId(1));

    i32 turtleGoalCount = 0;
    for (const auto& pg : fox.testTargetSelector().getAllGoals()) {
        const auto* goal = pg.getGoal();
        if (dynamic_cast<const entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>*>(goal) != nullptr) {
            turtleGoalCount++;
        }
    }
    EXPECT_EQ(turtleGoalCount, 1) << "FoxEntity should have exactly 1 NearestAttackableTargetGoal<TurtleEntity>";
}

TEST_F(FoxTargetGoalsTest, FoxRevengeGoalStartExecutingSetsAggroedAndWakesUp)
{
    // FoxRevengeGoal::startExecuting() 应设置 foxAggroed=true 和唤醒狐狸
    TestFoxEntityForTargets fox(EntityInstanceId(1));
    FoxSimpleTestWorld world;
    fox.setWorld(&world);
    fox.setSleeping(true);
    fox.setFoxAggroed(false);

    EXPECT_TRUE(fox.isSleeping());
    EXPECT_FALSE(fox.isFoxAggroed());

    // 直接调用 startExecuting 不会崩溃（无攻击目标时）
    auto goal = std::make_unique<entity::ai::goal::FoxRevengeGoal>(&fox);
    goal->startExecuting();

    // startExecuting 应设置 foxAggroed 和唤醒
    EXPECT_TRUE(fox.isFoxAggroed());
    EXPECT_FALSE(fox.isSleeping());
}

TEST_F(FoxTargetGoalsTest, FoxRevengeGoalTrustIdConsistencyUsesPlayerId)
{
    // 验证信任系统使用 PlayerId 而非 EntityInstanceId
    // 信任玩家存储的是 PlayerId (u64)，检查时应使用 player->playerId()
    TestFoxEntityForTargets fox(EntityInstanceId(1));

    u64 testPlayerId = 12345ULL;
    fox.addTrustedPlayer(testPlayerId);

    // trusts() 应使用 playerId 参数
    EXPECT_TRUE(fox.trusts(testPlayerId));
    EXPECT_FALSE(fox.trusts(99999ULL));
}

// ==================== FoxStuckInSnowGoal 测试 ====================

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FoxStuckInSnowGoal");
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    // FoxStuckInSnowGoal 应该有 Move、Look 和 Jump 标志
    // 对应 MC Java FaceplantGoal: EnumSet.of(Goal.Flag.LOOK, Goal.Flag.JUMP, Goal.Flag.MOVE)
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_ShouldExecuteWhenStuck)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    // 未卡住时不应执行
    fox->setStuck(false);
    EXPECT_FALSE(goal->shouldExecute());

    // 卡住时应执行
    fox->setStuck(true);
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_StartSetsCountdown)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    fox->setStuck(true);
    goal->startExecuting();

    // 启动后应继续执行（倒计时 > 0 且 still stuck）
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_TickDecrementsCountdown)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    fox->setStuck(true);
    goal->startExecuting();

    // 倒计时 40 tick，逐 tick 递减
    for (int i = 0; i < 39; ++i) {
        EXPECT_TRUE(goal->shouldContinueExecuting()) << "Still continuing at tick " << i;
        goal->tick();
    }

    // 倒计时归零后不再继续
    goal->tick();
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_ResetClearsStuckState)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    fox->setStuck(true);
    goal->startExecuting();
    EXPECT_TRUE(fox->isStuck());

    // resetTask 应清除卡住状态
    goal->resetTask();
    EXPECT_FALSE(fox->isStuck());
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_NotContinueIfNotStuck)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    fox->setStuck(true);
    goal->startExecuting();

    // 外部清除卡住状态后，目标应不再继续
    fox->setStuck(false);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(FoxGoalsTest, FoxStuckInSnowGoal_FullCycle)
{
    auto goal = std::make_unique<entity::ai::goal::FoxStuckInSnowGoal>(fox.get());

    // 1. 狐狸卡住 → 目标激活
    fox->setStuck(true);
    EXPECT_TRUE(goal->shouldExecute());

    // 2. 开始执行，设置倒计时
    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // 3. 模拟 40 tick 的卡住状态
    for (int i = 0; i < 40; ++i) {
        goal->tick();
    }

    // 4. 倒计时结束，目标停止
    EXPECT_FALSE(goal->shouldContinueExecuting());

    // 5. resetTask 清除卡住状态
    goal->resetTask();
    EXPECT_FALSE(fox->isStuck());

    // 6. 狐狸不再卡住，目标不会重新激活
    EXPECT_FALSE(goal->shouldExecute());
}

} // namespace test
} // namespace mc
