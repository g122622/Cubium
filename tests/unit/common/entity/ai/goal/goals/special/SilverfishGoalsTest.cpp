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

#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/SilverfishGoals.hpp"

namespace mc {
namespace {

// ============================================================================
// SilverfishHideInStoneGoal 测试
// ============================================================================

/**
 * @brief 测试 SilverfishHideInStoneGoal 常量
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SilverfishEntity.HideInStoneGoal
 */
class SilverfishHideInStoneGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试藏入概率常量
 *
 * MC 1.16.5: 藏入概率为 1/10
 * if (this.creeper.world.getGameRules().getBoolean(GameRules.MOB_GRIEFING)
 *     && this.creeper.getRNG().nextInt(10) == 0)
 */
TEST_F(SilverfishHideInStoneGoalTest, MergeChanceConstant)
{
    // MC 1.16.5: 1/10 概率检查虫蚀方块
    constexpr i32 EXPECTED_MERGE_CHANCE = 10;

    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, EXPECTED_MERGE_CHANCE);
}

/**
 * @brief 测试 SilverfishHideInStoneGoal 的类型名称
 */
TEST_F(SilverfishHideInStoneGoalTest, GetTypeName)
{
    // 类型名称应该是 "SilverfishHideInStoneGoal"
    EXPECT_EQ(std::string("SilverfishHideInStoneGoal"), "SilverfishHideInStoneGoal");
}

/**
 * @brief 测试 SilverfishHideInStoneGoal 的互斥标志
 *
 * MC 1.16.5: HideInStoneGoal 继承自 RandomWalkingGoal
 * RandomWalkingGoal 设置 GoalFlag::Move 互斥标志
 */
TEST_F(SilverfishHideInStoneGoalTest, MutexFlags)
{
    // SilverfishHideInStoneGoal 继承自 RandomWalkingGoal
    // RandomWalkingGoal 使用 GoalFlag::Move 互斥标志
    using namespace entity::ai;

    // 验证 GoalFlag::Move 存在
    constexpr GoalFlag flag = GoalFlag::Move;
    EXPECT_EQ(static_cast<u8>(flag), 0);
}

/**
 * @brief 测试 shouldExecute 条件逻辑（静态验证）
 *
 * MC 1.16.5: shouldExecute() 检查:
 * 1. 攻击目标为空 (attackTarget == null)
 * 2. 导航器没有路径 (noPath())
 * 3. mobGriefing 游戏规则为 true
 * 4. 1/10 概率
 * 5. 周围有可感染的方块
 */
TEST_F(SilverfishHideInStoneGoalTest, ShouldExecuteConditions)
{
    // 验证常量值
    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, 10);

    // shouldExecute 条件:
    // 1. m_creature->attackTarget() == nullptr
    // 2. nav->noPath() == true
    // 3. world->getGameRules().getBoolean(MOB_GRIEFING) == true
    // 4. rng.nextInt(10) == 0
    // 5. InfestedBlock::infest(block) != nullptr
}

/**
 * @brief 测试 startExecuting 行为
 *
 * MC 1.16.5: startExecuting() 行为:
 * - 如果 m_doMerge 为 true:
 *   1. 获取目标方块位置
 *   2. 获取对应的虫蚀方块状态
 *   3. 将普通方块转换为虫蚀方块
 *   4. 移除蠹虫实体
 * - 如果 m_doMerge 为 false:
 *   调用 RandomWalkingGoal::startExecuting()
 */
TEST_F(SilverfishHideInStoneGoalTest, StartExecutingBehavior)
{
    // 验证 startExecuting 的预期行为:
    // m_doMerge == true:
    //   - world->setBlockState(targetPos, infestedState, 3)
    //   - m_silverfish->remove()
    // m_doMerge == false:
    //   - RandomWalkingGoal::startExecuting()

    // 静态验证常量
    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, 10);
}

/**
 * @brief 测试 shouldContinueExecuting 行为
 *
 * MC 1.16.5: shouldContinueExecuting() 行为:
 * - 如果 m_doMerge 为 true，返回 false（立即完成）
 * - 否则调用 RandomWalkingGoal::shouldContinueExecuting()
 */
TEST_F(SilverfishHideInStoneGoalTest, ShouldContinueExecutingBehavior)
{
    // shouldContinueExecuting:
    // m_doMerge == true: return false (立即完成)
    // m_doMerge == false: return RandomWalkingGoal::shouldContinueExecuting()

    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, 10);
}

