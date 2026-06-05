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

#include <gtest/gtest.h>

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/PrioritizedGoal.hpp"

using namespace mc;
using namespace mc::entity::ai;

// ============================================================================
// EnumSet 测试
// ============================================================================

class TestEnum {
public:
    enum Value : u8 { A = 0, B = 1, C = 2, Count = 3 };
};

TEST(EnumSetTest, DefaultConstruction)
{
    EnumSet<TestEnum::Value> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.count(), 0);
}

TEST(EnumSetTest, InitializerListConstruction)
{
    EnumSet<TestEnum::Value> set{TestEnum::A, TestEnum::C};
    EXPECT_FALSE(set.empty());
    EXPECT_EQ(set.count(), 2);
    EXPECT_TRUE(set.test(TestEnum::A));
    EXPECT_FALSE(set.test(TestEnum::B));
    EXPECT_TRUE(set.test(TestEnum::C));
}

TEST(EnumSetTest, SetAndReset)
{
    EnumSet<TestEnum::Value> set;
    set.set(TestEnum::A);
    EXPECT_TRUE(set.test(TestEnum::A));
    EXPECT_FALSE(set.test(TestEnum::B));

    set.reset(TestEnum::A);
    EXPECT_FALSE(set.test(TestEnum::A));
}

TEST(EnumSetTest, Operators)
{
    EnumSet<TestEnum::Value> a{TestEnum::A, TestEnum::B};
    EnumSet<TestEnum::Value> b{TestEnum::B, TestEnum::C};

    // 并集
    auto union_ = a | b;
    EXPECT_TRUE(union_.test(TestEnum::A));
    EXPECT_TRUE(union_.test(TestEnum::B));
    EXPECT_TRUE(union_.test(TestEnum::C));

    // 交集
    auto intersect = a & b;
    EXPECT_FALSE(intersect.test(TestEnum::A));
    EXPECT_TRUE(intersect.test(TestEnum::B));
    EXPECT_FALSE(intersect.test(TestEnum::C));

    // 差集
    auto diff = a - b;
    EXPECT_TRUE(diff.test(TestEnum::A));
    EXPECT_FALSE(diff.test(TestEnum::B));
}

TEST(EnumSetTest, Intersects)
{
    EnumSet<TestEnum::Value> a{TestEnum::A, TestEnum::B};
    EnumSet<TestEnum::Value> b{TestEnum::B, TestEnum::C};
    EnumSet<TestEnum::Value> c{TestEnum::C};

    EXPECT_TRUE(a.intersects(b));
    EXPECT_FALSE(a.intersects(c));
}

TEST(EnumSetTest, ForEach)
{
    EnumSet<TestEnum::Value> set{TestEnum::A, TestEnum::C};
    std::vector<TestEnum::Value> values;
    set.forEach([&values](TestEnum::Value v) { values.push_back(v); });

    ASSERT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], TestEnum::A);
    EXPECT_EQ(values[1], TestEnum::C);
}

// ============================================================================
// 测试目标类
// ============================================================================

class TestGoal : public Goal {
public:
    TestGoal()
        : m_shouldExecute(false)
        , m_shouldContinue(false)
        , m_startCount(0)
        , m_resetCount(0)
        , m_tickCount(0)
    {}

    explicit TestGoal(const EnumSet<GoalFlag>& flags)
        : Goal(flags)
        , m_shouldExecute(false)
        , m_shouldContinue(false)
        , m_startCount(0)
        , m_resetCount(0)
        , m_tickCount(0)
    {}

    [[nodiscard]] bool shouldExecute() override { return m_shouldExecute; }

    [[nodiscard]] bool shouldContinueExecuting() override { return m_shouldContinue; }

    void startExecuting() override { m_startCount++; }

    void resetTask() override { m_resetCount++; }

    void tick() override { m_tickCount++; }

    [[nodiscard]] std::string getTypeName() const override { return "TestGoal"; }

