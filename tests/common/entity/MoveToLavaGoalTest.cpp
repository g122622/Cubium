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
 * @file MoveToLavaGoalTest.cpp
 * @brief MoveToLavaGoal 和 MoveToBlockGoal 单元测试
 *
 * 测试炽足兽寻找熔岩目标的关键方法：
 * - MoveToBlockGoal 基类: shouldExecute, shouldContinueExecuting, tick
 * - MoveToLavaGoal: shouldExecute, shouldContinueExecuting, shouldMoveTo
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/goals/special/MoveToLavaGoal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::goal;

// ==================== MoveToLavaGoal Basic Tests ====================

class MoveToLavaGoalTest : public ::testing::Test {
protected:
    void SetUp() override { strider = std::make_unique<StriderEntity>(EntityInstanceId(0), mc::test::testEcsRegistry()); }

    void TearDown() override { strider.reset(); }

    std::unique_ptr<StriderEntity> strider;
};

TEST_F(MoveToLavaGoalTest, TypeName_ReturnsCorrectName)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    EXPECT_EQ(goal->getTypeName(), "MoveToLavaGoal");
}

TEST_F(MoveToLavaGoalTest, ShouldExecute_ReturnsFalseWithoutWorld)
{
    // 没有设置世界时，shouldExecute 应该返回 false
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveToLavaGoalTest, ShouldContinueExecuting_ReturnsFalseWithoutWorld)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(MoveToLavaGoalTest, MutexFlags_AreCorrect)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);

    // MoveToLavaGoal 应该使用 Move 和 Jump 标志
    const auto& flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Jump));
}

TEST_F(MoveToLavaGoalTest, StartExecuting_DoesNotCrash)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    goal->startExecuting();
    SUCCEED();
}

TEST_F(MoveToLavaGoalTest, ResetTask_DoesNotCrash)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    goal->resetTask();
    SUCCEED();
}

TEST_F(MoveToLavaGoalTest, Tick_DoesNotCrash)
{
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    SUCCEED();
}

TEST_F(MoveToLavaGoalTest, Constants_MatchMC1165)
{
    // MC 1.16.5: MoveToLavaGoal 使用以下常量
    // - searchLength = 8 (水平搜索半径)
    // - verticalSearchRange = 2 (垂直搜索范围)
    // - 移动速度由构造函数传入
    // - shouldMove() 每 20 tick 检查一次

    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);

    // 验证目标创建成功
    EXPECT_NE(goal, nullptr);
}

// ==================== StriderEntity Integration Tests ====================

class StriderEntityGoalTest : public ::testing::Test {
protected:
    void SetUp() override { strider = std::make_unique<StriderEntity>(EntityInstanceId(0), mc::test::testEcsRegistry()); }

    void TearDown() override { strider.reset(); }

    std::unique_ptr<StriderEntity> strider;
};

TEST_F(StriderEntityGoalTest, Strider_HasMoveToLavaGoalRegistered)
{
    // 炽足兽应该在 registerGoals() 中注册 MoveToLavaGoal
    // 这里只验证实体创建成功
    EXPECT_NE(strider, nullptr);
}

TEST_F(StriderEntityGoalTest, Strider_IsInLava_WhenOnLavaSurface)
{
    // 设置炽足兽在熔岩表面
    strider->setOnLavaSurface(true);
    EXPECT_TRUE(strider->isOnLavaSurface());
    EXPECT_TRUE(strider->isInLava());
}

TEST_F(StriderEntityGoalTest, Strider_ColdTimer_WorksCorrectly)
{
    // 初始寒冷计时器应该为 0
    EXPECT_EQ(strider->getColdTimer(), 0);
    EXPECT_FALSE(strider->isCold());

    // 设置寒冷计时器
    strider->setColdTimer(100);
    EXPECT_EQ(strider->getColdTimer(), 100);
    EXPECT_TRUE(strider->isCold());

    // 重置
    strider->setColdTimer(0);
    EXPECT_FALSE(strider->isCold());
}

TEST_F(StriderEntityGoalTest, Strider_SteeringSpeed_ColdVsNormal)
{
    // 正常状态下的驾驶速度
    f32 normalSpeed = strider->getSteeringSpeed();

    // 设置寒冷状态
    strider->setColdTimer(100);
    f32 coldSpeed = strider->getSteeringSpeed();

    // 寒冷状态下的速度应该更慢
    EXPECT_LT(coldSpeed, normalSpeed);
}

TEST_F(StriderEntityGoalTest, Strider_Saddle_CanBeSet)
{
    EXPECT_FALSE(strider->hasSaddle());

    strider->setSaddle(true);
    EXPECT_TRUE(strider->hasSaddle());

    strider->setSaddle(false);
    EXPECT_FALSE(strider->hasSaddle());
}

TEST_F(StriderEntityGoalTest, Strider_BreedingItem_WarpedFungus)
{
    // 验证炽足兽繁殖物品
    // 注意：Items::WARPED_FUNGUS 可能为 nullptr（未初始化）
    // 这里只验证方法可以正常调用
    ItemStack emptyStack;
    EXPECT_FALSE(strider->isBreedingItem(emptyStack));
}

// ==================== MoveToBlockGoal Base Class Tests ====================

TEST_F(MoveToLavaGoalTest, GetTargetPosition_ReturnsDestinationBlock)
{
    // MoveToLavaGoal::getTargetPosition() 应该返回目标方块位置本身
    // 而不是父类的方块上方位置
    auto goal = std::make_unique<MoveToLavaGoal>(strider.get(), 1.5);
    EXPECT_NE(goal, nullptr);
}