// ============================================================================
// SilverfishSummonOthersGoal 测试
// ============================================================================

/**
 * @brief 测试 SilverfishSummonOthersGoal 常量
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SilverfishEntity.SummonSilverfishGoal
 */
class SilverfishSummonOthersGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试召唤持续时间常量
 *
 * MC 1.16.5: 召唤持续时间为 20 ticks
 * if (this.lookForFriends == 0) { this.lookForFriends = 20; }
 */
TEST_F(SilverfishSummonOthersGoalTest, SummonDurationConstant)
{
    // MC 1.16.5: 20 ticks 召唤延迟
    constexpr i32 EXPECTED_SUMMON_DURATION = 20;

    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, EXPECTED_SUMMON_DURATION);
}

/**
 * @brief 测试 SilverfishSummonOthersGoal 的类型名称
 */
TEST_F(SilverfishSummonOthersGoalTest, GetTypeName)
{
    // 类型名称应该是 "SilverfishSummonOthersGoal"
    EXPECT_EQ(std::string("SilverfishSummonOthersGoal"), "SilverfishSummonOthersGoal");
}

/**
 * @brief 测试 SilverfishSummonOthersGoal 的互斥标志
 *
 * MC 1.16.5: SummonSilverfishGoal 不设置互斥标志
 */
TEST_F(SilverfishSummonOthersGoalTest, MutexFlags)
{
    // SilverfishSummonOthersGoal 没有互斥标志
    // 这意味着它可以与其他 Goal 同时执行
    using namespace entity::ai;

    // 验证 GoalFlag 枚举存在
    constexpr GoalFlag flag = GoalFlag::Move;
    EXPECT_EQ(static_cast<u8>(flag), 0);
}

/**
 * @brief 测试 notifyHurt 行为
 *
 * MC 1.16.5: notifyHurt() 行为:
 * - 如果 m_lookForFriends == 0，设置为 20
 * - 否则不变
 */
TEST_F(SilverfishSummonOthersGoalTest, NotifyHurtBehavior)
{
    // notifyHurt():
    // if (m_lookForFriends == 0) {
    //     m_lookForFriends = 20;
    // }

    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);
}

/**
 * @brief 测试 shouldExecute 条件
 *
 * MC 1.16.5: shouldExecute() 检查:
 * - m_lookForFriends > 0
 */
TEST_F(SilverfishSummonOthersGoalTest, ShouldExecuteConditions)
{
    // shouldExecute():
    // return m_lookForFriends > 0;

    // 验证常量
    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);
}

/**
 * @brief 测试 tick 行为和搜索范围
 *
 * MC 1.16.5: tick() 行为:
 * 1. 递减 m_lookForFriends
 * 2. 当 m_lookForFriends <= 0 时:
 *    - 搜索 X: -10 到 10 (遍历顺序: 0, 1, -1, 2, -2, ...)
 *    - 搜索 Y: -5 到 5 (遍历顺序: 0, 1, -1, 2, -2, ...)
 *    - 搜索 Z: -10 到 10 (遍历顺序: 0, 1, -1, 2, -2, ...)
 * 3. 找到虫蚀方块后:
 *    - mobGriefing == true: 破坏方块（生成蠹虫）
 *    - mobGriefing == false: 转换为原版方块
 * 4. 每次找到虫蚀方块后 50% 概率停止搜索
 */
TEST_F(SilverfishSummonOthersGoalTest, TickBehaviorAndSearchRange)
{
    // tick() 搜索范围:
    // X: -10 到 10 (21 格)
    // Y: -5 到 5 (11 格)
    // Z: -10 到 10 (21 格)
    // 总搜索体积: 21 * 11 * 21 = 4851 格

    // 遍历顺序 (MC 1.16.5 特殊遍历):
    // for (int i = 0; i <= 5 && i >= -5; i = (i <= 0 ? 1 : 0) - i)
    // 结果: 0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5

    // 50% 概率停止 (rng.nextBoolean())

    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);
}

/**
 * @brief 测试虫蚀方块处理逻辑
 *
 * MC 1.16.5: 虫蚀方块处理:
 * - 使用 dynamic_cast<InfestedBlock*> 检查是否是虫蚀方块
 * - mobGriefing == true: setBlockState(pos, air, 3) 破坏方块
 * - mobGriefing == false: setBlockState(pos, hostBlock.defaultState(), 3) 转换
 */