    void setShouldExecute(bool value) { m_shouldExecute = value; }
    void setShouldContinue(bool value) { m_shouldContinue = value; }
    [[nodiscard]] int getStartCount() const { return m_startCount; }
    [[nodiscard]] int getResetCount() const { return m_resetCount; }
    [[nodiscard]] int getTickCount() const { return m_tickCount; }

private:
    bool m_shouldExecute;
    bool m_shouldContinue;
    int m_startCount;
    int m_resetCount;
    int m_tickCount;
};

// ============================================================================
// Goal 测试
// ============================================================================

TEST(GoalTest, DefaultFlags)
{
    TestGoal goal;
    EXPECT_TRUE(goal.getMutexFlags().empty());
}

TEST(GoalTest, SetMutexFlags)
{
    TestGoal goal;
    goal.setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});

    EXPECT_TRUE(goal.getMutexFlags().test(GoalFlag::Move));
    EXPECT_TRUE(goal.getMutexFlags().test(GoalFlag::Look));
    EXPECT_FALSE(goal.getMutexFlags().test(GoalFlag::Jump));
}

TEST(GoalTest, ConstructWithFlags)
{
    TestGoal goal(EnumSet<GoalFlag>{GoalFlag::Target});
    EXPECT_TRUE(goal.getMutexFlags().test(GoalFlag::Target));
}

TEST(GoalTest, DefaultPreemptible)
{
    TestGoal goal;
    EXPECT_TRUE(goal.isPreemptible());
}

// ============================================================================
// PrioritizedGoal 测试
// ============================================================================

TEST(PrioritizedGoalTest, Priority)
{
    auto innerGoal = std::make_unique<TestGoal>();
    TestGoal* innerPtr = innerGoal.get();

    PrioritizedGoal prioritized(5, std::move(innerGoal));
    EXPECT_EQ(prioritized.getPriority(), 5);
    EXPECT_EQ(prioritized.getGoal(), innerPtr);
}

TEST(PrioritizedGoalTest, Preemption)
{
    auto lowGoal = std::make_unique<TestGoal>();
    auto highGoal = std::make_unique<TestGoal>();

    lowGoal->setShouldExecute(true);
    highGoal->setShouldExecute(true);

    PrioritizedGoal low(10, std::move(lowGoal));
    PrioritizedGoal high(5, std::move(highGoal));

    // 默认可抢占，高优先级可以抢占低优先级
    EXPECT_TRUE(low.isPreemptedBy(high));
    EXPECT_FALSE(high.isPreemptedBy(low));
}

TEST(PrioritizedGoalTest, NonPreemptible)
{
    class NonPreemptibleGoal : public Goal {
    public:
        [[nodiscard]] bool shouldExecute() override { return true; }
        [[nodiscard]] bool isPreemptible() const override { return false; }
    };

    auto lowGoal = std::make_unique<NonPreemptibleGoal>();
    auto highGoal = std::make_unique<TestGoal>();
    highGoal->setShouldExecute(true);

    PrioritizedGoal low(10, std::move(lowGoal));
    PrioritizedGoal high(5, std::move(highGoal));

    // 不可抢占的目标不能被抢占
    EXPECT_FALSE(low.isPreemptedBy(high));
}

TEST(PrioritizedGoalTest, RunningState)
{
    auto innerGoal = std::make_unique<TestGoal>();
    TestGoal* innerPtr = innerGoal.get();
    innerGoal->setShouldExecute(true);

    PrioritizedGoal prioritized(5, std::move(innerGoal));

    EXPECT_FALSE(prioritized.isRunning());

    prioritized.startExecuting();
    EXPECT_TRUE(prioritized.isRunning());
    EXPECT_EQ(innerPtr->getStartCount(), 1);

    // 重复调用 startExecuting 不会多次启动
    prioritized.startExecuting();
    EXPECT_EQ(innerPtr->getStartCount(), 1);

    prioritized.resetTask();
    EXPECT_FALSE(prioritized.isRunning());
    EXPECT_EQ(innerPtr->getResetCount(), 1);
}

