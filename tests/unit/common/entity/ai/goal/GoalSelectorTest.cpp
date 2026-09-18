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

} // namespace test
} // namespace mc