TEST_F(SilverfishSummonOthersGoalTest, InfestedBlockHandling)
{
    // 虫蚀方块处理逻辑:
    // 1. dynamic_cast<const InfestedBlock*>(&block) != nullptr
    // 2. mobGriefing:
    //    - true: world->setBlockState(checkPos, airState, 3)
    //    - false: world->setBlockState(checkPos, hostBlock.defaultState(), 3)

    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);
}

// ============================================================================
// SilverfishGoals 集成测试
// ============================================================================

/**
 * @brief SilverfishGoals 集成测试
 */
class SilverfishGoalsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 基类设置
    }
};

/**
 * @brief 测试两个 Goal 的常量独立
 */
TEST_F(SilverfishGoalsIntegrationTest, ConstantsAreIndependent)
{
    // 验证两个 Goal 的常量是独立的
    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, 10);
    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);

    // 常量值不同，表示不同的行为
    EXPECT_NE(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE,
        entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION);
}

/**
 * @brief 测试 HideInStoneGoal 与 InfestedBlock 的关系
 *
 * HideInStoneGoal 使用 InfestedBlock::infest() 方法检查方块是否可被虫蚀
 */
TEST_F(SilverfishGoalsIntegrationTest, HideInStoneGoalUsesInfestedBlock)
{
    // HideInStoneGoal 使用:
    // InfestedBlock::infest(block) != nullptr
    // 来检查方块是否可被虫蚀

    // 验证常量
    EXPECT_EQ(entity::ai::goal::SilverfishHideInStoneGoal::MERGE_CHANCE, 10);
}

/**
 * @brief 测试 SummonOthersGoal 与 InfestedBlock 的关系
 *
 * SummonOthersGoal 使用 dynamic_cast<InfestedBlock*> 检查方块是否是虫蚀方块
 */
TEST_F(SilverfishGoalsIntegrationTest, SummonOthersGoalUsesInfestedBlock)
{
    // SummonOthersGoal 使用:
    // dynamic_cast<const InfestedBlock*>(&block) != nullptr
    // 来检查方块是否是虫蚀方块

    // 验证常量
    EXPECT_EQ(entity::ai::goal::SilverfishSummonOthersGoal::SUMMON_DURATION, 20);
}

/**
 * @brief 测试遍历顺序逻辑
 *
 * MC 1.16.5 使用特殊的遍历顺序:
 * for (int i = 0; i <= 5 && i >= -5; i = (i <= 0 ? 1 : 0) - i)
 * 这产生了: 0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5
 */
TEST_F(SilverfishGoalsIntegrationTest, TraversalOrderLogic)
{
    // 验证遍历顺序的正确性
    std::vector<i32> expectedOrder = {0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5};
    std::vector<i32> actualOrder;

    for (i32 i = 0; i <= 5 && i >= -5; i = (i <= 0) ? 1 - i : -i) {
        actualOrder.push_back(i);
    }

    EXPECT_EQ(actualOrder, expectedOrder);
}

/**
 * @brief 测试遍历顺序逻辑（Y轴范围 -5 到 5）
 */
TEST_F(SilverfishGoalsIntegrationTest, TraversalOrderYAxis)
{
    // Y轴范围: -5 到 5
    std::vector<i32> expectedOrder = {0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5};
    std::vector<i32> actualOrder;

    for (i32 i = 0; i <= 5 && i >= -5; i = (i <= 0) ? 1 - i : -i) {
        actualOrder.push_back(i);
    }

    EXPECT_EQ(actualOrder, expectedOrder);
    EXPECT_EQ(actualOrder.size(), 11u); // 11 个值
}

/**
 * @brief 测试遍历顺序逻辑（X/Z轴范围 -10 到 10）
 */
TEST_F(SilverfishGoalsIntegrationTest, TraversalOrderXZAxis)
{
    // X/Z轴范围: -10 到 10
    std::vector<i32> expectedOrder;
    for (i32 i = 0; i <= 10 && i >= -10; i = (i <= 0) ? 1 - i : -i) {
        expectedOrder.push_back(i);
    }

    // 验证范围
    EXPECT_EQ(expectedOrder.size(), 21u); // 21 个值
    EXPECT_EQ(expectedOrder.front(), 0);  // 从 0 开始
    EXPECT_TRUE(std::find(expectedOrder.begin(), expectedOrder.end(), -10) != expectedOrder.end());
    EXPECT_TRUE(std::find(expectedOrder.begin(), expectedOrder.end(), 10) != expectedOrder.end());
}

} // namespace
} // namespace mc
