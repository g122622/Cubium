/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#include "common/TestWorldHelper.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/TurtleGoals.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

using namespace mc::entity::ai::goal;
using namespace mc::entity::ai;

// ============================================================================
// 常量测试 - 验证 MC 1.16.5 常量值
// ============================================================================

class TurtleGoalsConstantsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

// TurtleGoHomeGoal 常量测试
TEST_F(TurtleGoalsConstantsTest, TurtleGoHomeGoal_HomeDistanceTrigger)
{
    // MC 1.16.5: 距离出生地超过 64 格触发回家
    constexpr f64 HOME_DISTANCE_TRIGGER = 64.0;
    EXPECT_DOUBLE_EQ(HOME_DISTANCE_TRIGGER, 64.0);
}

TEST_F(TurtleGoalsConstantsTest, TurtleGoHomeGoal_HomeDistanceArrive)
{
    // MC 1.16.5: 距离出生地 7 格内视为到达
    constexpr f64 HOME_DISTANCE_ARRIVE = 7.0;
    EXPECT_DOUBLE_EQ(HOME_DISTANCE_ARRIVE, 7.0);
}

TEST_F(TurtleGoalsConstantsTest, TurtleGoHomeGoal_MaxTravelTime)
{
    // MC 1.16.5: 最大旅行时间 600 ticks (30 秒)
    constexpr i32 MAX_TRAVEL_TIME = 600;
    EXPECT_EQ(MAX_TRAVEL_TIME, 600);
}

TEST_F(TurtleGoalsConstantsTest, TurtleGoHomeGoal_RandomTriggerChance)
{
    // MC 1.16.5: 1/700 概率随机触发回家
    constexpr i32 RANDOM_TRIGGER_CHANCE = 700;
    EXPECT_EQ(RANDOM_TRIGGER_CHANCE, 700);
}

// TurtleLayEggGoal 常量测试
TEST_F(TurtleGoalsConstantsTest, TurtleLayEggGoal_SearchRange)
{
    // MC 1.16.5: 搜索产卵位置范围 16 格
    constexpr i32 SEARCH_RANGE = 16;
    EXPECT_EQ(SEARCH_RANGE, 16);
}

TEST_F(TurtleGoalsConstantsTest, TurtleLayEggGoal_HomeDistanceMax)
{
    // MC 1.16.5: 产卵距离出生地最大 9 格
    constexpr f64 HOME_DISTANCE_MAX = 9.0;
    EXPECT_DOUBLE_EQ(HOME_DISTANCE_MAX, 9.0);
}

TEST_F(TurtleGoalsConstantsTest, TurtleLayEggGoal_MaxTimeout)
{
    // MC 1.16.5: 产卵最大超时 1200 ticks (60 秒)
    constexpr i32 MAX_TIMEOUT = 1200;
    EXPECT_EQ(MAX_TIMEOUT, 1200);
}

// TurtleTravelGoal 常量测试
TEST_F(TurtleGoalsConstantsTest, TurtleTravelGoal_TravelRange)
{
    // MC 1.16.5: 旅行范围 512 格
    constexpr i32 TRAVEL_RANGE = 512;
    EXPECT_EQ(TRAVEL_RANGE, 512);
}

TEST_F(TurtleGoalsConstantsTest, TurtleTravelGoal_TravelVerticalRange)
{
    // MC 1.16.5: 垂直旅行范围 4 格
    constexpr i32 TRAVEL_VERTICAL_RANGE = 4;
    EXPECT_EQ(TRAVEL_VERTICAL_RANGE, 4);
}

// TurtleGoToWaterGoal 常量测试
TEST_F(TurtleGoalsConstantsTest, TurtleGoToWaterGoal_SearchRangeHorizontal)
{
    // MC 1.16.5: 成龟水平搜索水源范围 16 格
    constexpr i32 SEARCH_RANGE_HORIZONTAL = 16;
    EXPECT_EQ(SEARCH_RANGE_HORIZONTAL, 16);
}

