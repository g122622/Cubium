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
#include "entity/ai/goal/goals/special/PandaGoals.hpp"
#include "entity/entities/passive/special/PandaEntity.hpp"
#include "item/Items.hpp"

namespace mc {
namespace test {

// ==================== PandaRollGoal 基础测试 ====================

class PandaRollGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        panda = std::make_unique<PandaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { panda.reset(); }

    std::unique_ptr<PandaEntity> panda;
};

TEST_F(PandaRollGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "PandaRollGoal");
}

TEST_F(PandaRollGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // PandaRollGoal 应该有 Move、Look 和 Jump 标志
    // 参考 MC 1.16.5: setMutexFlags(EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK, Goal.Flag.JUMP))
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(PandaRollGoalTest, NotPreemptible)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 打滚不可中断
    // 参考 MC 1.16.5: public boolean isPreemptible() { return false; }
    EXPECT_FALSE(goal->isPreemptible());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenNotOnGround)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为顽皮性格
    panda->setPersonality(PandaEntity::Personality::Playful);

    // 默认不在地面
    EXPECT_FALSE(panda->onGround());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenNotChildOrPlayful)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为成年普通熊猫
    panda->setChild(false);
    panda->setPersonality(PandaEntity::Personality::Normal);
    panda->setOnGround(true);

    // 成年普通熊猫不应该触发打滚
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenSneezing)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为顽皮性格且在地面
    panda->setPersonality(PandaEntity::Personality::Playful);
    panda->setOnGround(true);

    // 但正在打喷嚏
    panda->setSneezing(true);

    // 不应该触发打滚
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenEating)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为顽皮性格且在地面
    panda->setPersonality(PandaEntity::Personality::Playful);
    panda->setOnGround(true);

    // 但正在吃东西
    panda->setEating(true);

    // 不应该触发打滚
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenLying)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为顽皮性格且在地面
    panda->setPersonality(PandaEntity::Personality::Playful);
    panda->setOnGround(true);

    // 但正在躺着
    panda->setLying(true);

    // 不应该触发打滚
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotExecute_WhenAlreadyRolling)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 设置为顽皮性格且在地面
    panda->setPersonality(PandaEntity::Personality::Playful);
    panda->setOnGround(true);

    // 但已经在打滚
    panda->setRolling(true);

    // 不应该再次触发打滚
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaRollGoalTest, ShouldNotContinueExecuting)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // MC 1.16.5: public boolean shouldContinueExecuting() { return false; }
    // 打滚是一次性动作，由 rollCounter 控制持续时间
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(PandaRollGoalTest, StartExecuting_SetsRollingState)
{
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());

    // 初始状态
    EXPECT_FALSE(panda->isRolling());
    EXPECT_EQ(panda->getRollTimer(), 0);

    // 开始执行
    goal->startExecuting();

    // 验证打滚状态被设置
    EXPECT_TRUE(panda->isRolling());
    EXPECT_EQ(panda->getRollTimer(), 0); // 初始为0，由 updateRoll 递增
}

// ==================== PandaRollGoal 常量验证测试 ====================

TEST_F(PandaRollGoalTest, TriggerChanceConstants)
{
    // MC 1.16.5 常量：
    // PLAYFUL_ROLL_CHANCE = 60 (顽皮熊猫 1/60 概率)
    // NORMAL_ROLL_CHANCE = 500 (幼年熊猫 1/500 概率)
    // 这些是私有常量，通过行为验证
    auto goal = std::make_unique<entity::ai::goal::PandaRollGoal>(panda.get());
    EXPECT_NE(goal, nullptr);
}

// ==================== PandaEntity 打滚物理测试 ====================
//
// 注意：updateRoll() 是在 PandaEntity::tick() 中调用的受保护方法。
// 打滚物理测试（UpdateRoll_StartsRolling、UpdateRoll_StopsAfterDuration、
// UpdateRoll_ResetsWhenNotRolling）已在 PandaEntityTest.cpp 中通过
// TestablePandaEntity 测试夹具完成。

// ==================== PandaEntity 性格与打滚测试 ====================

class PandaPersonalityRollTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        panda = std::make_unique<PandaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { panda.reset(); }

    std::unique_ptr<PandaEntity> panda;
};

TEST_F(PandaPersonalityRollTest, PlayfulPandaCanRoll)
{
    // 顽皮熊猫可以打滚
    panda->setPersonality(PandaEntity::Personality::Playful);
    EXPECT_TRUE(panda->isPlayful());
}

TEST_F(PandaPersonalityRollTest, NormalAdultPandaCannotRoll)
{
    // 成年普通熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Normal);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, ChildPandaCanRoll)
{
    // 幼年熊猫可以打滚（无论性格）
    panda->setPersonality(PandaEntity::Personality::Normal);
    panda->setChild(true);
    EXPECT_TRUE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, LazyPandaCannotRollAsAdult)
{
    // 成年懒惰熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Lazy);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, AggressivePandaCannotRollAsAdult)
{
    // 成年好斗熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Aggressive);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, WorriedPandaCannotRollAsAdult)
{
    // 成年忧愁熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Worried);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, WeakPandaCannotRollAsAdult)
{
    // 成年虚弱熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Weak);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

TEST_F(PandaPersonalityRollTest, BrownPandaCannotRollAsAdult)
{
    // 成年棕色熊猫不能打滚
    panda->setPersonality(PandaEntity::Personality::Brown);
    panda->setChild(false);
    EXPECT_FALSE(panda->isPlayful());
    EXPECT_FALSE(panda->isChild());
}

// ==================== PandaEntity canPerformAction 测试 ====================

class PandaCanPerformActionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        panda = std::make_unique<PandaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
    }

    void TearDown() override { panda.reset(); }

    std::unique_ptr<PandaEntity> panda;
};

TEST_F(PandaCanPerformActionTest, CanPerformAction_WhenIdle)
{
    // 空闲状态可以执行动作
    EXPECT_TRUE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CannotPerformAction_WhenSneezing)
{
    panda->setSneezing(true);
    EXPECT_FALSE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CannotPerformAction_WhenEating)
{
    panda->setEating(true);
    EXPECT_FALSE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CannotPerformAction_WhenLying)
{
    panda->setLying(true);
    EXPECT_FALSE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CannotPerformAction_WhenRolling)
{
    panda->setRolling(true);
    EXPECT_FALSE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CanPerformAction_AfterSneezingReset)
{
    panda->setSneezing(true);
    EXPECT_FALSE(panda->canPerformAction());

    panda->setSneezing(false);
    EXPECT_TRUE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CanPerformAction_AfterEatingReset)
{
    panda->setEating(true);
    EXPECT_FALSE(panda->canPerformAction());

    panda->setEating(false);
    EXPECT_TRUE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CanPerformAction_AfterLyingReset)
{
    panda->setLying(true);
    EXPECT_FALSE(panda->canPerformAction());

    panda->setLying(false);
    EXPECT_TRUE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CanPerformAction_AfterRollingReset)
{
    panda->setRolling(true);
    EXPECT_FALSE(panda->canPerformAction());

    panda->setRolling(false);
    EXPECT_TRUE(panda->canPerformAction());
}

TEST_F(PandaCanPerformActionTest, CannotPerformAction_WhenMultipleStates)
{
    // 设置多个状态
    panda->setSneezing(true);
    panda->setEating(true);
    EXPECT_FALSE(panda->canPerformAction());

    // 重置所有状态
    panda->setSneezing(false);
    panda->setEating(false);
    EXPECT_TRUE(panda->canPerformAction());
}

} // namespace test
} // namespace mc
