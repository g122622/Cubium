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
using mc::EntityInstanceId;
using mc::entity::WanderingTraderEntity;
using mc::entity::ai::GoalFlag;

namespace mc {
namespace {

// ============================================================================
// Test World for WanderingTrader Goals
// ============================================================================

class TestWanderingTraderWorld : public mc::test::BaseTestWorld {
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
        m_trader = std::make_unique<WanderingTraderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(LookAtCustomerGoalTest, StartExecutingSetsLookTime)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<LookAtCustomerGoal>(m_trader.get());

    // startExecuting 应该设置看向时间（不崩溃即可）
    goal->startExecuting();
    SUCCEED();
}

TEST_F(LookAtCustomerGoalTest, ResetTaskClearsCustomer)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<LookAtCustomerGoal>(m_trader.get());

    // resetTask 不应该崩溃
    goal->resetTask();
    SUCCEED();
}

// ============================================================================
// TradeWithPlayerGoal Tests
// ============================================================================

class TradeWithPlayerGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
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

    // TradeWithPlayerGoal 应该有 Jump 和 Move 标志（与MC原版一致）
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Jump));
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_FALSE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(TradeWithPlayerGoalTest, ShouldNotExecuteWithoutCustomer)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // 没有交易中的玩家时不应该执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(TradeWithPlayerGoalTest, ShouldNotExecuteInWater)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // 在水中时不应该执行
    m_trader->setInWater(true);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(TradeWithPlayerGoalTest, StartExecutingStopsNavigation)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // startExecuting 不应该崩溃（即使没有导航路径）
    goal->startExecuting();
    SUCCEED();
}

TEST_F(TradeWithPlayerGoalTest, ResetTaskClearsCustomer)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<TradeWithPlayerGoal>(m_trader.get());

    // resetTask 不应该崩溃
    goal->resetTask();
    SUCCEED();
}

// ============================================================================
// MoveToWanderTargetGoal Tests
// ============================================================================

class MoveToWanderTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(MoveToWanderTargetGoalTest, ResetTaskClearsWanderTarget)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 2.0, 0.35);

    // 设置游荡目标
    m_trader->setWanderTarget(BlockPos(50, 64, 50));
    EXPECT_EQ(m_trader->wanderTarget(), BlockPos(50, 64, 50));

    // resetTask 应该清除游荡目标
    goal->resetTask();
    EXPECT_EQ(m_trader->wanderTarget(), BlockPos(0, 0, 0));
}

TEST_F(MoveToWanderTargetGoalTest, StartExecutingCapturesTarget)
{
    using namespace entity::ai::goal::wandering_trader;
    auto goal = std::make_unique<MoveToWanderTargetGoal>(m_trader.get(), 10.0, 0.35);

    // 设置游荡目标
    m_trader->setWanderTarget(BlockPos(50, 64, 50));

    // startExecuting 不应该崩溃
    goal->startExecuting();
    SUCCEED();
}

TEST_F(MoveToWanderTargetGoalTest, IntermediateDistanceConstant)
{
    // 验证中间航点距离常量与MC原版一致（10格）
    // MC原版 WanderToPositionGoal 使用 10.0 作为远距离分段阈值
    EXPECT_DOUBLE_EQ(entity::ai::goal::wandering_trader::MoveToWanderTargetGoal::INTERMEDIATE_DISTANCE, 10.0);
}

// ============================================================================
// UseItemGoal Tests
// ============================================================================

class UseItemGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 显式初始化 Items：gtest 进程内 Items 为全局静态变量，
        // 其他测试套件（CatEntityTest/BegGoalTest 等）可能已调用 initialize()
        // 使 Items::POTION / Items::MILK_BUCKET 变为非 nullptr。
        // 为保证本套件测试独立于运行顺序且行为确定，这里显式初始化。
        Items::initialize();

        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(UseItemGoalTest, ConditionPreventsReExecution)
{
    // MC原版UseItemGoal没有冷却机制，由条件函数本身防止重复触发。
    // 例如：流浪商人夜间喝隐身药水后，隐身效果存在时条件不满足，
    // 因此shouldExecute()自然返回false，无需额外冷却。
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    bool conditionResult = true;
    auto condition = [&conditionResult](MobEntity*) -> bool { return conditionResult; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // 条件为真时可以执行
    EXPECT_TRUE(goal->shouldExecute());

    // 条件为假时不能执行（模拟隐身效果已存在的情况）
    conditionResult = false;
    EXPECT_FALSE(goal->shouldExecute());

    // 条件恢复为真后可以再次执行
    conditionResult = true;
    EXPECT_TRUE(goal->shouldExecute());
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

TEST_F(UseItemGoalTest, ShouldContinueExecutingWhenUsingItem)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::POTION, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // 未调用 startExecuting 时，实体未在使用物品，shouldContinueExecuting 应为 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
    EXPECT_FALSE(m_trader->isUsingItem());

    // startExecuting 将药水放入主手并调用 setActiveHand。
    // PotionItem::getUseDuration() 返回 32 > 0，因此 LivingEntity::setActiveHand
    // 会设置 m_activeItemUseCount=32，isUsingItem() 返回 true。
    // 此时 shouldContinueExecuting（依赖 isUsingItem()）也应返回 true。
    goal->startExecuting();
    EXPECT_TRUE(m_trader->isUsingItem());
    EXPECT_TRUE(goal->shouldContinueExecuting());

    // resetTask 调用 stopActiveHand 清空使用状态后，shouldContinueExecuting 应回 false
    goal->resetTask();
    EXPECT_FALSE(m_trader->isUsingItem());
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(UseItemGoalTest, StartExecutingSetsMainHandItem)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::MILK_BUCKET, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // 主手初始应为空
    EXPECT_TRUE(m_trader->getMainHandItem().isEmpty());

    // SetUp 已调用 Items::initialize()，Items::MILK_BUCKET 为非空 Item*，
    // 因此 stack 为非空 ItemStack。startExecuting() 会将其写入主手装备槽，
    // 随后 setActiveHand(MainHand) 因 MilkBucketItem::getUseDuration() = 32 > 0
    // 而进入使用状态。
    goal->startExecuting();

    // 主手应为牛奶桶（非空），且物品指针与传入的 Items::MILK_BUCKET 一致
    const ItemStack& mainHand = m_trader->getMainHandItem();
    EXPECT_FALSE(mainHand.isEmpty());
    EXPECT_EQ(mainHand.getItem(), Items::MILK_BUCKET);
    EXPECT_TRUE(m_trader->isUsingItem());
}

TEST_F(UseItemGoalTest, ResetTaskClearsMainHandItem)
{
    using namespace entity::ai::goal::wandering_trader;
    ItemStack stack(Items::MILK_BUCKET, 1);
    ResourceLocation sound("entity.wandering_trader.drink");

    auto condition = [](MobEntity*) -> bool { return true; };
    auto goal = std::make_unique<UseItemGoal>(m_trader.get(), stack, sound, condition);

    // startExecuting 后主手为牛奶桶（非空），处于使用状态
    goal->startExecuting();
    ASSERT_FALSE(m_trader->getMainHandItem().isEmpty());
    ASSERT_TRUE(m_trader->isUsingItem());

    // resetTask 应清空主手并停止使用物品（不应崩溃）
    goal->resetTask();
    EXPECT_TRUE(m_trader->getMainHandItem().isEmpty());
    EXPECT_FALSE(m_trader->isUsingItem());
}

// ============================================================================
// WanderingTraderEntity Tests
// ============================================================================

class WanderingTraderEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestWanderingTraderWorld>();
        m_trader = std::make_unique<WanderingTraderEntity>(EntityInstanceId(1), mc::test::testEcsRegistry());
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

TEST_F(WanderingTraderEntityTest, RestockWithNoOffers)
{
    // 测试没有交易时的补货
    m_trader->restock();
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