TEST(PrioritizedGoalTest, DelegatesToInner)
{
    auto innerGoal = std::make_unique<TestGoal>();
    TestGoal* innerPtr = innerGoal.get();
    innerPtr->setShouldExecute(true);
    innerPtr->setShouldContinue(true);

    PrioritizedGoal prioritized(5, std::move(innerGoal));

    EXPECT_TRUE(prioritized.shouldExecute());
    EXPECT_TRUE(prioritized.shouldContinueExecuting());

    prioritized.setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
    EXPECT_TRUE(prioritized.getMutexFlags().test(GoalFlag::Move));

    prioritized.tick();
    EXPECT_EQ(innerPtr->getTickCount(), 1);
}

// ============================================================================
// GoalSelector 测试
// ============================================================================

TEST(GoalSelectorTest, AddAndRemoveGoal)
{
    GoalSelector selector;

    auto goal = new TestGoal();
    selector.addGoal(5, goal);

    EXPECT_EQ(selector.getAllGoals().size(), 1);

    selector.removeGoal(goal);
    EXPECT_EQ(selector.getAllGoals().size(), 0);
}

TEST(GoalSelectorTest, PriorityOrdering)
{
    GoalSelector selector;

    auto lowGoal = new TestGoal();
    auto highGoal = new TestGoal();

    lowGoal->setShouldExecute(true);
    lowGoal->setShouldContinue(true);
    highGoal->setShouldExecute(true);
    highGoal->setShouldContinue(true);

    // 添加低优先级目标（数值大=优先级低）
    selector.addGoal(10, lowGoal);
    // 添加高优先级目标（数值小=优先级高）
    selector.addGoal(5, highGoal);

    // 两个目标都应该存在
    EXPECT_EQ(selector.getAllGoals().size(), 2);
}

TEST(GoalSelectorTest, MutexFlags)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal1 = new TestGoal(EnumSet<GoalFlag>{GoalFlag::Move});
    auto goal2 = new TestGoal(EnumSet<GoalFlag>{GoalFlag::Move});

    goal1->setShouldExecute(true);
    goal1->setShouldContinue(true);
    goal2->setShouldExecute(true);
    goal2->setShouldContinue(true);

    selector.addGoal(5, goal1);
    selector.addGoal(10, goal2);

    // 第一次 tick 应该启动 goal1（高优先级）
    selector.tick();
    EXPECT_TRUE(selector.hasRunningGoals());
    EXPECT_EQ(goal1->getStartCount(), 1);
    EXPECT_EQ(goal2->getStartCount(), 0);
}

TEST(GoalSelectorTest, DisableFlags)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal = new TestGoal(EnumSet<GoalFlag>{GoalFlag::Move});
    goal->setShouldExecute(true);
    goal->setShouldContinue(true);

    selector.addGoal(5, goal);

    // 禁用 Move 标志
    selector.disableFlag(GoalFlag::Move);
    EXPECT_TRUE(selector.isFlagDisabled(GoalFlag::Move));

    // 目标不应该启动
    selector.tick();
    EXPECT_EQ(goal->getStartCount(), 0);

    // 启用 Move 标志
    selector.enableFlag(GoalFlag::Move);
    EXPECT_FALSE(selector.isFlagDisabled(GoalFlag::Move));

    // 现在目标应该可以启动
    selector.tick();
    EXPECT_EQ(goal->getStartCount(), 1);
}

TEST(GoalSelectorTest, TickRunningGoals)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal = new TestGoal();
    goal->setShouldExecute(true);
    goal->setShouldContinue(true);

    selector.addGoal(5, goal);

    selector.tick();
    EXPECT_EQ(goal->getTickCount(), 1);

    selector.tick();
    EXPECT_EQ(goal->getTickCount(), 2);
}

