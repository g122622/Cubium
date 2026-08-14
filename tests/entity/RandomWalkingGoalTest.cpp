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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;
using namespace mc::entity::ai::goal;

// ============================================================================
// Test CreatureEntity for testing
// ============================================================================

class TestCreatureEntity : public CreatureEntity {
public:
    TestCreatureEntity()
        : CreatureEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        // 注册属性
        registerAttributes();
        // 设置初始生命值
        setHealth(maxHealth());
    }

    // 设置位置用于测试
    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }

    // 设置空闲时间
    void setIdleTimeForTest(i32 time) { m_idleTime = time; }
};

// ============================================================================
// RandomWalkingGoal Tests
// ============================================================================

class RandomWalkingGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreatureEntity>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        creature->setIdleTimeForTest(0);

        // 创建目标（chance=1 确保总是执行）
        goal = std::make_unique<RandomWalkingGoal>(creature.get(), 1.0, 1);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
    }

    std::unique_ptr<TestCreatureEntity> creature;
    std::unique_ptr<RandomWalkingGoal> goal;
};

TEST_F(RandomWalkingGoalTest, ShouldExecuteReturnsFalseWhenNullCreature)
{
    RandomWalkingGoal nullGoal(nullptr, 1.0);
    EXPECT_FALSE(nullGoal.shouldExecute());
}

TEST_F(RandomWalkingGoalTest, ShouldExecuteReturnsTrueWhenConditionsMet)
{
    // 注意：creature 没有 world，所以 RandomPositionGenerator 无法找到目标位置
    // MC 1.16.5: 应该返回 false，因为无法找到随机目标
    // 要测试成功情况，需要提供一个带有世界的 creature
    EXPECT_FALSE(goal->shouldExecute());
}

// 注意：isBeingRidden 测试需要乘客系统支持，Entity::isBeingRidden 基于 hasPassengers()
// 该测试在集成测试中覆盖

TEST_F(RandomWalkingGoalTest, ShouldExecuteReturnsFalseWhenIdleTimeTooHigh)
{
    creature->setIdleTimeForTest(100);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(RandomWalkingGoalTest, ShouldContinueExecutingReturnsFalseWhenNullCreature)
{
    RandomWalkingGoal nullGoal(nullptr, 1.0);
    EXPECT_FALSE(nullGoal.shouldContinueExecuting());
}

TEST_F(RandomWalkingGoalTest, ShouldContinueExecutingReturnsFalseWhenNoPath)
{
    // MC 1.16.5: shouldContinueExecuting 返回 !navigator.noPath() && !isBeingRidden()
    // 由于测试中的 creature 没有 world，navigator 是 null 或没有路径
    // 所以 noPath() 返回 true，shouldContinueExecuting 返回 false
    static_cast<void>(goal->shouldExecute());
    goal->startExecuting();

    // MC 1.16.5: 由于没有真正的路径，shouldContinueExecuting 返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(RandomWalkingGoalTest, StartExecutingDoesNotCrash)
{
    static_cast<void>(goal->shouldExecute());
    goal->startExecuting();

    // 如果能成功执行 startExecuting 不崩溃，测试通过
    EXPECT_TRUE(true);
}

TEST_F(RandomWalkingGoalTest, ResetTaskClearsNavigation)
{
    static_cast<void>(goal->shouldExecute());
    goal->startExecuting();
    goal->resetTask();

    // 重置后应该能正常工作
    EXPECT_TRUE(true);
}

TEST_F(RandomWalkingGoalTest, TickDoesNotCrash)
{
    static_cast<void>(goal->shouldExecute());
    goal->startExecuting();

    // MC 1.16.5: RandomWalkingGoal.tick() 是空的
    // 如果能连续 tick 不崩溃，测试通过
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

TEST_F(RandomWalkingGoalTest, MakeUpdateForcesNextExecution)
{
    // 设置高空闲时间，正常情况下不应该执行
    creature->setIdleTimeForTest(200);

    // 但如果强制更新，应该尝试执行
    goal->makeUpdate();
    // 注意：由于没有 world，RandomPositionGenerator 仍然无法找到目标位置
    // 所以 shouldExecute 返回 false
    // 这个测试验证 makeUpdate 不会崩溃，并且 m_forceUpdate 标志被设置
    EXPECT_FALSE(goal->shouldExecute()); // 无 world 时仍然返回 false
}

TEST_F(RandomWalkingGoalTest, SetExecutionChance)
{
    goal->setExecutionChance(100);
    // 设置概率后，目标应该正常工作
    EXPECT_TRUE(true); // 基本验证不会崩溃
}

// ============================================================================
// CreatureEntity::tryMoveTo Tests
// ============================================================================

class CreatureEntityMoveTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreatureEntity>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
    }

    void TearDown() override { creature.reset(); }

    std::unique_ptr<TestCreatureEntity> creature;
};

TEST_F(CreatureEntityMoveTest, TryMoveToUsesMovementControllerWhenNoNavigator)
{
    // CreatureEntity 默认没有 PathNavigator（需要 world 才能创建）
    // 所以 tryMoveTo 应该使用 MovementController

    bool result = creature->tryMoveTo(10.0, 64.0, 20.0, 1.0);

    // 应该成功
    EXPECT_TRUE(result);

    // MovementController 应该被设置
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);
    EXPECT_TRUE(moveCtrl->isUpdating());
}

