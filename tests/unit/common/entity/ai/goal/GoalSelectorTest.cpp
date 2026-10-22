/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/SwimGoal.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/entities/passive/basic/PigEntity.hpp"

namespace mc {
namespace test {

// ==================== 辅助测试目标类 ====================

/**
 * @brief 始终可执行的测试目标
 */
class AlwaysExecuteGoal : public entity::ai::Goal {
public:
    AlwaysExecuteGoal() { setMutexFlags(EnumSet<entity::ai::GoalFlag>{entity::ai::GoalFlag::Move}); }

    [[nodiscard]] bool shouldExecute() override { return true; }
    [[nodiscard]] bool shouldContinueExecuting() override { return true; }
    [[nodiscard]] std::string getTypeName() const override { return "AlwaysExecuteGoal"; }
};

// ==================== GoalSelector::removeGoalsOfType 测试 ====================

class GoalSelectorRemoveGoalsOfTypeTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveGoalsOfType_RemovesMatchingType)
{
    entity::ai::GoalSelector selector;

    // 添加一个 HurtByTargetGoal 和一个 LookAtGoal
    selector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get()));
    selector.addGoal(2, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));

    // 验证初始状态：2 个目标
    const auto& goals = selector.getAllGoals();
    EXPECT_EQ(goals.size(), 2u);

    // 移除所有 HurtByTargetGoal
    selector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 验证只剩 1 个目标，且是 LookAtGoal
    EXPECT_EQ(goals.size(), 1u);
    for (const auto& pg : goals) {
        EXPECT_NE(dynamic_cast<const entity::ai::goal::LookAtGoal*>(pg.getGoal()), nullptr);
    }
}

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveGoalsOfType_RemovesMultipleMatching)
{
    entity::ai::GoalSelector selector;

    // 添加两个 HurtByTargetGoal（不同优先级）和一个 LookAtGoal
    selector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get()));
    selector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get(), true));
    selector.addGoal(3, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));

    // 验证初始状态：3 个目标
    EXPECT_EQ(selector.getAllGoals().size(), 3u);

    // 移除所有 HurtByTargetGoal
    selector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 验证只剩 1 个目标
    EXPECT_EQ(selector.getAllGoals().size(), 1u);
    for (const auto& pg : selector.getAllGoals()) {
        EXPECT_NE(dynamic_cast<const entity::ai::goal::LookAtGoal*>(pg.getGoal()), nullptr);
    }
}

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveGoalsOfType_NoMatch_DoesNothing)
{
    entity::ai::GoalSelector selector;

    selector.addGoal(1, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));
    selector.addGoal(2, std::make_unique<entity::ai::goal::SwimGoal>(pig.get()));

    EXPECT_EQ(selector.getAllGoals().size(), 2u);

    // 尝试移除不存在的类型
    selector.removeGoalsOfType<entity::ai::goal::HurtByTargetGoal>();

    // 目标数量不变
    EXPECT_EQ(selector.getAllGoals().size(), 2u);
}

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveGoalsOfType_StopsRunningGoalsBeforeRemoval)
{
    entity::ai::GoalSelector selector;

    // 添加一个始终可执行的目标
    selector.addGoal(1, std::make_unique<AlwaysExecuteGoal>());

    // 设置 tickRate=1 使每次 tick 都执行目标更新
    selector.setTickRate(1);

    // tick 让目标开始运行
    selector.tick();

    // 验证目标正在运行
    bool hasRunning = false;
    selector.forEachRunningGoal([&](entity::ai::PrioritizedGoal& pg) { hasRunning = true; });
    EXPECT_TRUE(hasRunning);

    // 移除运行中的目标
    selector.removeGoalsOfType<AlwaysExecuteGoal>();

    // 验证目标已被移除
    EXPECT_EQ(selector.getAllGoals().size(), 0u);
}

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveGoalsOfType_BaseClassRemovesDerivedClass)
{
    // 测试使用基类类型是否也能移除派生类目标
    // HurtByTargetGoal 继承自 TargetGoal，使用 TargetGoal 类型移除
    entity::ai::GoalSelector selector;

    selector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get()));
    selector.addGoal(2, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));

    EXPECT_EQ(selector.getAllGoals().size(), 2u);

    // 使用基类 TargetGoal 类型移除
    selector.removeGoalsOfType<entity::ai::goal::TargetGoal>();

    // HurtByTargetGoal 也应该被移除（因为 dynamic_cast<TargetGoal*> 对 HurtByTargetGoal 返回非空）
    EXPECT_EQ(selector.getAllGoals().size(), 1u);
    for (const auto& pg : selector.getAllGoals()) {
        EXPECT_NE(dynamic_cast<const entity::ai::goal::LookAtGoal*>(pg.getGoal()), nullptr);
    }
}

