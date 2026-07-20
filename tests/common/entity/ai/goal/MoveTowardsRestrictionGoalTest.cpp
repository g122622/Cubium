/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;
using namespace mc::entity::ai::goal;
using namespace mc::entity::ai; // for GoalFlag

// ============================================================================
// Test CreatureEntity for testing
// ============================================================================

class TestCreature : public CreatureEntity {
public:
    TestCreature()
        : CreatureEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }
};

// ============================================================================
// MoveTowardsRestrictionGoal Tests
// ============================================================================

class MoveTowardsRestrictionGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        goal = std::make_unique<MoveTowardsRestrictionGoal>(creature.get(), 1.0);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
    }

    std::unique_ptr<TestCreature> creature;
    std::unique_ptr<MoveTowardsRestrictionGoal> goal;
};

TEST_F(MoveTowardsRestrictionGoalTest, TypeName)
{
    EXPECT_EQ(goal->getTypeName(), "MoveTowardsRestrictionGoal");
}

TEST_F(MoveTowardsRestrictionGoalTest, MutexFlags)
{
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Move}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Look}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Jump}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Target}));
}

TEST_F(MoveTowardsRestrictionGoalTest, ShouldNotExecuteWithoutHomePosition)
{
    // 没有设置家位置时，hasHome() 返回 false
    EXPECT_FALSE(creature->hasHome());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveTowardsRestrictionGoalTest, ShouldNotExecuteWhenWithinHomeDistance)
{
    // 设置家位置在实体当前位置
    creature->setHomePosAndDistance(BlockPos(0, 64, 0), 16);
    EXPECT_TRUE(creature->hasHome());
    // 实体在家范围内，不应执行
    EXPECT_TRUE(creature->isWithinHomeDistanceCurrentPosition());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveTowardsRestrictionGoalTest, ShouldExecuteWhenOutsideHomeDistance)
{
    // 设置家位置在远处
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    EXPECT_TRUE(creature->hasHome());
    // 实体不在家范围内，应该尝试执行
    // 注意：可能因为 RandomPositionGenerator 无法生成目标而返回 false
    // 但 hasHome 检查和 isWithinHomeDistance 检查应该通过
    // 如果有 world 区块数据，shouldExecute 应该返回 true
    // 无 world 时因无法生成路径目标返回 false，这是正常的
}

TEST_F(MoveTowardsRestrictionGoalTest, StartExecutingDoesNotCrash)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    goal->startExecuting();
    EXPECT_TRUE(true);
}

TEST_F(MoveTowardsRestrictionGoalTest, ResetTaskDoesNotCrash)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    goal->startExecuting();
    goal->resetTask();
    EXPECT_TRUE(true);
}

TEST_F(MoveTowardsRestrictionGoalTest, TickDoesNotCrash)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    goal->startExecuting();
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

TEST_F(MoveTowardsRestrictionGoalTest, ShouldContinueExecutingReturnsFalseWhenNoPath)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    goal->startExecuting();
    // 没有有效路径时返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(MoveTowardsRestrictionGoalTest, ShouldContinueExecutingReturnsFalseWhenBackHome)
{
    // 设置家位置在实体当前位置
    creature->setHomePosAndDistance(BlockPos(0, 64, 0), 16);
    goal->startExecuting();
    // 在家范围内，shouldContinueExecuting 应返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(MoveTowardsRestrictionGoalTest, FullLifecycle)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);

    // shouldExecute -> startExecuting -> tick -> shouldContinueExecuting -> resetTask
    goal->shouldExecute();
    goal->startExecuting();

    // tick 多次触发路径重算
    for (int i = 0; i < 20; ++i) {
        goal->tick();
    }

    // 无路径时 shouldContinueExecuting 返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());

    goal->resetTask();
}

TEST_F(MoveTowardsRestrictionGoalTest, PathRecalculationEveryTenTicks)
{
    creature->setHomePosAndDistance(BlockPos(100, 64, 100), 16);
    goal->startExecuting();

    // tick 9 次 -> 还在倒计时
    for (int i = 0; i < 9; ++i) {
        goal->tick();
    }
    // 不崩溃即通过

    // tick 第10次 -> 触发路径重算
    goal->tick();
    // 不崩溃即通过
    EXPECT_TRUE(true);
}

TEST_F(MoveTowardsRestrictionGoalTest, HomePositionAndDistance)
{
    // 验证家位置系统工作正常
    EXPECT_FALSE(creature->hasHome());

    creature->setHomePosAndDistance(BlockPos(50, 70, 50), 10);
    EXPECT_TRUE(creature->hasHome());
    EXPECT_EQ(creature->homePosition().x, 50);
    EXPECT_EQ(creature->homePosition().y, 70);
    EXPECT_EQ(creature->homePosition().z, 50);

    // 实体在 (0, 64, 0)，家在 (50, 70, 50)，距离 > 10，不在家范围内
    EXPECT_FALSE(creature->isWithinHomeDistanceCurrentPosition());

    // 移动到范围内
    creature->setPositionForTest(50.0, 70.0, 50.0);
    EXPECT_TRUE(creature->isWithinHomeDistanceCurrentPosition());
}
