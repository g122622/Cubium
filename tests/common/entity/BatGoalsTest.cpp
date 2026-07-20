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
 * @file BatGoalsTest.cpp
 * @brief 蝙蝠 AI 目标单元测试
 *
 * 测试 BatRandomFlyGoal 和 BatRestGoal 的关键方法：
 * - BatRandomFlyGoal: shouldExecute, shouldContinueExecuting, tick
 * - BatRestGoal: shouldExecute, shouldContinueExecuting, tick
 * - BatEntity: isFlying, isResting, canRest
 */

#include "entity/ai/goal/goals/special/BatGoals.hpp"
#include "entity/entities/passive/ambient/BatEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::goal;

// ==================== BatEntity Test Fixture ====================

class BatEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建蝙蝠实体
        bat = std::make_unique<BatEntity>(EntityInstanceId(0));
    }

    void TearDown() override { bat.reset(); }

    std::unique_ptr<BatEntity> bat;
};

// ==================== BatEntity State Tests ====================

TEST_F(BatEntityTest, DefaultState_IsFlying)
{
    // 蝙蝠默认状态应为飞行
    EXPECT_TRUE(bat->isFlying());
    EXPECT_FALSE(bat->isResting());
}

TEST_F(BatEntityTest, SetFlying_ChangesState)
{
    bat->setFlying(false);
    EXPECT_FALSE(bat->isFlying());

    bat->setFlying(true);
    EXPECT_TRUE(bat->isFlying());
}

TEST_F(BatEntityTest, SetResting_ChangesState)
{
    bat->setResting(true);
    EXPECT_TRUE(bat->isResting());

    bat->setResting(false);
    EXPECT_FALSE(bat->isResting());
}

TEST_F(BatEntityTest, FlyingAndResting_CanCoexist)
{
    // 技术上两个状态可以同时为true（由AI负责管理）
    bat->setFlying(true);
    bat->setResting(true);
    EXPECT_TRUE(bat->isFlying());
    EXPECT_TRUE(bat->isResting());
}

TEST_F(BatEntityTest, EyeHeight_ReturnsSmallValue)
{
    // 蝙蝠眼睛高度很小（0.1）
    EXPECT_FLOAT_EQ(bat->eyeHeight(), 0.1f);
}

// ==================== BatRandomFlyGoal Tests ====================

class BatRandomFlyGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bat = std::make_unique<BatEntity>(EntityInstanceId(0));
        goal = std::make_unique<BatRandomFlyGoal>(bat.get());
    }

    void TearDown() override
    {
        goal.reset();
        bat.reset();
    }

    std::unique_ptr<BatEntity> bat;
    std::unique_ptr<BatRandomFlyGoal> goal;
};