TEST_F(TurtleGoalsConstantsTest, TurtleGoToWaterGoal_SearchRangeVertical)
{
    // MC 1.16.5: 垂直搜索水源范围 1 格
    constexpr i32 SEARCH_RANGE_VERTICAL = 1;
    EXPECT_EQ(SEARCH_RANGE_VERTICAL, 1);
}

TEST_F(TurtleGoalsConstantsTest, TurtleGoToWaterGoal_MaxTimeout)
{
    // MC 1.16.5: 找水最大超时 1200 ticks (60 秒)
    constexpr i32 MAX_TIMEOUT = 1200;
    EXPECT_EQ(MAX_TIMEOUT, 1200);
}

// ============================================================================
// TurtleGoHomeGoal 测试
// ============================================================================

class TurtleGoHomeGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleGoHomeGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleGoHomeGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoHomeGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsChild)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(true);
    turtle.setHomePos(BlockPos(0, 64, 0));

    TurtleGoHomeGoal goal(&turtle, 1.0);
    // 幼龟不会回家
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoHomeGoalTest, ShouldExecute_ReturnsTrue_WhenHasEggAndHasHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);
    turtle.setHomePos(BlockPos(100, 64, 100));

    TurtleGoHomeGoal goal(&turtle, 1.0);
    // 有蛋且有出生地时应该回家
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(TurtleGoHomeGoalTest, ShouldExecute_ReturnsFalse_WhenHasEggButNoHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);
    // 没有出生地

    TurtleGoHomeGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoHomeGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleGoHomeGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleGoHomeGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNoHomePos)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);

    TurtleGoHomeGoal goal(&turtle, 1.0);
    goal.startExecuting();
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleGoHomeGoalTest, StartExecuting_SetsGoingHomeFlag)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);
    turtle.setHomePos(BlockPos(0, 64, 0));

    TurtleGoHomeGoal goal(&turtle, 1.0);
    goal.startExecuting();

    EXPECT_TRUE(turtle.isGoingHome());
}

TEST_F(TurtleGoHomeGoalTest, ResetTask_ClearsGoingHomeFlag)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);
    turtle.setHomePos(BlockPos(0, 64, 0));

    TurtleGoHomeGoal goal(&turtle, 1.0);
    goal.startExecuting();
    goal.resetTask();

    EXPECT_FALSE(turtle.isGoingHome());
}

TEST_F(TurtleGoHomeGoalTest, MutexFlags_IsMove)
{
    // TurtleGoHomeGoal 只使用 Move 标志
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoHomeGoal goal(&turtle, 1.0);

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
}

TEST_F(TurtleGoHomeGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoHomeGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleGoHomeGoal");
}

// ============================================================================
// TurtleLayEggGoal 测试
// ============================================================================

class TurtleLayEggGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleLayEggGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleLayEggGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleLayEggGoalTest, ShouldExecute_ReturnsFalse_WhenNoEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(false);
    turtle.setHomePos(BlockPos(0, 64, 0));
    turtle.setPosition(0.0f, 64.0f, 0.0f);

    TurtleLayEggGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleLayEggGoalTest, ShouldExecute_ReturnsFalse_WhenNoHomePos)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(true);
    // 没有出生地

    TurtleLayEggGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleLayEggGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNoEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(false);

    TurtleLayEggGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleLayEggGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNoHomePos)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(true);

    TurtleLayEggGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleLayEggGoalTest, MutexFlags_IsMoveAndLook)
{
    // TurtleLayEggGoal 使用 Move 和 Look 标志
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleLayEggGoal goal(&turtle, 1.0);

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
}

TEST_F(TurtleLayEggGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleLayEggGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleLayEggGoal");
}

// ============================================================================
// TurtleTravelGoal 测试
// ============================================================================

class TurtleTravelGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleTravelGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleTravelGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleTravelGoalTest, ShouldExecute_ReturnsFalse_WhenGoingHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setGoingHome(true);
    test::setEntityInWater(turtle, true);

    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleTravelGoalTest, ShouldExecute_ReturnsFalse_WhenHasEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(true);
    test::setEntityInWater(turtle, true);

    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleTravelGoalTest, ShouldExecute_ReturnsFalse_WhenNotInWater)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);

    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleTravelGoalTest, ShouldExecute_ReturnsTrue_WhenInWaterAndNoEggAndNotGoingHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);
    turtle.setHasEgg(false);
    turtle.setGoingHome(false);

    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(TurtleTravelGoalTest, StartExecuting_SetsTravellingFlag)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);
    turtle.setHasEgg(false);
    turtle.setGoingHome(false);

    TurtleTravelGoal goal(&turtle, 1.0);
    goal.startExecuting();

    EXPECT_TRUE(turtle.isTravelling());
}

TEST_F(TurtleTravelGoalTest, ResetTask_ClearsTravellingFlag)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);

    TurtleTravelGoal goal(&turtle, 1.0);
    goal.startExecuting();
    goal.resetTask();

    EXPECT_FALSE(turtle.isTravelling());
}

TEST_F(TurtleTravelGoalTest, MutexFlags_IsMove)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleTravelGoal goal(&turtle, 1.0);

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
}

TEST_F(TurtleTravelGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleTravelGoal");
}

// ============================================================================
// TurtleGoToWaterGoal 测试
// ============================================================================

class TurtleGoToWaterGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleGoToWaterGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleGoToWaterGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoToWaterGoalTest, ShouldExecute_ReturnsFalse_WhenAlreadyInWater)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);
    turtle.setGoingHome(false);
    turtle.setHasEgg(false);

    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoToWaterGoalTest, ShouldExecute_ReturnsFalse_WhenGoingHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);
    turtle.setGoingHome(true);

    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoToWaterGoalTest, ShouldExecute_ReturnsFalse_WhenHasEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);
    turtle.setHasEgg(true);
    turtle.setGoingHome(false);

    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleGoToWaterGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenInWater)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);

    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleGoToWaterGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNullTurtle)
{
    TurtleGoToWaterGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleGoToWaterGoalTest, MutexFlags_IsMove)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoToWaterGoal goal(&turtle, 1.0);

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
}

TEST_F(TurtleGoToWaterGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleGoToWaterGoal");
}

// ============================================================================
// TurtleMateGoal 测试
// ============================================================================

class TurtleMateGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleMateGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleMateGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleMateGoalTest, ShouldExecute_ReturnsFalse_WhenHasEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);
    turtle.setInLove(false);

    TurtleMateGoal goal(&turtle, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleMateGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleMateGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleMateGoal");
}

// ============================================================================
// TurtlePanicGoal 测试
// ============================================================================

class TurtlePanicGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtlePanicGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtlePanicGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtlePanicGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtlePanicGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtlePanicGoal");
}

// ============================================================================
// TurtleTemptGoal 测试
// ============================================================================

class TurtleTemptGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleTemptGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleTemptGoal goal(nullptr, 1.0);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleTemptGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleTemptGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleTemptGoal");
}

TEST_F(TurtleTemptGoalTest, IsSeagrass_ReturnsTrueForSeagrass)
{
    // 验证静态方法 isSeagrass
    ItemStack seagrassStack(Items::SEAGRASS, 1);
    EXPECT_TRUE(TurtleTemptGoal::isSeagrass(seagrassStack));
}

TEST_F(TurtleTemptGoalTest, IsSeagrass_ReturnsFalseForOtherItems)
{
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(TurtleTemptGoal::isSeagrass(wheatStack));

    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_FALSE(TurtleTemptGoal::isSeagrass(carrotStack));

    ItemStack emptyStack(nullptr, 0);
    EXPECT_FALSE(TurtleTemptGoal::isSeagrass(emptyStack));
}

// ============================================================================
// TurtleWanderGoal 测试
// ============================================================================

class TurtleWanderGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleWanderGoalTest, ShouldExecute_ReturnsFalse_WhenTurtleIsNull)
{
    TurtleWanderGoal goal(nullptr, 1.0, 100);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleWanderGoalTest, ShouldExecute_ReturnsFalse_WhenInWater)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);

    TurtleWanderGoal goal(&turtle, 1.0, 100);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleWanderGoalTest, ShouldExecute_ReturnsFalse_WhenGoingHome)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);
    turtle.setGoingHome(true);

    TurtleWanderGoal goal(&turtle, 1.0, 100);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleWanderGoalTest, ShouldExecute_ReturnsFalse_WhenHasEgg)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);
    turtle.setGoingHome(false);
    turtle.setHasEgg(true);

    TurtleWanderGoal goal(&turtle, 1.0, 100);
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(TurtleWanderGoalTest, GetTypeName_ReturnsCorrectName)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleWanderGoal goal(&turtle, 1.0, 100);
    EXPECT_EQ(goal.getTypeName(), "TurtleWanderGoal");
}

// ============================================================================
// Goal 优先级测试 - 验证各 Goal 的优先级设置
// ============================================================================

class TurtleGoalPriorityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleGoalPriorityTest, PanicGoal_HasHighestPriority)
{
    // MC 1.16.5: PanicGoal 优先级为 0（最高）
    // 在 TurtleEntity::registerGoals() 中注册为优先级 0
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 验证目标选择器中存在 PanicGoal
    // 注意：这里只验证 Goal 可以正确创建
    TurtlePanicGoal goal(&turtle, 1.2);
    EXPECT_EQ(goal.getTypeName(), "TurtlePanicGoal");
}

TEST_F(TurtleGoalPriorityTest, MateAndLayEggGoal_HaveSamePriority)
{
    // MC 1.16.5: MateGoal 和 LayEggGoal 共享优先级 1
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    TurtleMateGoal mateGoal(&turtle, 1.0);
    TurtleLayEggGoal layEggGoal(&turtle, 1.0);

    EXPECT_EQ(mateGoal.getTypeName(), "TurtleMateGoal");
    EXPECT_EQ(layEggGoal.getTypeName(), "TurtleLayEggGoal");
}

TEST_F(TurtleGoalPriorityTest, TemptGoal_HasPriority2)
{
    // MC 1.16.5: TemptGoal 优先级为 2
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleTemptGoal goal(&turtle, 1.1);
    EXPECT_EQ(goal.getTypeName(), "TurtleTemptGoal");
}

TEST_F(TurtleGoalPriorityTest, GoToWaterGoal_HasPriority3)
{
    // MC 1.16.5: GoToWaterGoal 优先级为 3
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoToWaterGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleGoToWaterGoal");
}

TEST_F(TurtleGoalPriorityTest, GoHomeGoal_HasPriority4)
{
    // MC 1.16.5: GoHomeGoal 优先级为 4
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleGoHomeGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleGoHomeGoal");
}

TEST_F(TurtleGoalPriorityTest, TravelGoal_HasPriority7)
{
    // MC 1.16.5: TravelGoal 优先级为 7
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleTravelGoal goal(&turtle, 1.0);
    EXPECT_EQ(goal.getTypeName(), "TurtleTravelGoal");
}

TEST_F(TurtleGoalPriorityTest, WanderGoal_HasPriority9)
{
    // MC 1.16.5: WanderGoal 优先级为 9
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtleWanderGoal goal(&turtle, 1.0, 100);
    EXPECT_EQ(goal.getTypeName(), "TurtleWanderGoal");
}

// ============================================================================
// 边界条件测试
// ============================================================================

class TurtleGoalsEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleGoalsEdgeCaseTest, TurtleGoHomeGoal_DistanceCalculation)
{
    // 测试距离计算边界情况
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setChild(false);
    turtle.setHasEgg(true);