TEST_F(GoalSelectorRemoveGoalsOfTypeTest, RemoveAllGoals_ClearsEverything)
{
    entity::ai::GoalSelector selector;

    selector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(pig.get()));
    selector.addGoal(2, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));
    selector.addGoal(3, std::make_unique<AlwaysExecuteGoal>());

    EXPECT_EQ(selector.getAllGoals().size(), 3u);

    selector.removeAllGoals();

    EXPECT_EQ(selector.getAllGoals().size(), 0u);
}

// ==================== GoalSelector 基础功能测试 ====================

class GoalSelectorBasicTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }

    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

TEST_F(GoalSelectorBasicTest, AddGoal_IncreasesGoalCount)
{
    entity::ai::GoalSelector selector;

    EXPECT_EQ(selector.getAllGoals().size(), 0u);

    selector.addGoal(1, std::make_unique<AlwaysExecuteGoal>());
    EXPECT_EQ(selector.getAllGoals().size(), 1u);

    selector.addGoal(2, std::make_unique<entity::ai::goal::LookAtGoal>(pig.get(), 8.0f));
    EXPECT_EQ(selector.getAllGoals().size(), 2u);
}

TEST_F(GoalSelectorBasicTest, AddGoal_DuplicateIgnored)
{
    entity::ai::GoalSelector selector;

    auto goal = std::make_unique<AlwaysExecuteGoal>();
    auto* goalPtr = goal.get();

    selector.addGoal(1, std::move(goal));
    EXPECT_EQ(selector.getAllGoals().size(), 1u);

    // 尝试用原始指针添加同一个目标（不应增加）
    selector.addGoal(2, goalPtr);
    EXPECT_EQ(selector.getAllGoals().size(), 1u);
}

TEST_F(GoalSelectorBasicTest, RemoveGoal_StopsRunningGoal)
{
    entity::ai::GoalSelector selector;

    auto goal = std::make_unique<AlwaysExecuteGoal>();
    auto* goalPtr = goal.get();

    selector.addGoal(1, std::move(goal));

    // 设置 tickRate=1 使每次 tick 都执行目标更新
    selector.setTickRate(1);
    selector.tick();

    // 目标应该正在运行
    bool hasRunning = false;
    selector.forEachRunningGoal([&](entity::ai::PrioritizedGoal& pg) { hasRunning = true; });
    EXPECT_TRUE(hasRunning);

    // 移除目标
    selector.removeGoal(goalPtr);

    // 目标列表应为空
    EXPECT_EQ(selector.getAllGoals().size(), 0u);
}

TEST_F(GoalSelectorBasicTest, HasRunningGoals_ReturnsCorrectStatus)
{
    entity::ai::GoalSelector selector;

    EXPECT_FALSE(selector.hasRunningGoals());

    selector.addGoal(1, std::make_unique<AlwaysExecuteGoal>());

    // 设置 tickRate=1 使每次 tick 都执行目标更新
    selector.setTickRate(1);
    selector.tick();

    EXPECT_TRUE(selector.hasRunningGoals());
}

// ==================== GoalSelector 半 tick 门控 / 启动停止顺序 ====================

namespace {

/**
 * @brief 计数型测试目标（可选择是否需要每 tick 更新）
 */
class CountingGoal final : public entity::ai::Goal {
public:
    explicit CountingGoal(bool everyTick)
        : m_everyTick(everyTick)
    {
        setMutexFlags(EnumSet<entity::ai::GoalFlag>{entity::ai::GoalFlag::Move});
    }

    [[nodiscard]] bool shouldExecute() override
    {
        ++canUseCalls;
        return m_canUse;
    }
    [[nodiscard]] bool shouldContinueExecuting() override { return true; }
    void tick() override { ++tickCalls; }
    void resetTask() override { ++resetCalls; }
    [[nodiscard]] bool requiresUpdateEveryTick() const override { return m_everyTick; }
    [[nodiscard]] std::string getTypeName() const override { return "CountingGoal"; }