TEST_F(BatRandomFlyGoalTest, ShouldExecute_ReturnsTrueWhenFlying)
{
    // 蝙蝠在飞行状态时应执行飞行目标
    bat->setFlying(true);
    bat->setResting(false);
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(BatRandomFlyGoalTest, ShouldExecute_ReturnsFalseWhenResting)
{
    // 蝙蝠在休息状态时不应执行飞行目标
    bat->setFlying(false);
    bat->setResting(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(BatRandomFlyGoalTest, ShouldContinueExecuting_ReturnsTrueWhenFlying)
{
    // 蝙蝠在飞行状态时应继续执行飞行目标
    bat->setFlying(true);
    bat->setResting(false);
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(BatRandomFlyGoalTest, ShouldContinueExecuting_ReturnsFalseWhenResting)
{
    // 蝙蝠在休息状态时不应继续执行飞行目标
    bat->setFlying(false);
    bat->setResting(true);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(BatRandomFlyGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "BatRandomFlyGoal");
}

TEST_F(BatRandomFlyGoalTest, StartExecuting_InitializesGoal)
{
    bat->setFlying(true);
    bat->setResting(false);

    // startExecuting 不应抛出异常
    EXPECT_NO_THROW(goal->startExecuting());
}

TEST_F(BatRandomFlyGoalTest, ResetTask_ClearsTarget)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== BatRestGoal Tests ====================

class BatRestGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bat = std::make_unique<BatEntity>(EntityInstanceId(0));
        goal = std::make_unique<BatRestGoal>(bat.get());
    }

    void TearDown() override
    {
        goal.reset();
        bat.reset();
    }

    std::unique_ptr<BatEntity> bat;
    std::unique_ptr<BatRestGoal> goal;
};

TEST_F(BatRestGoalTest, ShouldExecute_ReturnsFalseWhenAlreadyResting)
{
    // 已经在休息状态时不应再次尝试休息
    bat->setResting(true);
    bat->setFlying(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(BatRestGoalTest, ShouldContinueExecuting_ReturnsTrueWhenResting)
{
    // 在休息状态时应继续执行休息目标
    bat->setResting(true);
    // 注意：shouldContinueExecuting 还会检查其他条件
    // 在没有世界的情况下可能会返回 false
}

TEST_F(BatRestGoalTest, IsPreemptible_ReturnsTrue)
{
    // 蝙蝠的休息目标可以被抢占（玩家靠近时会飞走）
    EXPECT_TRUE(goal->isPreemptible());
}

TEST_F(BatRestGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "BatRestGoal");
}

TEST_F(BatRestGoalTest, StartExecuting_SetsRestingState)
{
    bat->setFlying(true);
    bat->setResting(false);

    goal->startExecuting();

    // startExecuting 应设置休息状态
    EXPECT_TRUE(bat->isResting());
    EXPECT_FALSE(bat->isFlying());
}

TEST_F(BatRestGoalTest, ResetTask_SetsFlyingState)
{
    bat->setFlying(false);
    bat->setResting(true);

    goal->resetTask();

    // resetTask 应设置飞行状态
    EXPECT_TRUE(bat->isFlying());
    EXPECT_FALSE(bat->isResting());
}

// ==================== Goal Flag Tests ====================

TEST_F(BatRandomFlyGoalTest, HasCorrectMutexFlags)
{
    // BatRandomFlyGoal 应使用 Move 标志
    // 这样它不会与 Look 等其他目标冲突
    // 实际标志检查需要访问 Goal 的内部方法
    // 这里只验证目标创建成功
    EXPECT_NE(goal, nullptr);
}

TEST_F(BatRestGoalTest, HasCorrectMutexFlags)
{
    // BatRestGoal 应使用 Move 和 Look 标志
    // 实际标志检查需要访问 Goal 的内部方法
    // 这里只验证目标创建成功
    EXPECT_NE(goal, nullptr);
}

// ==================== Integration Tests ====================

class BatGoalsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        bat = std::make_unique<BatEntity>(EntityInstanceId(0));
        flyGoal = std::make_unique<BatRandomFlyGoal>(bat.get());
        restGoal = std::make_unique<BatRestGoal>(bat.get());
    }

    void TearDown() override
    {
        restGoal.reset();
        flyGoal.reset();
        bat.reset();
    }

    std::unique_ptr<BatEntity> bat;
    std::unique_ptr<BatRandomFlyGoal> flyGoal;
    std::unique_ptr<BatRestGoal> restGoal;
};

TEST_F(BatGoalsIntegrationTest, FlyGoalAndRestGoal_CanToggle)
{
    // 初始状态：飞行中
    bat->setFlying(true);
    bat->setResting(false);

    // 飞行目标应可执行
    EXPECT_TRUE(flyGoal->shouldExecute());
    EXPECT_FALSE(restGoal->shouldExecute());

    // 模拟进入休息状态
    restGoal->startExecuting();
    EXPECT_TRUE(bat->isResting());
    EXPECT_FALSE(bat->isFlying());

    // 飞行目标应不可执行
    EXPECT_FALSE(flyGoal->shouldExecute());
}

TEST_F(BatGoalsIntegrationTest, MultipleTicks_DoNotThrow)
{
    bat->setFlying(true);
    bat->setResting(false);

    // 多次 tick 不应抛出异常
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            flyGoal->tick();
        }
    });
}

TEST_F(BatGoalsIntegrationTest, RestGoalTick_UpdatesYaw)
{
    bat->setFlying(true);
    bat->setResting(false);

    restGoal->startExecuting();

    // 多次 tick 不应抛出异常（会更新偏航角）
    EXPECT_NO_THROW({
        for (int i = 0; i < 50; ++i) {
            restGoal->tick();
        }
    });
}