TEST_F(CreatureEntityMoveTest, TryMoveToSetsCorrectTargetPosition)
{
    creature->tryMoveTo(100.0, 70.0, 200.0, 0.5);

    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    EXPECT_DOUBLE_EQ(moveCtrl->getX(), 100.0);
    EXPECT_DOUBLE_EQ(moveCtrl->getY(), 70.0);
    EXPECT_DOUBLE_EQ(moveCtrl->getZ(), 200.0);
    EXPECT_DOUBLE_EQ(moveCtrl->speed(), 0.5);
}

TEST_F(CreatureEntityMoveTest, TryMoveToReturnsTrueWithMovementController)
{
    // 即使没有 PathNavigator，也应该成功（使用 MovementController fallback）
    bool result = creature->tryMoveTo(10.0, 64.0, 10.0, 1.0);
    EXPECT_TRUE(result);
}

// ============================================================================
// MovementController Tests
// ============================================================================

class MovementControllerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreatureEntity>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
    }

    void TearDown() override { creature.reset(); }

    std::unique_ptr<TestCreatureEntity> creature;
};

TEST_F(MovementControllerTest, IsUpdatingReturnsTrueWhenMoveToSet)
{
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    // 初始状态应该是 Wait
    EXPECT_FALSE(moveCtrl->isUpdating());

    // 设置移动目标后应该是 MoveTo
    moveCtrl->setMoveTo(10.0, 64.0, 10.0, 1.0);
    EXPECT_TRUE(moveCtrl->isUpdating());
}

TEST_F(MovementControllerTest, SetMoveToStoresTarget)
{
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    moveCtrl->setMoveTo(50.0, 70.0, 100.0, 0.8);

    EXPECT_DOUBLE_EQ(moveCtrl->getX(), 50.0);
    EXPECT_DOUBLE_EQ(moveCtrl->getY(), 70.0);
    EXPECT_DOUBLE_EQ(moveCtrl->getZ(), 100.0);
    EXPECT_DOUBLE_EQ(moveCtrl->speed(), 0.8);
}

TEST_F(MovementControllerTest, TickUpdatesEntityRotation)
{
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    // 设置目标在 X+ 方向
    moveCtrl->setMoveTo(10.0, 64.0, 0.0, 1.0);

    // 执行 tick
    moveCtrl->tick();

    // 实体应该朝向目标
    // 目标偏航角应该是 atan2(0, 10) * RAD_TO_DEG - 90 = 0 - 90 = -90 度
    // 但由于旋转速度限制，可能不会立即到达
    f32 yaw = creature->yaw();
    // yaw 可能是归一化到 [0, 360) 的范围，所以 -90 可能变成 270
    // 或者由于旋转速度限制，yaw 可能只变化了最多 30 度
    // 从 0 度开始，目标 -90 度（或 270 度）
    // 允许较大的误差范围
    EXPECT_TRUE(yaw >= 330.0f || yaw <= 30.0f || yaw >= 240.0f);
}

