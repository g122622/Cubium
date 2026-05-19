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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/WanderingTraderGoals.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"

using mc::BlockPos;
using mc::EntityId;
using mc::entity::WanderingTraderEntity;
using mc::entity::ai::GoalFlag;

namespace mc {
namespace {

// ============================================================================
// Test World for WanderingTrader Goals
// ============================================================================

class TestWanderingTraderWorld : public test::BaseTestWorld {
public:
    TestWanderingTraderWorld()
        : m_dayTime(0)
        , m_currentTick(0)
    {}

    void setDayTime(i64 time) { m_dayTime = time; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

private:
    i64 m_dayTime = 0;
    u64 m_currentTick = 0;
};

// ============================================================================
// LookAtCustomerGoal Tests
// ============================================================================

class LookAtCustomerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityId(1));
        m_trader->setWorld(m_world.get());
        m_trader->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_trader.reset();
        m_world.reset();
    }

    std::unique_ptr<TestWanderingTraderWorld> m_world;
    std::unique_ptr<WanderingTraderEntity> m_trader;
};

TEST_F(LookAtCustomerGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::wandering_trader::LookAtCustomerGoal>(m_trader.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "LookAtCustomerGoal");
}

TEST_F(LookAtCustomerGoalTest, MutexFlags)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<LookAtCustomerGoal>(m_trader.get());

    // LookAtCustomerGoal 应该只有 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(LookAtCustomerGoalTest, ShouldNotExecuteWithoutCustomer)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<LookAtCustomerGoal>(m_trader.get());

    // 没有交易中的玩家时不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// TradeWithPlayerGoal Tests
// ============================================================================

class TradeWithPlayerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityId(1));
        m_trader->setWorld(m_world.get());
        m_trader->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_trader.reset();
        m_world.reset();
    }

    std::unique_ptr<TestWanderingTraderWorld> m_world;
    std::unique_ptr<WanderingTraderEntity> m_trader;
};

TEST_F(TradeWithPlayerGoalTest, Construction)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "TradeWithPlayerGoal");
}

TEST_F(TradeWithPlayerGoalTest, MutexFlags)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // TradeWithPlayerGoal 应该有 Look 和 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(TradeWithPlayerGoalTest, ShouldNotExecuteWithoutCustomer)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // 没有交易中的玩家时不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// MoveToWanderTargetGoal Tests
// ============================================================================

class MoveToWanderTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityId(1));
        m_trader->setWorld(m_world.get());
        m_trader->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_trader.reset();
        m_world.reset();
    }

    std::unique_ptr<TestWanderingTraderWorld> m_world;
    std::unique_ptr<WanderingTraderEntity> m_trader;
};

TEST_F(MoveToWanderTargetGoalTest, Construction)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 2.0, 0.35);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "MoveToWanderTargetGoal");
}

TEST_F(MoveToWanderTargetGoalTest, MutexFlags)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 2.0, 0.35);

    // MoveToWanderTargetGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(MoveToWanderTargetGoalTest, ShouldNotExecuteWithoutTarget)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 2.0, 0.35);

    // 没有游荡目标时不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(MoveToWanderTargetGoalTest, ShouldExecuteWithDistantTarget)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 10.0, 0.35);

    // 设置一个远处的游荡目标
    m_trader->setWanderTarget(BlockPos(50, 64, 50));

    // 目标距离超过阈值，应该执行
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(MoveToWanderTargetGoalTest, ShouldNotExecuteWithNearTarget)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 2.0, 0.35);

    // 设置一个近处的游荡目标
    m_trader->setWanderTarget(BlockPos(1, 64, 1));

    // 目标距离不超过阈值，不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// UseItemGoal Tests
// ============================================================================

class UseItemGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityId(1));
        m_trader->setWorld(m_world.get());
        m_trader->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_trader.reset();
        m_world.reset();
    }

    std::unique_ptr<TestWanderingTraderWorld> m_world;
    std::unique_ptr<WanderingTraderEntity> m_trader;
};

TEST_F(UseItemGoalTest, Construction)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "UseItemGoal");
}

TEST_F(UseItemGoalTest, MutexFlags)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // UseItemGoal 应该只有 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(UseItemGoalTest, ShouldExecuteWithTrueCondition)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    // 条件始终为真
    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(UseItemGoalTest, ShouldNotExecuteWithFalseCondition)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    // 条件始终为假
    auto condition = [](MobEntity*) -> bool { return false; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(UseItemGoalTest, ShouldNotExecuteDuringCooldown)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // 首次执行应该成功
    EXPECT_TRUE(goal->shouldExecute());

    // 执行一次
    goal->startExecuting();

    // 模拟使用完成
    for (int i = 0; i < 32; ++i) {
        goal->tick();
    }
    goal->resetTask();

    // 冷却期间不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(UseItemGoalTest, NightTimeCondition)
{
    using namespace entity::ai::goal::wandering_trader;

    // 条件：只在夜间执行（dayTime >= 12000）
    auto nightCondition = [](MobEntity* mob) -> bool {
        if (mob == nullptr || mob->world() == nullptr) {
            return false;
        }
        return !mob->world()->isDaytime();
    };

    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, nightCondition);

    // 白天（dayTime = 6000）不应该执行
    m_world->setDayTime(6000);
    EXPECT_FALSE(goal->shouldExecute());

    // 夜间（dayTime = 18000）应该执行
    m_world->setDayTime(18000);
    EXPECT_TRUE(goal->shouldExecute());
}

// ============================================================================
// WanderingTraderEntity Tests
// ============================================================================

class WanderingTraderEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityId(1));
        m_trader->setWorld(m_world.get());
        m_trader->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_trader.reset();
        m_world.reset();
    }

    std::unique_ptr<TestWanderingTraderWorld> m_world;
    std::unique_ptr<WanderingTraderEntity> m_trader;
};

TEST_F(WanderingTraderEntityTest, DespawnDelay)
{
    // 测试消失倒计时
    m_trader->setDespawnDelay(1000);
    EXPECT_EQ(m_trader->despawnDelay(), 1000);

    m_trader->setDespawnDelay(0);
    EXPECT_TRUE(m_trader->canDespawn());
}

TEST_F(WanderingTraderEntityTest, LlamaCount)
{
    // 测试羊驼数量
    m_trader->setLlamaCount(2);
    EXPECT_EQ(m_trader->llamaCount(), 2);
}

TEST_F(WanderingTraderEntityTest, WanderTarget)
{
    // 测试游荡目标
    BlockPos target(100, 64, 200);
    m_trader->setWanderTarget(target);
    EXPECT_EQ(m_trader->wanderTarget(), target);
}

TEST_F(WanderingTraderEntityTest, RestockTradesWithNoOffers)
{
    // 测试没有交易时的补充
    m_trader->restockTrades();
    // 不应该崩溃
    SUCCEED();
}

TEST_F(WanderingTraderEntityTest, SpawnLlamasWithNoWorld)
{
    // 测试没有世界时的羊驼生成
    m_trader->setWorld(nullptr);
    m_trader->spawnLlamas();
    // 不应该崩溃
    SUCCEED();
}

TEST_F(WanderingTraderEntityTest, SpawnLlamasAlreadySpawned)
{
    // 测试已经生成过羊驼的情况
    m_trader->setLlamaCount(2);
    // 先生成一次
    m_trader->spawnLlamas();
    // 再次调用不应该崩溃
    m_trader->spawnLlamas();
    SUCCEED();
}

} // namespace
} // namespace mc
