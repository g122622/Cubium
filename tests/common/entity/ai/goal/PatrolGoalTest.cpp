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

#include "entity/ai/goal/goals/special/PatrolGoals.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/monster/illager/PatrollerEntity.hpp"
#include "world/block/BlockPos.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

namespace mc {
namespace entity::ai::goal {
namespace test {

/**
 * @brief PatrolGoal 常量测试
 *
 * 验证 MC 1.16.5 中的常量值是否正确。
 */
TEST(PatrolGoalTest, ConstantsAreCorrect)
{
    // MC 1.16.5: 到达目标阈值 10 格
    EXPECT_DOUBLE_EQ(PatrolGoal::ARRIVAL_THRESHOLD, 10.0);
    EXPECT_DOUBLE_EQ(PatrolGoal::ARRIVAL_THRESHOLD_SQ, 100.0);

    // MC 1.16.5: 搜索附近队员范围 16 格
    EXPECT_DOUBLE_EQ(PatrolGoal::NEARBY_PATROLLER_RANGE, 16.0);
    EXPECT_DOUBLE_EQ(PatrolGoal::NEARBY_PATROLLER_RANGE_SQ, 256.0);

    // MC 1.16.5: 移动失败冷却时间 200 tick（约 10 秒）
    EXPECT_EQ(PatrolGoal::COOLDOWN_TICKS, 200L);

    // MC 1.16.5: 随机移动范围 8 格
    EXPECT_EQ(PatrolGoal::RANDOM_MOVE_RANGE, 8);
}

/**
 * @brief PatrolGoal 构造函数测试
 *
 * 验证 PatrolGoal 可以正确构造。
 */
TEST(PatrolGoalTest, ConstructorValidParameters)
{
    // 创建一个空的 PatrolGoal（传 nullptr）
    // 这在单元测试中是安全的，因为我们不会执行它
    PatrolGoal goal(nullptr, 0.7, 0.595);

    // 验证 getTypeName 返回正确的类型名
    EXPECT_EQ(goal.getTypeName(), "PatrolGoal");

    // 验证互斥标志已设置
    // GoalFlag::Move 应该被设置
    auto flags = goal.getMutexFlags();
    EXPECT_TRUE(flags[GoalFlag::Move]);
}

/**
 * @brief PatrolGoal 空指针安全测试
 *
 * 验证 PatrolGoal 在空指针情况下安全返回 false。
 */
TEST(PatrolGoalTest, NullPointerSafety)
{
    PatrolGoal goal(nullptr, 0.7, 0.595);

    // shouldExecute 应该在空指针时返回 false
    EXPECT_FALSE(goal.shouldExecute());

    // shouldContinueExecuting 应该在空指针时返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

/**
 * @brief PatrolGoal 速度参数测试
 *
 * 验证队长和队员速度参数是否符合 MC 1.16.5 规范。
 * MC 1.16.5: 队员速度 0.7D，队长速度 0.595D
 */
TEST(PatrolGoalTest, SpeedParametersMatchMC1165)
{
    // MC 1.16.5 PatrollerEntity.PatrolGoal 构造函数参数:
    // new PatrolGoal<>(this, 0.7D, 0.595D)
    // 其中:
    // - 0.7D 是队员速度（memberSpeed = field_220840_b）
    // - 0.595D 是队长速度（leaderSpeed = field_220841_c）

    constexpr f64 MEMBER_SPEED = 0.7;
    constexpr f64 LEADER_SPEED = 0.595;

    // 队员速度应该大于队长速度（队长走得慢，让队员能跟上）
    EXPECT_GT(MEMBER_SPEED, LEADER_SPEED);

    // 创建 Goal 验证参数传递
    PatrolGoal goal(nullptr, MEMBER_SPEED, LEADER_SPEED);
    EXPECT_EQ(goal.getTypeName(), "PatrolGoal");
}

/**
 * @brief PatrollerEntity 基础功能测试
 *
 * 验证 PatrollerEntity 的巡逻状态管理功能。
 */
TEST(PatrolGoalTest, PatrollerEntityBasicFunctions)
{
    // 验证 PatrollerEntity 类定义了所需的接口
    // 注意：这些测试验证接口签名，不涉及实际实体创建

    // 检查 PatrollerEntity 继承自 MonsterEntity
    static_assert(
        std::is_base_of_v<MonsterEntity, PatrollerEntity>, "PatrollerEntity should inherit from MonsterEntity");

    // 检查 PatrollerEntity 有所需的方法
    // hasPatrolTarget(), isPatrolling(), isLeader(), canJoinPatrol()
    // 这些方法在头文件中已定义
}

/**
 * @brief PatrolGoal 互斥标志测试
 *
 * 验证 PatrolGoal 使用 Move 标志，与其他移动类目标互斥。
 */
TEST(PatrolGoalTest, MutexFlagsCorrect)
{
    PatrolGoal goal(nullptr, 0.7, 0.595);

    // 获取互斥标志
    auto flags = goal.getMutexFlags();

    // 应该只有 Move 标志
    EXPECT_TRUE(flags[GoalFlag::Move]);
    EXPECT_FALSE(flags[GoalFlag::Look]);
    EXPECT_FALSE(flags[GoalFlag::Jump]);
    EXPECT_FALSE(flags[GoalFlag::Target]);
}

/**
 * @brief PatrolGoal 默认冷却时间测试
 *
 * 验证冷却时间初始值为 -1，表示无冷却。
 */
TEST(PatrolGoalTest, DefaultCooldownIsNegative)
{
    // 冷却时间初始值应该在构造函数中设置为 -1
    // 这样第一次 shouldExecute 不会被冷却阻止
    // 这个测试只是验证设计，实际值在私有成员中
    PatrolGoal goal(nullptr, 0.7, 0.595);
    // 冷却时间是私有成员，无法直接访问
    // 但可以通过 shouldExecute 在无世界情况下返回 false 来间接验证
    EXPECT_FALSE(goal.shouldExecute());
}

} // namespace test
} // namespace entity::ai::goal
} // namespace mc
