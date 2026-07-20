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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/special/DrownedGoals.hpp"
#include "entity/core/EntityTypeIdNumber.hpp"
#include "entity/entities/monster/undead/DrownedEntity.hpp"
#include "entity/registry/VanillaEntities.hpp"

namespace mc {
namespace test {

// ==================== DrownedGoals 基础测试 ====================

class DrownedGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        entity::VanillaEntities::registerAll();
        drowned = std::make_unique<DrownedEntity>(EntityId(1));
    }

    void TearDown() override { drowned.reset(); }

    std::unique_ptr<DrownedEntity> drowned;
};

// ==================== DrownedGoToWaterGoal 测试 ====================

TEST_F(DrownedGoalsTest, DrownedGoToWaterGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedGoToWaterGoal>(drowned.get(), 1.0);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DrownedGoToWaterGoal");
}

TEST_F(DrownedGoalsTest, DrownedGoToWaterGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedGoToWaterGoal>(drowned.get(), 1.0);

    // DrownedGoToWaterGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DrownedGoalsTest, DrownedGoToWaterGoal_ShouldNotExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedGoToWaterGoal>(drowned.get(), 1.0);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== DrownedTridentAttackGoal 测试 ====================

TEST_F(DrownedGoalsTest, DrownedTridentAttackGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedTridentAttackGoal>(drowned.get(), 1.0, 40, 10.0f);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DrownedTridentAttackGoal");
}

TEST_F(DrownedGoalsTest, DrownedTridentAttackGoal_ShouldNotExecuteWithoutTrident)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedTridentAttackGoal>(drowned.get(), 1.0, 40, 10.0f);

    // 默认没有三叉戟，不应执行
    EXPECT_FALSE(drowned->hasTrident());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(DrownedGoalsTest, DrownedTridentAttackGoal_ShouldExecuteWithTrident)
{
    drowned->setHasTrident(true);
    auto goal = std::make_unique<entity::ai::goal::DrownedTridentAttackGoal>(drowned.get(), 1.0, 40, 10.0f);

    // 有三叉戟但没有世界和攻击目标，shouldExecute 应返回 false
    // (RangedAttackGoal::shouldExecute 需要攻击目标)
    EXPECT_TRUE(drowned->hasTrident());
    // 无攻击目标时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== DrownedAttackGoal 测试 ====================

TEST_F(DrownedGoalsTest, DrownedAttackGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedAttackGoal>(drowned.get(), 1.0, false);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DrownedAttackGoal");
}

TEST_F(DrownedGoalsTest, DrownedAttackGoal_ShouldNotExecuteWithoutTarget)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedAttackGoal>(drowned.get(), 1.0, false);

    // 无攻击目标时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== DrownedGoToBeachGoal 测试 ====================

TEST_F(DrownedGoalsTest, DrownedGoToBeachGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedGoToBeachGoal>(drowned.get(), 1.0);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DrownedGoToBeachGoal");
}

TEST_F(DrownedGoalsTest, DrownedGoToBeachGoal_ShouldNotExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedGoToBeachGoal>(drowned.get(), 1.0);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== DrownedSwimUpGoal 测试 ====================

TEST_F(DrownedGoalsTest, DrownedSwimUpGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedSwimUpGoal>(drowned.get(), 1.0, 63);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DrownedSwimUpGoal");
}

TEST_F(DrownedGoalsTest, DrownedSwimUpGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedSwimUpGoal>(drowned.get(), 1.0, 63);

    // DrownedSwimUpGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DrownedGoalsTest, DrownedSwimUpGoal_ShouldNotExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::DrownedSwimUpGoal>(drowned.get(), 1.0, 63);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(DrownedGoalsTest, StartAndResetManagingSearchingForLand)
{
    auto drowned = std::make_unique<DrownedEntity>(EntityId(1));
    auto goal = std::make_unique<entity::ai::goal::DrownedSwimUpGoal>(drowned.get(), 1.0, 63);

    // startExecuting 应设置 searchingForLand 为 true
    EXPECT_FALSE(drowned->isSearchingForLand());
    goal->startExecuting();
    EXPECT_TRUE(drowned->isSearchingForLand());

    // resetTask 应设置 searchingForLand 为 false
    goal->resetTask();
    EXPECT_FALSE(drowned->isSearchingForLand());
}

// ==================== DrownedEntity 辅助方法测试 ====================

TEST_F(DrownedGoalsTest, DrownedEntity_OkTarget_Nullptr)
{
    // nullptr 目标应返回 false
    EXPECT_FALSE(drowned->okTarget(nullptr));
}

TEST_F(DrownedGoalsTest, DrownedEntity_WantsToSwim_DefaultFalse)
{
    // 默认不在搜索陆地且没有攻击目标，应返回 false
    EXPECT_FALSE(drowned->isSearchingForLand());
    EXPECT_FALSE(drowned->wantsToSwim());
}

TEST_F(DrownedGoalsTest, DrownedEntity_WantsToSwim_SearchingForLand)
{
    // 搜索陆地时应返回 true
    drowned->setSearchingForLand(true);
    EXPECT_TRUE(drowned->wantsToSwim());
}

TEST_F(DrownedGoalsTest, DrownedEntity_TridentDefault)
{
    // 默认没有三叉戟
    EXPECT_FALSE(drowned->hasTrident());

    // 可以设置三叉戟
    drowned->setHasTrident(true);
    EXPECT_TRUE(drowned->hasTrident());
}

TEST_F(DrownedGoalsTest, DrownedEntity_SearchingForLand)
{
    EXPECT_FALSE(drowned->isSearchingForLand());

    drowned->setSearchingForLand(true);
    EXPECT_TRUE(drowned->isSearchingForLand());

    drowned->setSearchingForLand(false);
    EXPECT_FALSE(drowned->isSearchingForLand());
}

} // namespace test
} // namespace mc
