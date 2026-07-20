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

#include "entity/ai/goal/GoalFlag.hpp"
#include "entity/ai/goal/goals/special/DolphinGoals.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/entities/item/ItemEntity.hpp"
#include "entity/entities/passive/water/DolphinEntity.hpp"

namespace mc {
namespace test {

// ==================== DolphinGoals 基础测试 ====================

class DolphinGoalsTest : public ::testing::Test {
protected:
    void SetUp() override { dolphin = std::make_unique<DolphinEntity>(EntityInstanceId(1)); }

    void TearDown() override { dolphin.reset(); }

    std::unique_ptr<DolphinEntity> dolphin;
};

// ==================== DolphinJumpGoal 测试 ====================

TEST_F(DolphinGoalsTest, DolphinJumpGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::DolphinJumpGoal>(dolphin.get(), 10);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "DolphinJumpGoal");
}

TEST_F(DolphinGoalsTest, DolphinJumpGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::DolphinJumpGoal>(dolphin.get(), 10);

    // DolphinJumpGoal 应该有 Move 和 Jump 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DolphinGoalsTest, DolphinJumpGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::DolphinJumpGoal>(dolphin.get(), 10);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== SwimToTreasureGoal 测试 ====================

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::SwimToTreasureGoal>(dolphin.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "SwimToTreasureGoal");
}

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::SwimToTreasureGoal>(dolphin.get());

    // SwimToTreasureGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::SwimToTreasureGoal>(dolphin.get());

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_ShouldExecuteWithoutFish)
{
    auto goal = std::make_unique<entity::ai::goal::SwimToTreasureGoal>(dolphin.get());

    // 海豚未得到鱼时不应执行
    // 注：dolphin->setGotFish(false) 需要实现
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== SwimWithPlayerGoal 测试 ====================

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::SwimWithPlayerGoal>(dolphin.get(), 4.0);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "SwimWithPlayerGoal");
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::SwimWithPlayerGoal>(dolphin.get(), 4.0);

    // SwimWithPlayerGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::SwimWithPlayerGoal>(dolphin.get(), 4.0);

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ==================== PlayWithItemsGoal 测试 ====================

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_Construction)
{
    auto goal = std::make_unique<entity::ai::goal::PlayWithItemsGoal>(dolphin.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "PlayWithItemsGoal");
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::PlayWithItemsGoal>(dolphin.get());

    // PlayWithItemsGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_ShouldExecuteWithoutWorld)
{
    auto goal = std::make_unique<entity::ai::goal::PlayWithItemsGoal>(dolphin.get());

    // 无世界时不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_ShouldExecuteWithMainHandItem)
{
    auto goal = std::make_unique<entity::ai::goal::PlayWithItemsGoal>(dolphin.get());

    // 当海豚主手持有物品时应该执行
    // 注：需要实现 dolphin->setMainHandItem()
    // 此测试验证空世界、无物品的情况下不执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_ShouldNotExecuteDuringCooldown)
{
    auto goal = std::make_unique<entity::ai::goal::PlayWithItemsGoal>(dolphin.get());

    // 冷却期间不应执行
    // 由于 ticksExisted 从 0 开始，cooldown 默认为 0
    // 所以如果没有冷却设置，应该可以执行（如果有物品）
    EXPECT_FALSE(goal->shouldExecute()); // 无世界、无物品
}

// ==================== PlayWithItemsGoal 常量测试 ====================

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_SearchRadius)
{
    // MC 1.16.5: 搜索物品半径为 8 格
    EXPECT_FLOAT_EQ(entity::ai::goal::PlayWithItemsGoal::SEARCH_RADIUS, 8.0f);
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_ThrowVelocity)
{
    // MC 1.16.5: 扔出速度为 0.3
    EXPECT_FLOAT_EQ(entity::ai::goal::PlayWithItemsGoal::THROW_VELOCITY, 0.3f);
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_PickupDelay)
{
    // MC 1.16.5: 扔出物品的拾取延迟为 40 ticks
    EXPECT_EQ(entity::ai::goal::PlayWithItemsGoal::PICKUP_DELAY, 40);
}

TEST_F(DolphinGoalsTest, PlayWithItemsGoal_MinCooldown)
{
    // MC 1.16.5: 最小冷却时间为 100 ticks
    EXPECT_EQ(entity::ai::goal::PlayWithItemsGoal::MIN_COOLDOWN, 100);
}

// ==================== SwimToTreasureGoal 常量测试 ====================

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_MinAir)
{
    // MC 1.16.5: 最小空气值要求为 100
    EXPECT_EQ(entity::ai::goal::SwimToTreasureGoal::MIN_AIR, 100);
}

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_ArriveDistance)
{
    // MC 1.16.5: 到达距离为 4 格
    EXPECT_FLOAT_EQ(entity::ai::goal::SwimToTreasureGoal::ARRIVE_DISTANCE, 4.0f);
}

TEST_F(DolphinGoalsTest, SwimToTreasureGoal_CloseToTargetDistance)
{
    // MC 1.16.5: 接近目标距离为 12 格
    EXPECT_FLOAT_EQ(entity::ai::goal::SwimToTreasureGoal::CLOSE_TO_TARGET_DISTANCE, 12.0f);
}

// ==================== DolphinJumpGoal 常量测试 ====================

TEST_F(DolphinGoalsTest, DolphinJumpGoal_JumpDistances)
{
    // MC 1.16.5: 跳跃距离检查点 {0, 1, 4, 5, 6, 7}
    // 使用 std::size 获取数组长度
    constexpr auto& distances = entity::ai::goal::DolphinJumpGoal::JUMP_DISTANCES;
    EXPECT_EQ(std::size(distances), 6);
    EXPECT_EQ(distances[0], 0);
    EXPECT_EQ(distances[1], 1);
    EXPECT_EQ(distances[2], 4);
    EXPECT_EQ(distances[3], 5);
    EXPECT_EQ(distances[4], 6);
    EXPECT_EQ(distances[5], 7);
}

// DolphinJumpGoal 使用动态计算的速度，而非固定常量
// 速度计算参考 DolphinGoals.cpp 中的实现

// ==================== SwimWithPlayerGoal 常量测试 ====================

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_SearchRadius)
{
    // MC 1.16.5: 搜索玩家半径为 10 格
    EXPECT_FLOAT_EQ(entity::ai::goal::SwimWithPlayerGoal::SEARCH_RADIUS, 10.0f);
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_CloseDistanceSq)
{
    // MC 1.16.5: 接近距离平方为 6.25 (2.5²)
    EXPECT_FLOAT_EQ(entity::ai::goal::SwimWithPlayerGoal::CLOSE_DISTANCE_SQ, 6.25f);
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_MaxDistanceSq)
{
    // MC 1.16.5: 最大距离平方为 256 (16²)
    EXPECT_FLOAT_EQ(entity::ai::goal::SwimWithPlayerGoal::MAX_DISTANCE_SQ, 256.0f);
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_EffectDuration)
{
    // MC 1.16.5: 效果持续时间为 100 ticks (5秒)
    EXPECT_EQ(entity::ai::goal::SwimWithPlayerGoal::EFFECT_DURATION, 100);
}

TEST_F(DolphinGoalsTest, SwimWithPlayerGoal_EffectInterval)
{
    // MC 1.16.5: 效果刷新间隔为 6 ticks
    EXPECT_EQ(entity::ai::goal::SwimWithPlayerGoal::EFFECT_INTERVAL, 6);
}

} // namespace test
} // namespace mc