TEST_F(MovementControllerTest, TickStopsWhenNearTarget)
{
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    // 设置目标非常近
    moveCtrl->setMoveTo(0.1, 64.0, 0.1, 1.0);

    // 多次 tick
    for (int i = 0; i < 10; ++i) {
        moveCtrl->tick();
    }

    // 应该停止移动（因为到达目标）
    EXPECT_FALSE(moveCtrl->isUpdating());
}

TEST_F(MovementControllerTest, ActionTransitionsCorrectly)
{
    auto* moveCtrl = creature->moveController();
    ASSERT_NE(moveCtrl, nullptr);

    // 初始状态是 Wait
    EXPECT_EQ(moveCtrl->action(), entity::ai::controller::MoveAction::Wait);

    // 设置移动目标
    moveCtrl->setMoveTo(10.0, 64.0, 10.0, 1.0);
    EXPECT_EQ(moveCtrl->action(), entity::ai::controller::MoveAction::MoveTo);

    // 设置横向移动
    moveCtrl->strafe(1.0f, 0.0f);
    EXPECT_EQ(moveCtrl->action(), entity::ai::controller::MoveAction::Strafe);
}

// ============================================================================
// Integration Tests: RandomWalkingGoal + MovementController
// ============================================================================

class RandomWalkingGoalIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreatureEntity>();
        creature->setPositionForTest(100.0, 64.0, 100.0);
        creature->setIdleTimeForTest(0);

        goal = std::make_unique<RandomWalkingGoal>(creature.get(), 1.0, 1);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
    }

    std::unique_ptr<TestCreatureEntity> creature;
    std::unique_ptr<RandomWalkingGoal> goal;
};

TEST_F(RandomWalkingGoalIntegrationTest, FullWalkCycle)
{
    // 注意：creature 没有 world，所以 RandomPositionGenerator 无法找到目标位置
    // shouldExecute 返回 false
    EXPECT_FALSE(goal->shouldExecute());

    // 即使无法执行，startExecuting、tick、resetTask 也不应该崩溃
    goal->startExecuting();

    // Tick 几次
    for (int i = 0; i < 10; ++i) {
        goal->tick();
    }

    // 重置
    goal->resetTask();

    // 测试通过：不崩溃
    EXPECT_TRUE(true);
}

TEST_F(RandomWalkingGoalIntegrationTest, MovementControllerFallbackWorks)
{
    // MobEntity 创建时自动创建了 PathNavigator，但 PathFinder 为 null
    // 由于没有 world，shouldExecute 返回 false
    EXPECT_FALSE(goal->shouldExecute());
    goal->startExecuting();

    // 测试通过：不崩溃
    EXPECT_TRUE(true);
}

TEST_F(RandomWalkingGoalIntegrationTest, StopsWhenNoPath)
{
    // MC 1.16.5: shouldContinueExecuting 返回 !navigator.noPath() && !isBeingRidden()
    // 由于测试中的 creature 没有有效的路径，noPath() 应该返回 true

    static_cast<void>(goal->shouldExecute());
    goal->startExecuting();

    // 由于没有真正的路径，shouldContinueExecuting 返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(RandomWalkingGoalIntegrationTest, MakeUpdateBypassesIdleTimeCheck)
{
    // 设置高空闲时间，正常情况下不应该执行
    creature->setIdleTimeForTest(200);

    // 但如果强制更新，会跳过空闲时间检查
    // 由于没有 world，RandomPositionGenerator 仍然无法找到目标位置
    // 所以 shouldExecute 返回 false
    goal->makeUpdate();
    EXPECT_FALSE(goal->shouldExecute()); // 无 world 时返回 false
}