TEST(GoalSelectorTest, StopWhenShouldNotContinue)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal = new TestGoal();
    goal->setShouldExecute(true);
    goal->setShouldContinue(true);

    selector.addGoal(5, goal);

    // 启动目标
    selector.tick();
    EXPECT_EQ(goal->getStartCount(), 1);
    EXPECT_EQ(goal->getResetCount(), 0);

    // 目标不应该继续
    goal->setShouldContinue(false);

    // 下一次 tick 应该停止目标
    selector.tick();
    EXPECT_EQ(goal->getResetCount(), 1);
}

TEST(GoalSelectorTest, RemoveAllGoals)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal1 = new TestGoal();
    auto goal2 = new TestGoal();

    goal1->setShouldExecute(true);
    goal2->setShouldExecute(true);

    selector.addGoal(5, goal1);
    selector.addGoal(10, goal2);

    selector.tick();
    EXPECT_TRUE(selector.hasRunningGoals());

    selector.removeAllGoals();
    EXPECT_EQ(selector.getAllGoals().size(), 0);
    EXPECT_FALSE(selector.hasRunningGoals());
}

TEST(GoalSelectorTest, ForEachRunningGoal)
{
    GoalSelector selector;
    selector.setTickRate(1); // 设置 tickRate 为 1 以便测试

    auto goal1 = new TestGoal(EnumSet<GoalFlag>{GoalFlag::Move});
    auto goal2 = new TestGoal(EnumSet<GoalFlag>{GoalFlag::Look});

    goal1->setShouldExecute(true);
    goal1->setShouldContinue(true);
    goal2->setShouldExecute(true);
    goal2->setShouldContinue(true);

    selector.addGoal(5, goal1);
    selector.addGoal(5, goal2);

    selector.tick();

    int count = 0;
    selector.forEachRunningGoal([&count](PrioritizedGoal& /*goal*/) { count++; });

    EXPECT_EQ(count, 2);
}

// ============================================================================
// GoalFlag 测试
// ============================================================================

TEST(GoalFlagTest, AllFlags)
{
    auto all = allGoalFlags();

    EXPECT_TRUE(all.test(GoalFlag::Move));
    EXPECT_TRUE(all.test(GoalFlag::Look));
    EXPECT_TRUE(all.test(GoalFlag::Jump));
    EXPECT_TRUE(all.test(GoalFlag::Target));
    EXPECT_EQ(all.count(), 4);
}

// ============================================================================
// TargetPredicate 类型测试
// ============================================================================

#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"

using namespace mc::entity::ai::goal;

TEST(TargetPredicateTest, CanBeCreatedFromLambda)
{
    // 测试 TargetPredicate 可以从 lambda 创建
    TargetPredicate pred = [](const LivingEntity* entity) -> bool { return entity != nullptr; };

    // 验证谓词可以调用
    EXPECT_TRUE(pred(nullptr) == false);
}

TEST(TargetPredicateTest, CanBeUsedForConditionChecking)
{
    // 测试用于条件检查的谓词
    int callCount = 0;
    TargetPredicate countingPred = [&callCount](const LivingEntity* /*entity*/) -> bool {
        callCount++;
        return true;
    };

    // 验证谓词可以创建和存储
    EXPECT_TRUE(static_cast<bool>(countingPred));
}

TEST(TargetPredicateTest, CanBeEmpty)
{
    // 测试空谓词
    TargetPredicate emptyPred;

    // 空谓词应该可以安全地存储和检查
    EXPECT_FALSE(static_cast<bool>(emptyPred));
}

TEST(TargetPredicateTest, CanCaptureExternalState)
{
    // 测试谓词可以捕获外部状态
    bool flag = false;
    TargetPredicate statefulPred = [&flag](const LivingEntity* /*entity*/) -> bool {
        flag = true;
        return flag;
    };

    EXPECT_TRUE(statefulPred(nullptr));
    EXPECT_TRUE(flag);
}
