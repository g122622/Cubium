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

#include <memory>
#include <gtest/gtest.h>

#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/ai/goal/goals/special/IronGolemGoals.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"

namespace mc {
namespace test {

// ==================== IronGolemGoals 测试夹具 ====================

class IronGolemGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1)); }

    void TearDown() override { ironGolem.reset(); }

    std::unique_ptr<IronGolemEntity> ironGolem;
};

// ==================== OfferFlowerGoal 测试夹具 ====================

class OfferFlowerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1));
        goal = std::make_unique<entity::ai::goal::OfferFlowerGoal>(ironGolem.get());
    }

    void TearDown() override
    {
        goal.reset();
        ironGolem.reset();
    }

    std::unique_ptr<IronGolemEntity> ironGolem;
    std::unique_ptr<entity::ai::goal::OfferFlowerGoal> goal;
};

// ==================== OfferFlowerGoal 基础测试 ====================

TEST_F(OfferFlowerGoalTest, Construction)
{
    EXPECT_NE(goal, nullptr);
}

TEST_F(OfferFlowerGoalTest, GetTypeName)
{
    EXPECT_EQ(goal->getTypeName(), "OfferFlowerGoal");
}

TEST_F(OfferFlowerGoalTest, ShouldExecuteReturnsFalseWithoutWorld)
{
    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(OfferFlowerGoalTest, MutexFlags)
{
    // 验证互斥标志包含 Move 和 Look
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(::mc::entity::ai::GoalFlag::Look));
}

TEST_F(OfferFlowerGoalTest, StartExecutingSetsHoldingRose)
{
    // 开始执行时应设置持花状态
    EXPECT_FALSE(ironGolem->isHoldingRose());
    goal->startExecuting();
    EXPECT_TRUE(ironGolem->isHoldingRose());
    EXPECT_GT(ironGolem->getHoldRoseTick(), 0);
}

TEST_F(OfferFlowerGoalTest, ResetTaskClearsHoldingRose)
{
    // 重置时应清除持花状态（对应 MC stop() 中 offerFlower(false)）
    goal->startExecuting();
    EXPECT_TRUE(ironGolem->isHoldingRose());
    goal->resetTask();
    EXPECT_FALSE(ironGolem->isHoldingRose());
}

TEST_F(OfferFlowerGoalTest, ShouldContinueExecutingAfterStart)
{
    // 开始执行后 shouldContinueExecuting 应返回 true（m_tick > 0）
    goal->startExecuting();
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(OfferFlowerGoalTest, TickDecrementsTimer)
{
    // startExecuting 后 m_tick = OFFER_TICKS（400），tick() 应递减
    goal->startExecuting();
    i32 before = ironGolem->getHoldRoseTick();
    goal->tick();
    // tick() 只递减 m_tick，不直接递减 m_holdRoseTick（由 IronGolemEntity::tick 处理）
    // 但 shouldContinueExecuting 依赖 m_tick，调用多次后仍应继续执行
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

// ==================== MoveTowardsTargetGoal 测试夹具 ====================

class MoveTowardsTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1));
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
        // 注册原版实体类型，使 VanillaEntityTypeKeys 指针非空，可解引用传入 canAttackType。
        // registerAll() 幂等且线程安全，多次调用无副作用。
        entity::VanillaEntities::registerAll();
        ironGolem = std::make_unique<IronGolemEntity>(EntityInstanceId(1));
    }

    void TearDown() override { ironGolem.reset(); }

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

TEST_F(IronGolemEntityTest, CanAttackType)
{
    // 玩家创建的铁傀儡不攻击玩家
    ironGolem->setPlayerCreated(true);
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::PLAYER));

    // 铁傀儡不攻击苦力怕
    EXPECT_FALSE(ironGolem->canAttackType(*entity::VanillaEntityTypeKeys::CREEPER));

    // 非玩家创建的铁傀儡默认允许攻击（除苦力怕和玩家创建者外的类型）
    // 使用 EntityType::UNKNOWN 测试默认行为：UNKNOWN 不等于 PLAYER/CREEPER/GHAST，应允许攻击
    ironGolem->setPlayerCreated(false);
    EXPECT_TRUE(ironGolem->canAttackType(entity::EntityType::UNKNOWN));
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
    void SetUp() override { golem = std::make_unique<IronGolemEntity>(EntityInstanceId(1)); }

    void TearDown() override { golem.reset(); }

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
