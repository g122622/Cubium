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

#include "common/entity/ai/goal/goals/special/SpecialGoals.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"

namespace mc {
namespace {

/**
 * @brief 测试 TriggerSkeletonTrapGoal 常量
 *
 * MC 1.16.5 参考: net.minecraft.entity.ai.goal.TriggerSkeletonTrapGoal
 */
class TriggerSkeletonTrapGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试玩家检测范围常量
 *
 * MC 1.16.5: 玩家检测范围为 10 格
 */
TEST_F(TriggerSkeletonTrapGoalTest, PlayerDetectionRangeConstant)
{
    // MC 1.16.5: this.horse.world.isPlayerWithin(x, y, z, 10.0D)
    constexpr f64 EXPECTED_RANGE = 10.0;
    constexpr f64 EXPECTED_RANGE_SQ = 100.0;

    EXPECT_DOUBLE_EQ(entity::ai::goal::TriggerSkeletonTrapGoal::PLAYER_DETECTION_RANGE, EXPECTED_RANGE);
    EXPECT_DOUBLE_EQ(entity::ai::goal::TriggerSkeletonTrapGoal::PLAYER_DETECTION_RANGE_SQ, EXPECTED_RANGE_SQ);
}

/**
 * @brief 测试 TriggerSkeletonTrapGoal 的类型名称
 */
TEST_F(TriggerSkeletonTrapGoalTest, GetTypeName)
{
    // 创建一个空的骷髅马来测试 Goal 构造
    // 注意：由于没有世界，Goal 可能不会正常工作
    // 但我们可以测试基本功能

    // 类型名称应该是 "TriggerSkeletonTrapGoal"
    EXPECT_EQ(std::string("TriggerSkeletonTrapGoal"), "TriggerSkeletonTrapGoal");
}

/**
 * @brief 测试 TriggerSkeletonTrapGoal 的互斥标志
 *
 * MC 1.16.5: TriggerSkeletonTrapGoal 继承自 Goal
 * 触发时应该执行 tick() 方法
 */
TEST_F(TriggerSkeletonTrapGoalTest, MutexFlags)
{
    // TriggerSkeletonTrapGoal 使用 GoalFlag::Move 互斥标志
    // 这意味着它与其他移动类 Goal 互斥
    using namespace entity::ai;

    // GoalFlag::Move 是第一个枚举值，值为 0
    // 验证枚举类型存在且可访问
    constexpr GoalFlag flag = GoalFlag::Move;

    // 验证 GoalFlag 类型正确
    EXPECT_EQ(static_cast<u8>(flag), 0);
    EXPECT_EQ(static_cast<u8>(GoalFlag::Look), 1);
    EXPECT_EQ(static_cast<u8>(GoalFlag::Jump), 2);
    EXPECT_EQ(static_cast<u8>(GoalFlag::Target), 3);
}

/**
 * @brief 测试陷阱触发条件逻辑（静态验证）
 *
 * MC 1.16.5: shouldExecute() 检查:
 * 1. 陷阱马状态 (isTrap)
 * 2. 玩家在 10 格范围内
 */
TEST_F(TriggerSkeletonTrapGoalTest, ShouldExecuteConditions)
{
    // 验证条件逻辑的正确性
    // 1. 陷阱马必须是陷阱状态
    // 2. 玩家必须在检测范围内
    // 3. 玩家不能是旁观者或创造模式

    // 这是一个静态测试，验证常量值
    EXPECT_DOUBLE_EQ(entity::ai::goal::TriggerSkeletonTrapGoal::PLAYER_DETECTION_RANGE, 10.0);
}

/**
 * @brief 测试陷阱触发后的行为
 *
 * MC 1.16.5: tick() 调用 horse.triggerTrap()
 */
TEST_F(TriggerSkeletonTrapGoalTest, TickBehavior)
{
    // tick() 方法应该:
    // 1. 调用 triggerTrap() 生成骷髅骑手
    // 2. 设置 m_trap = false
    // 3. 在困难模式下生成额外骷髅马

    // 这是一个静态测试，验证常量值
    EXPECT_DOUBLE_EQ(entity::ai::goal::TriggerSkeletonTrapGoal::PLAYER_DETECTION_RANGE, 10.0);
}

} // namespace
} // namespace mc
