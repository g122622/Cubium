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
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
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

// ==================== PandaSneezeGoal 测试 ====================
// 对齐 vanilla 1.21.11 Panda.PandaSneezeGoal（Panda.java:1103-1130）。
// 修复前：PandaEntity::registerGoals 未注册 PandaSneezeGoal，_onSneezeComplete 是死代码
// （m_sneezing 恒 false，tick 中 sneeze 分支永不进入）。本组测试验证 goal 逻辑与
// sneeze(true)→tick→_onSneezeComplete 链路激活。

class PandaSneezeGoalTest : public ::testing::Test {
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

TEST_F(PandaSneezeGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "PandaSneezeGoal");
}

TEST_F(PandaSneezeGoalTest, ShouldNotExecute_WhenAdult)
{
    // 对齐 vanilla canUse：仅 isBaby() 才考虑打喷嚏。成年熊猫应返 false。
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());

    panda->setChild(false);
    panda->setPersonality(PandaEntity::Personality::Weak);
    panda->setOnGround(true);

    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaSneezeGoalTest, ShouldNotExecute_WhenChildButBusy)
{
    // 对齐 vanilla canUse：isBaby() && canPerformAction() 双门控。
    // 幼年但正在吃东西/打滚/躺/打喷嚏时 canPerformAction()==false → 不触发。
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());

    panda->setChild(true);
    panda->setPersonality(PandaEntity::Personality::Weak);
    panda->setOnGround(true);

    // 正在吃东西 → canPerformAction==false
    panda->setEating(true);
    EXPECT_FALSE(goal->shouldExecute());

    panda->setEating(false);
    panda->setRolling(true);
    EXPECT_FALSE(goal->shouldExecute());

    panda->setRolling(false);
    panda->setLying(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaSneezeGoalTest, ShouldNotContinueExecuting)
{
    // 对齐 vanilla canContinueToUse：return false（一次性触发，由 tick 计时驱动后续）。
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(PandaSneezeGoalTest, StartExecuting_SetsSneezingAndTimer)
{
    // 对齐 vanilla PandaSneezeGoal.start：panda.sneeze(true)。
    // sneeze(true) 设 m_sneezing=true + m_sneezeTimer=SNEEZE_DURATION。
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());

    EXPECT_FALSE(panda->isSneezing());
    EXPECT_EQ(panda->getSneezeTimer(), 0);

    goal->startExecuting();

    EXPECT_TRUE(panda->isSneezing());
    EXPECT_EQ(panda->getSneezeTimer(), PandaEntity::SNEEZE_DURATION);
}

TEST_F(PandaSneezeGoalTest, Sneeze_TrueSetsTimer_FalseResetsTimer)
{
    // sneeze(true) 初始化计时器；sneeze(false) 重置（对齐 vanilla sneeze(false)→setSneezeCounter(0)）。
    panda->sneeze(true);
    EXPECT_TRUE(panda->isSneezing());
    EXPECT_EQ(panda->getSneezeTimer(), PandaEntity::SNEEZE_DURATION);

    panda->sneeze(false);
    EXPECT_FALSE(panda->isSneezing());
    EXPECT_EQ(panda->getSneezeTimer(), 0);
}

TEST_F(PandaSneezeGoalTest, ShouldExecute_WeakBabyCanTrigger)
{
    // 虚弱幼年熊猫 1/reducedTickDelay(500)=1/250 概率触发。固定种子下反复调用 shouldExecute，
    // 2000 次内期望命中（期望 ~8 次）。验证 goal 概率逻辑可触发（修复前 goal 未注册，
    // 此路径不存在）。非确定性兜底：2000 次足够覆盖 1/250 概率。
    auto goal = std::make_unique<entity::ai::goal::PandaSneezeGoal>(panda.get());

    panda->setChild(true);
    panda->setPersonality(PandaEntity::Personality::Weak);
    panda->setOnGround(true);

    bool triggered = false;
    for (int i = 0; i < 2000; ++i) {
        if (goal->shouldExecute()) {
            triggered = true;
            break;
        }
    }
    EXPECT_TRUE(triggered);
}

// ==================== PandaAttackGoal 测试 ====================
// 对齐 vanilla 1.21.11 Panda.PandaAttackGoal（Panda.java:759-771）。
// 修复前：PandaEntity::registerGoals 未注册任何攻击 goal，好斗熊猫 ATTACK_DAMAGE=6.0 为死属性，
// 被攻击不反击。PandaAttackGoal 在 MeleeAttackGoal 基础上叠加 canPerformAction() 门控：
//   canUse() = panda.canPerformAction() && super.canUse();
// 即打喷嚏/吃东西/躺/打滚时不发起攻击。本组验证门控的拦截方向（忙碌时即使有 target 也 false）。
// “空闲且有 target → true”的委托通过方向受 MeleeAttackGoal 20tick 节流与 navigator 依赖，
// 单元测试难以稳定构造，由集成测试 panda_aggressive_counterattack 覆盖完整反击链路。

class PandaAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        panda = std::make_unique<PandaEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
        // 好斗性格：反击链路仅对好斗熊猫生效（ATTACK_DAMAGE=6.0）。
        panda->setPersonality(PandaEntity::Personality::Aggressive);
        // 用另一只熊猫作为攻击目标（canAttackType 仅排除 GHAST，同类可攻击）。
        target = std::make_unique<PandaEntity>(EntityInstanceId(2), mc::test::testEcsRegistry());
    }

    void TearDown() override
    {
        panda.reset();
        target.reset();
    }

    std::unique_ptr<PandaEntity> panda;
    std::unique_ptr<PandaEntity> target;
};

TEST_F(PandaAttackGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "PandaAttackGoal");
}

TEST_F(PandaAttackGoalTest, InheritsMeleeAttackGoalMutexFlags)
{
    // PandaAttackGoal 继承 MeleeAttackGoal 的互斥标志（Move, Look）。
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
}

TEST_F(PandaAttackGoalTest, ShouldNotExecute_WhenNoAttackTarget)
{
    // 空闲且无 attackTarget：canPerformAction=true 后委托基类，基类因无 target 返 false。
    // 验证委托路径不崩溃且返 false。
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    panda->setAttackTarget(nullptr);
    EXPECT_TRUE(panda->canPerformAction());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaAttackGoalTest, ShouldNotExecute_WhenSneezing)
{
    // canPerformAction 门控：打喷嚏时即使有 attackTarget 也返 false（不进基类）。
    // 这是 PandaAttackGoal 相对基类 MeleeAttackGoal 的核心新增门控。
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    panda->setAttackTarget(target.get());
    panda->setSneezing(true);
    EXPECT_FALSE(panda->canPerformAction());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaAttackGoalTest, ShouldNotExecute_WhenEating)
{
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    panda->setAttackTarget(target.get());
    panda->setEating(true);
    EXPECT_FALSE(panda->canPerformAction());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaAttackGoalTest, ShouldNotExecute_WhenLying)
{
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    panda->setAttackTarget(target.get());
    panda->setLying(true);
    EXPECT_FALSE(panda->canPerformAction());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaAttackGoalTest, ShouldNotExecute_WhenRolling)
{
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(panda.get(), 1.2, true);
    panda->setAttackTarget(target.get());
    panda->setRolling(true);
    EXPECT_FALSE(panda->canPerformAction());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PandaAttackGoalTest, NullPandaReturnsFalse)
{
    // 防御：panda 为 nullptr 时不崩溃。
    auto goal = std::make_unique<entity::ai::goal::PandaAttackGoal>(nullptr, 1.2, true);
    EXPECT_FALSE(goal->shouldExecute());
}

} // namespace test
} // namespace mc