    i32 canUseCalls = 0;
    i32 tickCalls = 0;
    i32 resetCalls = 0;
    bool m_canUse = true;

private:
    bool m_everyTick;
};

} // namespace

class GoalSelectorTickGateTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1), mc::test::testEcsRegistry()); }
    void TearDown() override { pig.reset(); }

    std::unique_ptr<PigEntity> pig;
};

// tickRunningGoalsEveryTick 只更新 requiresUpdateEveryTick() 的运行中目标。
TEST_F(GoalSelectorTickGateTest, HalfTickUpdatesOnlyEveryTickGoals)
{
    entity::ai::GoalSelector selector;
    auto perTick = std::make_unique<CountingGoal>(true);
    auto* perTickRaw = perTick.get();
    selector.addGoal(1, std::move(perTick));

    entity::ai::GoalSelector other;
    auto slow = std::make_unique<CountingGoal>(false);
    auto* slowRaw = slow.get();
    other.addGoal(1, std::move(slow));

    // 两个 selector 都先做一次完整评估让目标进入运行状态。
    // DEFAULT_TICK_RATE 为 2：CountingGoal 的 shouldExecute 不读取随机数、不过 adjustedTickDelay，
    // 因此每次低频评估都会重新通过，这里把 tickRate 设为 1 让首次 tick 即完成评估。
    selector.setTickRate(1);
    other.setTickRate(1);
    selector.tick();
    other.tick();
    ASSERT_TRUE(selector.getAllGoals().front().isRunning());
    ASSERT_TRUE(other.getAllGoals().front().isRunning());

    perTickRaw->tickCalls = 0;
    slowRaw->tickCalls = 0;
    selector.tickRunningGoalsEveryTick();
    other.tickRunningGoalsEveryTick();

    EXPECT_EQ(perTickRaw->tickCalls, 1) << "requiresUpdateEveryTick 的 goal 应在半 tick 帧 tick";
    EXPECT_EQ(slowRaw->tickCalls, 0) << "普通 goal 在半 tick 帧不应 tick";
    // 半 tick 帧不做 canUse 评估。
    EXPECT_EQ(perTickRaw->canUseCalls, 1);
    EXPECT_EQ(slowRaw->canUseCalls, 1);
}

// 启动后 canUse 评估先于 startExecuting：vanilla 在 canUse 通过后才 start()，
// 因此启动帧的 tick() 必须在 startExecuting 之后、下一次完整评估之前发生。
TEST_F(GoalSelectorTickGateTest, StartedGoalTicksInSameFullEvaluation)
{
    entity::ai::GoalSelector selector;
    auto goal = std::make_unique<CountingGoal>(false);
    auto* raw = goal.get();
    selector.addGoal(1, std::move(goal));
    selector.setTickRate(1);

    selector.tick();
    EXPECT_EQ(raw->canUseCalls, 1);
    EXPECT_EQ(raw->tickCalls, 1) << "完整评估帧在启动后应立即 tick 一次";
    EXPECT_TRUE(selector.getAllGoals().front().isRunning());
}

// 被更高优先级同标志目标抢占时，低优先级目标应被 stop（resetTask 恰好一次）。
TEST_F(GoalSelectorTickGateTest, PreemptionStopsLowerPriorityOnce)
{
    entity::ai::GoalSelector selector;
    auto low = std::make_unique<CountingGoal>(false);
    auto* lowRaw = low.get();
    selector.addGoal(5, std::move(low));
    selector.tick();
    ASSERT_TRUE(selector.getAllGoals().front().isRunning());

    auto high = std::make_unique<CountingGoal>(false);
    auto* highRaw = high.get();
    selector.addGoal(1, std::move(high));
    selector.tick();

    EXPECT_EQ(lowRaw->resetCalls, 1) << "低优先级目标被抢占时应停止一次";
    EXPECT_EQ(highRaw->canUseCalls, 1);
    i32 running = 0;
    selector.forEachRunningGoal([&running](entity::ai::PrioritizedGoal&) { ++running; });
    EXPECT_EQ(running, 1) << "同一时刻同标志只允许一个目标运行";
}

} // namespace test
} // namespace mc