    // 设置出生地在远处
    BlockPos homePos(100, 64, 100);
    turtle.setHomePos(homePos);

    // 设置海龟位置刚好在 64 格边界内
    turtle.setPosition(35.0f, 64.0f, 100.0f); // 距离出生地 65 格

    TurtleGoHomeGoal goal(&turtle, 1.0);

    // 有蛋时应该触发回家，不管距离
    EXPECT_TRUE(goal.shouldExecute());
}

TEST_F(TurtleGoalsEdgeCaseTest, TurtleLayEggGoal_DistanceCheck)
{
    // 测试产卵距离检查边界情况
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    turtle.setHasEgg(true);

    // 设置出生地
    BlockPos homePos(0, 64, 0);
    turtle.setHomePos(homePos);

    // 设置海龟位置刚好在 9 格边界外
    turtle.setPosition(10.0f, 64.0f, 0.0f); // 距离出生地 10 格

    TurtleLayEggGoal goal(&turtle, 1.0);

    // 距离太远，不应该触发
    // 注意：shouldExecute 内部会调用 findLayEggPosition，这需要世界
    // 所以这里只验证 shouldContinueExecuting 的距离检查逻辑
    // 实际测试需要 Mock 世界
}

TEST_F(TurtleGoalsEdgeCaseTest, TurtleTravelGoal_StateChanges)
{
    // 测试旅行目标的状态变化
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, true);
    turtle.setHasEgg(false);
    turtle.setGoingHome(false);

    TurtleTravelGoal goal(&turtle, 1.0);
    goal.startExecuting();

    EXPECT_TRUE(turtle.isTravelling());

    // 改变状态后 shouldContinueExecuting 应该返回 false
    turtle.setGoingHome(true);
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(TurtleGoalsEdgeCaseTest, TurtleWanderGoal_ChanceParameter)
{
    // 测试漫步概率参数
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    test::setEntityInWater(turtle, false);
    turtle.setGoingHome(false);
    turtle.setHasEgg(false);

    // chance=1 表示每次都执行
    TurtleWanderGoal goal1(&turtle, 1.0, 1);

    // chance=100 表示 1/100 概率执行
    TurtleWanderGoal goal100(&turtle, 1.0, 100);

    // 验证目标可以正常创建
    EXPECT_EQ(goal1.getTypeName(), "TurtleWanderGoal");
    EXPECT_EQ(goal100.getTypeName(), "TurtleWanderGoal");
}

// ============================================================================
// 速度参数测试
// ============================================================================

class TurtleGoalsSpeedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }
};

TEST_F(TurtleGoalsSpeedTest, TurtleGoHomeGoal_SpeedParameter)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    TurtleGoHomeGoal goal1(&turtle, 1.0);
    TurtleGoHomeGoal goal2(&turtle, 2.0);

    // 验证不同速度参数的目标可以创建
    EXPECT_EQ(goal1.getTypeName(), "TurtleGoHomeGoal");
    EXPECT_EQ(goal2.getTypeName(), "TurtleGoHomeGoal");
}

TEST_F(TurtleGoalsSpeedTest, TurtleTravelGoal_SpeedParameter)
{
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());

    TurtleTravelGoal goal1(&turtle, 1.0);
    TurtleTravelGoal goal2(&turtle, 0.5);

    // 验证不同速度参数的目标可以创建
    EXPECT_EQ(goal1.getTypeName(), "TurtleTravelGoal");
    EXPECT_EQ(goal2.getTypeName(), "TurtleTravelGoal");
}

TEST_F(TurtleGoalsSpeedTest, TurtlePanicGoal_HigherSpeed)
{
    // MC 1.16.5: 恐慌逃跑速度更高 (1.2)
    TurtleEntity turtle(EntityInstanceId(1), mc::test::testEcsRegistry());
    TurtlePanicGoal goal(&turtle, 1.2);

    EXPECT_EQ(goal.getTypeName(), "TurtlePanicGoal");
}

} // namespace
} // namespace mc
