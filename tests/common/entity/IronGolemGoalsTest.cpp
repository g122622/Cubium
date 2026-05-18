/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction restriction, including without limitation the rights
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
#include <memory>

#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"

namespace mc {
namespace test {

// ==================== IronGolemGoals 测试夹具 ====================

class IronGolemGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityId(1));
    }

    void TearDown() override
    {
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

// ==================== ShowVillagerFlowerGoal 测试夹具 ====================

class ShowVillagerFlowerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityId(1));
        goal = std::make_unique<entity::ai::goal::ShowVillagerFlowerGoal>(ironGolem.get());
    }

    void TearDown() override
    {
        goal.reset();
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
    std::unique_ptr<entity::ai::goal::ShowVillagerFlowerGoal> goal;
};

// ==================== ShowVillagerFlowerGoal 基础测试 ====================

TEST_F(ShowVillagerFlowerGoalTest, Construction)
{
    EXPECT_NE(goal, nullptr);
}

TEST_F(ShowVillagerFlowerGoalTest, GetTypeName)
{
    EXPECT_EQ(goal->getTypeName(), "ShowVillagerFlowerGoal");
}

TEST_F(ShowVillagerFlowerGoalTest, ShouldExecuteReturnsFalseWithoutWorld)
{
    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(ShowVillagerFlowerGoalTest, MutexFlags)
{
    // 验证互斥标志包含 Move 和 Look
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Look));
}

TEST_F(ShowVillagerFlowerGoalTest, StartExecutingSetsLookTime)
{
    // 开始执行时应设置看向时间
    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(ShowVillagerFlowerGoalTest, ResetTaskClearsVillager)
{
    // 重置时应清除村民引用
    goal->resetTask();
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

// ==================== MoveTowardsTargetGoal 测试夹具 ====================

class MoveTowardsTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityId(1));
        goal = std::make_unique<entity::ai::goal::MoveTowardsTargetGoal>(ironGolem.get(), 0.9, 32.0f);
    }

    void TearDown() override
    {
        goal.reset();
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
    std::unique_ptr<entity::ai::goal::MoveTowardsTargetGoal> goal;
};

TEST_F(MoveTowardsTargetGoalTest, Construction)
{
    EXPECT_NE(goal, nullptr);
}

TEST_F(MoveTowardsTargetGoalTest, GetTypeName)
{
    EXPECT_EQ(goal->getTypeName(), "MoveTowardsTargetGoal");
}

TEST_F(MoveTowardsTargetGoalTest, ShouldExecuteReturnsFalseWithoutTarget)
{
    // 无目标时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveTowardsTargetGoalTest, MutexFlags)
{
    // 验证互斥标志包含 Move
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Move));
}

// ==================== IronGolemEntity 集成测试 ====================

class IronGolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityId(1));
    }

    void TearDown() override
    {
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

TEST_F(IronGolemEntityTest, Construction)
{
    EXPECT_NE(ironGolem, nullptr);
    EXPECT_FALSE(ironGolem->isArmsRaised());
    EXPECT_FALSE(ironGolem->isHoldingRose());
    EXPECT_FALSE(ironGolem->isPlayerCreated());
}

TEST_F(IronGolemEntityTest, ArmsRaisedState)
{
    EXPECT_FALSE(ironGolem->isArmsRaised());
    ironGolem->setArmsRaised(true);
    EXPECT_TRUE(ironGolem->isArmsRaised());
    ironGolem->setArmsRaised(false);
    EXPECT_FALSE(ironGolem->isArmsRaised());
}

TEST_F(IronGolemEntityTest, HoldingRoseState)
{
    EXPECT_FALSE(ironGolem->isHoldingRose());
    ironGolem->setHoldingRose(true);
    EXPECT_TRUE(ironGolem->isHoldingRose());
    EXPECT_GT(ironGolem->getHoldRoseTick(), 0);
    ironGolem->setHoldingRose(false);
    EXPECT_FALSE(ironGolem->isHoldingRose());
    EXPECT_EQ(ironGolem->getHoldRoseTick(), 0);
}

TEST_F(IronGolemEntityTest, PlayerCreatedState)
{
    EXPECT_FALSE(ironGolem->isPlayerCreated());
    ironGolem->setPlayerCreated(true);
    EXPECT_TRUE(ironGolem->isPlayerCreated());
}

TEST_F(IronGolemEntityTest, CanAttackEntity)
{
    // 玩家创建的铁傀儡不攻击玩家
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::PLAYER));

    // 铁傀儡不攻击苦力怕
    EXPECT_FALSE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::CREEPER));

    // 可以攻击其他实体
    EXPECT_TRUE(ironGolem->canAttackEntity(entity::EntityTypeIdNumber::ZOMBIE));
}

TEST_F(IronGolemEntityTest, IAngerableInterface)
{
    // 测试愤怒接口
    EXPECT_FALSE(ironGolem->isAngry());
    ironGolem->setAngry(true);
    EXPECT_TRUE(ironGolem->isAngry());
    EXPECT_GT(ironGolem->getAngerTime(), 0);

    ironGolem->setAngry(false);
    EXPECT_FALSE(ironGolem->isAngry());
    EXPECT_EQ(ironGolem->getAngerTime(), 0);
}

TEST_F(IronGolemEntityTest, EyeHeight)
{
    EXPECT_FLOAT_EQ(ironGolem->eyeHeight(), 2.1f);
}

TEST_F(IronGolemEntityTest, Dimensions)
{
    EXPECT_FLOAT_EQ(ironGolem->width(), 1.4f);
    EXPECT_FLOAT_EQ(ironGolem->height(), 2.7f);
}

// ==================== GolemEntity 测试 ====================

class GolemEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        golem = std::make_unique<IronGolemEntity>(EntityId(1));
    }

    void TearDown() override
    {
        golem.reset();
    }

    std::unique_ptr<IronGolemEntity> golem;
};

TEST_F(GolemEntityTest, IAngerableGetRevengeTarget)
{
    // 无复仇目标时返回 nullptr
    EXPECT_EQ(golem->getRevengeTarget(), nullptr);
    EXPECT_EQ(golem->getRevengeTimer(), 0);
}

TEST_F(GolemEntityTest, IAngerableSetRevengeTarget)
{
    // 设置复仇目标（但目标实体不在世界中，所以 getRevengeTarget 仍返回 nullptr）
    // 这里只测试设置不崩溃
    golem->setRevengeTarget(nullptr);
    EXPECT_EQ(golem->getRevengeTarget(), nullptr);
}

} // namespace test
} // namespace mc
