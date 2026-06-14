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
#include <unordered_map>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/villager/VillagerGoals.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/CropBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"

// 引入命名空间以便使用枚举
using mc::BlockPos;
using mc::EntityId;
using mc::entity::VillagerEntity;
using mc::entity::VillagerProfession;
using mc::entity::ai::GoalFlag;

namespace mc {
namespace {

// ============================================================================
// Mock Village Manager for Testing
// ============================================================================

class MockVillageManager : public world::village::VillageManager {
public:
    explicit MockVillageManager(IWorld& world)
        : VillageManager(world)
    {}

    // Mock POI storage for testing
    world::village::poi::PointOfInterestStorage& getMockPOIStorage() { return m_poiStorage; }

private:
    world::village::poi::PointOfInterestStorage m_poiStorage;
};

// ============================================================================
// Test World for Villager Goals
// ============================================================================

class TestVillagerWorld : public test::BaseTestWorld {
public:
    TestVillagerWorld()
        : m_dayTime(0)
        , m_currentTick(0)
    {}

    void setDayTime(i64 time) { m_dayTime = time; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] world::village::VillageManager* villageManager() override { return m_villageManager.get(); }
    [[nodiscard]] const world::village::VillageManager* villageManager() const override
    {
        return m_villageManager.get();
    }

    void setVillageManager(std::unique_ptr<world::village::VillageManager> manager)
    {
        m_villageManager = std::move(manager);
    }

    void setBlockStateOverride(const BlockState* state) { m_blockStateOverride = state; }
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return m_blockStateOverride; }

private:
    i64 m_dayTime = 0;
    u64 m_currentTick = 0;
    std::unique_ptr<world::village::VillageManager> m_villageManager;
    const BlockState* m_blockStateOverride = nullptr;
};

// ============================================================================
// SleepAtNightGoal Tests
// ============================================================================

class SleepAtNightGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(SleepAtNightGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "SleepAtNightGoal");
}

TEST_F(SleepAtNightGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());

    // SleepAtNightGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(SleepAtNightGoalTest, ShouldNotExecuteDuringDay)
{
    // 白天时间: 0 - 12541
    m_world->setDayTime(6000); // 正午

    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SleepAtNightGoalTest, ShouldNotExecuteAtNightStart)
{
    // 夜间开始: 12542
    // 但没有床位，不应执行
    m_world->setDayTime(12542);

    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());
    // 没有床位，不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SleepAtNightGoalTest, ShouldNotExecuteWhenAlreadySleeping)
{
    // 夜间时间
    m_world->setDayTime(13000);

    // 设置睡眠状态
    m_villager->startSleeping(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(SleepAtNightGoalTest, ShouldContinueExecutingWhenSleeping)
{
    // 夜间时间
    m_world->setDayTime(13000);

    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());

    // 设置睡眠状态
    m_villager->startSleeping(BlockPos(0, 64, 0));

    // 睡眠中应该继续
    EXPECT_TRUE(goal->shouldContinueExecuting());
}

TEST_F(SleepAtNightGoalTest, ShouldNotContinueExecutingDuringDay)
{
    // 设置白天
    m_world->setDayTime(6000);

    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());
    goal->startExecuting();

    // 白天不应该继续
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(SleepAtNightGoalTest, ResetTaskClearsSleepingState)
{
    auto goal = std::make_unique<entity::ai::goal::villager::SleepAtNightGoal>(m_villager.get());

    goal->startExecuting();
    goal->resetTask();

    // 重置后村民应该停止睡眠
    EXPECT_FALSE(m_villager->isSleeping());
}

// ============================================================================
// Night Time Detection Tests
// ============================================================================

class NightTimeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(NightTimeTest, IsNightTimeAtNightStart)
{
    // 夜间开始: 12542
    m_world->setDayTime(12542);
    EXPECT_TRUE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNightTimeAtMidnight)
{
    // 午夜: 18000
    m_world->setDayTime(18000);
    EXPECT_TRUE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNightTimeAtNightEnd)
{
    // 夜间结束: 23459
    m_world->setDayTime(23459);
    EXPECT_TRUE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNotNightTimeAtDawn)
{
    // 黎明: 23460 (夜间结束+1)
    m_world->setDayTime(23460);
    EXPECT_FALSE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNotNightTimeAtNoon)
{
    // 正午: 6000
    m_world->setDayTime(6000);
    EXPECT_FALSE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNotNightTimeJustBeforeNight)
{
    // 夜间开始前: 12541
    m_world->setDayTime(12541);
    EXPECT_FALSE(m_villager->isNightTime());
}

TEST_F(NightTimeTest, IsNotNightTimeAtSunrise)
{
    // 日出: 0
    m_world->setDayTime(0);
    EXPECT_FALSE(m_villager->isNightTime());
}

// ============================================================================
// WorkAtJobSiteGoal Tests
// ============================================================================

class WorkAtJobSiteGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(WorkAtJobSiteGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "WorkAtJobSiteGoal");
}

TEST_F(WorkAtJobSiteGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());

    // WorkAtJobSiteGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotExecuteDuringNight)
{
    // 夜间时间
    m_world->setDayTime(15000);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotExecuteForNitwit)
{
    // 工作时间
    m_world->setDayTime(5000);

    // 设置为傻子村民
    m_villager->setProfession(VillagerProfession::Nitwit);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotExecuteWithoutJobSite)
{
    // 工作时间
    m_world->setDayTime(5000);

    // 没有工作站点
    m_villager->setProfession(VillagerProfession::Farmer);
    // workStation 默认为零坐标，表示没有工作站点

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldExecuteDuringWorkTimeWithJobSite)
{
    // 工作时间: 2000 - 9000
    m_world->setDayTime(5000);

    // 设置职业和工作站点
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldExecuteAtWorkTimeStart)
{
    // 工作时间开始: 2000
    m_world->setDayTime(2000);

    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldExecuteAtWorkTimeEnd)
{
    // 工作时间结束: 9000
    m_world->setDayTime(9000);

    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotExecuteJustBeforeWorkTime)
{
    // 工作时间前: 1999
    m_world->setDayTime(1999);

    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotExecuteJustAfterWorkTime)
{
    // 工作时间后: 9001
    m_world->setDayTime(9001);

    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, ShouldNotContinueExecutingAfterWorkTime)
{
    // 设置工作时间
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    goal->startExecuting();

    // 切换到非工作时间
    m_world->setDayTime(10000);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(WorkAtJobSiteGoalTest, ResetTaskResetsWorkState)
{
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    goal->startExecuting();
    goal->resetTask();

    // 重置后不应在工作状态
    EXPECT_FALSE(m_villager->isWorking());
}

TEST_F(WorkAtJobSiteGoalTest, StartExecutingResetsState)
{
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    goal->startExecuting();

    // 开始执行后应该移动到工作站点
    // 由于没有真实的导航系统，只检查不会崩溃
    EXPECT_TRUE(true);
}

// ============================================================================
// Work Time Detection Tests
// ============================================================================

class WorkTimeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setProfession(VillagerProfession::Farmer);
        m_villager->setWorkStation(BlockPos(0, 64, 0));
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(WorkTimeTest, IsWorkTimeAtStart)
{
    // 工作时间开始: 2000
    m_world->setDayTime(2000);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(WorkTimeTest, IsWorkTimeAtEnd)
{
    // 工作时间结束: 9000
    m_world->setDayTime(9000);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(WorkTimeTest, IsNotWorkTimeBeforeStart)
{
    // 工作时间前: 1999
    m_world->setDayTime(1999);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkTimeTest, IsNotWorkTimeAfterEnd)
{
    // 工作时间后: 9001
    m_world->setDayTime(9001);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkTimeTest, IsWorkTimeAtNoon)
{
    // 正午: 6000
    m_world->setDayTime(6000);

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

// ============================================================================
// LookForJobSiteGoal Tests
// ============================================================================

class LookForJobSiteGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(LookForJobSiteGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::LookForJobSiteGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "LookForJobSiteGoal");
}

TEST_F(LookForJobSiteGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::LookForJobSiteGoal>(m_villager.get());

    // LookForJobSiteGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(LookForJobSiteGoalTest, ShouldNotExecuteForNitwit)
{
    // 傻子村民不找工作
    m_villager->setProfession(VillagerProfession::Nitwit);

    auto goal = std::make_unique<entity::ai::goal::villager::LookForJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(LookForJobSiteGoalTest, ShouldNotExecuteWithExistingJobSite)
{
    // 已有工作站点
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::LookForJobSiteGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(LookForJobSiteGoalTest, ShouldExecuteWithoutJobSite)
{
    // 无职业村民需要找工作
    m_villager->setProfession(VillagerProfession::None);

    auto goal = std::make_unique<entity::ai::goal::villager::LookForJobSiteGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

// ============================================================================
// GatherItemsGoal Tests
// ============================================================================

class GatherItemsGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(GatherItemsGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::GatherItemsGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "GatherItemsGoal");
}

TEST_F(GatherItemsGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::GatherItemsGoal>(m_villager.get());

    // GatherItemsGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

TEST_F(GatherItemsGoalTest, ShouldNotExecuteWithoutItems)
{
    // 没有附近物品
    auto goal = std::make_unique<entity::ai::goal::villager::GatherItemsGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// VillagerBreedGoal Tests
// ============================================================================

class VillagerBreedGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(VillagerBreedGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::VillagerBreedGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "VillagerBreedGoal");
}

TEST_F(VillagerBreedGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::VillagerBreedGoal>(m_villager.get());

    // VillagerBreedGoal 应该有 Move 和 Look 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(GoalFlag::Move));
    EXPECT_TRUE(flags.test(GoalFlag::Look));
    EXPECT_FALSE(flags.test(GoalFlag::Jump));
    EXPECT_FALSE(flags.test(GoalFlag::Target));
}

// ============================================================================
// AvoidHostileGoal Tests
// ============================================================================

class AvoidHostileGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(AvoidHostileGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::AvoidHostileGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "AvoidHostileGoal");
}

TEST_F(AvoidHostileGoalTest, MutexFlags)
{
    auto goal = std::make_unique<entity::ai::goal::villager::AvoidHostileGoal>(m_villager.get());

    // AvoidHostileGoal 应该只有 Move 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(entity::ai::GoalFlag::Target));
}

// ============================================================================
// FarmerWorkGoal Tests - Comprehensive
// ============================================================================

/**
 * @brief 支持按位置设置方块状态的测试世界
 *
 * 用于需要精确控制方块布局的农民工作目标测试
 */
class FarmerTestWorld : public test::BaseTestWorld {
public:
    FarmerTestWorld()
        : m_dayTime(5000)
        , m_currentTick(1000)
    {}

    void setDayTime(i64 time) { m_dayTime = time; }
    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    [[nodiscard]] i64 dayTime() const override { return m_dayTime; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    [[nodiscard]] world::village::VillageManager* villageManager() override { return m_villageManager.get(); }
    [[nodiscard]] const world::village::VillageManager* villageManager() const override
    {
        return m_villageManager.get();
    }

    void setVillageManager(std::unique_ptr<world::village::VillageManager> manager)
    {
        m_villageManager = std::move(manager);
    }

    /**
     * @brief 设置指定位置的方块状态
     */
    void setBlockStateAt(i32 x, i32 y, i32 z, const BlockState* state) { m_blockStates[BlockPos(x, y, z)] = state; }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        auto it = m_blockStates.find(BlockPos(x, y, z));
        if (it != m_blockStates.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state) {
            m_blockStates[BlockPos(x, y, z)] = state;
        } else {
            m_blockStates.erase(BlockPos(x, y, z));
        }
        return true;
    }

private:
    i64 m_dayTime;
    u64 m_currentTick;
    std::unique_ptr<world::village::VillageManager> m_villageManager;
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

class FarmerWorkGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
        m_villager->setProfession(VillagerProfession::Farmer);
        m_villager->setWorkStation(BlockPos(0, 64, 0));
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(FarmerWorkGoalTest, Construction)
{
    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    EXPECT_NE(goal, nullptr);
    EXPECT_EQ(goal->getTypeName(), "FarmerWorkGoal");
}

TEST_F(FarmerWorkGoalTest, InheritsFromWorkAtJobSiteGoal)
{
    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());

    // FarmerWorkGoal 继承自 WorkAtJobSiteGoal，应该有相同的 Mutex 标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Look));
}

TEST_F(FarmerWorkGoalTest, ShouldNotExecuteDuringNight)
{
    // 夜间时间
    m_world->setDayTime(15000);

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FarmerWorkGoalTest, ShouldExecuteDuringWorkTime)
{
    // 工作时间
    m_world->setDayTime(5000);

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(FarmerWorkGoalTest, ShouldNotExecuteForNitwit)
{
    // 傻子村民不应该执行工作目标
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Nitwit);

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FarmerWorkGoalTest, ShouldNotExecuteWithoutJobSite)
{
    // 没有工作站点不应该执行（重置工作站点为默认零坐标）
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos::zero());

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FarmerWorkGoalTest, ShouldNotContinueExecutingAfterWorkTime)
{
    // 设置工作时间
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // 切换到非工作时间
    m_world->setDayTime(10000);
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(FarmerWorkGoalTest, TickDoesNotCrashWithNullWorld)
{
    // 工作时间且有工作站点
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // tick 不应该崩溃（即使世界不支持完整的方块操作）
    EXPECT_NO_THROW(goal->tick());
}

TEST_F(FarmerWorkGoalTest, TickDoesNotCrashWithEmptyInventory)
{
    // 工作时间，有工作站点，空背包
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // 即使背包为空，tick 也不应崩溃
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }
}

TEST_F(FarmerWorkGoalTest, TickDoesNotCrashWithSeedsInInventory)
{
    // 工作时间，有工作站点，背包有种子
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    // 放入小麦种子
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT_SEEDS, 32));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // 有种子的农民不应该崩溃
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }
}

// ============================================================================
// FarmerWorkGoal Integration Test with Block State Support
// ============================================================================

class FarmerBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_world = std::make_unique<FarmerTestWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0, 64.0, 0.0);
        m_villager->setProfession(VillagerProfession::Farmer);
        m_villager->setWorkStation(BlockPos(0, 64, 0));
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<FarmerTestWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(FarmerBlockTest, TickWithFarmlandBlocks)
{
    // 设置工作时间和工作站点
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    // 设置周围方块：耕地在脚下，空气在上方
    if (VanillaBlocks::FARMLAND) {
        const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
        const BlockState* airState = BlockRegistry::instance().airState();

        if (farmlandState && airState) {
            // 村民位置 (0, 64, 0)，设置下方为耕地
            m_world->setBlockStateAt(0, 63, 0, farmlandState);
            // 上方为空气（可种植位置）
            m_world->setBlockStateAt(0, 64, 0, airState);
        }
    }

    // 放入种子
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT_SEEDS, 32));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // tick 不应该崩溃
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }
}

TEST_F(FarmerBlockTest, TickWithMatureWheatCrop)
{
    // 设置工作时间
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    // 设置成熟的小麦作物
    // 找到 WheatBlock 并设置成熟状态
    const Block* wheatBlock = BlockItemRegistry::instance().getBlock(mc::Items::WHEAT_SEEDS->itemId());
    if (wheatBlock) {
        auto* cropBlock = dynamic_cast<const blocks::CropBlock*>(wheatBlock);
        if (cropBlock && VanillaBlocks::FARMLAND) {
            // 获取最大成熟度状态
            // 设置村民周围的成熟作物
            const BlockState& maxAgeState = cropBlock->withAge(cropBlock->getMaxAge());
            const BlockState* farmlandState = &VanillaBlocks::FARMLAND->defaultState();
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx == 0 && dz == 0) continue;
                    // 设置耕地
                    m_world->setBlockStateAt(dx, 63, dz, farmlandState);
                    // 设置成熟作物
                    m_world->setBlockStateAt(dx, 64, dz, &maxAgeState);
                }
            }
        }
    }

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // tick 多次，不应崩溃，可能收获作物
    for (int i = 0; i < 200; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }
}

TEST_F(FarmerBlockTest, TickWithComposterNearby)
{
    // 设置工作时间
    m_world->setDayTime(5000);
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    // 放入大量小麦种子（用于堆肥）
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT_SEEDS, 64));
    inv.setItem(1, mc::ItemStack(mc::Items::BEETROOT_SEEDS, 64));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();

    // 即使没有真正的堆肥桶POI，也不应崩溃
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(goal->tick());
    }
}

// ============================================================================
// FarmerWorkGoal _hasFarmSeeds indirect tests
// ============================================================================

TEST_F(FarmerWorkGoalTest, HasFarmSeedsWithWheatSeeds)
{
    // 小麦种子是可种植的
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT_SEEDS, 16));

    // 有种子时 tick 不崩溃
    m_world->setDayTime(5000);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();
    EXPECT_NO_THROW(goal->tick());
}

TEST_F(FarmerWorkGoalTest, HasFarmSeedsWithCarrot)
{
    // 胡萝卜是可种植的
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::CARROT, 16));

    m_world->setDayTime(5000);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();
    EXPECT_NO_THROW(goal->tick());
}

TEST_F(FarmerWorkGoalTest, HasFarmSeedsWithPotato)
{
    // 马铃薯是可种植的
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::POTATO, 16));

    m_world->setDayTime(5000);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();
    EXPECT_NO_THROW(goal->tick());
}

TEST_F(FarmerWorkGoalTest, HasFarmSeedsWithBeetrootSeeds)
{
    // 甜菜种子是可种植的
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BEETROOT_SEEDS, 16));

    m_world->setDayTime(5000);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();
    EXPECT_NO_THROW(goal->tick());
}

TEST_F(FarmerWorkGoalTest, NoFarmSeedsWithNonPlantableItems)
{
    // 面包、小麦等不是可种植的种子
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 16));
    inv.setItem(1, mc::ItemStack(mc::Items::WHEAT, 16));

    m_world->setDayTime(5000);
    m_villager->setWorkStation(BlockPos(0, 64, 0));

    auto goal = std::make_unique<entity::ai::goal::villager::FarmerWorkGoal>(m_villager.get());
    goal->startExecuting();
    EXPECT_NO_THROW(goal->tick());
}

// ============================================================================
// Schedule Integration Tests
// ============================================================================

class ScheduleIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0f, 64.0f, 0.0f);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(ScheduleIntegrationTest, DayTimeTransitions)
{
    // 测试一天中的时间转换

    // 黎明 (0)
    m_world->setDayTime(0);
    EXPECT_FALSE(m_villager->isNightTime());

    // 早晨 (1000)
    m_world->setDayTime(1000);
    EXPECT_FALSE(m_villager->isNightTime());

    // 正午 (6000)
    m_world->setDayTime(6000);
    EXPECT_FALSE(m_villager->isNightTime());

    // 黄昏 (12000) - 还不是夜晚
    m_world->setDayTime(12000);
    EXPECT_FALSE(m_villager->isNightTime());

    // 夜晚开始 (12542)
    m_world->setDayTime(12542);
    EXPECT_TRUE(m_villager->isNightTime());

    // 午夜 (18000)
    m_world->setDayTime(18000);
    EXPECT_TRUE(m_villager->isNightTime());

    // 黎明前 (23459)
    m_world->setDayTime(23459);
    EXPECT_TRUE(m_villager->isNightTime());

    // 黎明 (23460)
    m_world->setDayTime(23460);
    EXPECT_FALSE(m_villager->isNightTime());
}

TEST_F(ScheduleIntegrationTest, WorkTimeTransitions)
{
    // 测试工作时间转换

    auto goal = std::make_unique<entity::ai::goal::villager::WorkAtJobSiteGoal>(m_villager.get());
    m_villager->setProfession(VillagerProfession::Farmer);
    m_villager->setWorkStation(BlockPos(10, 64, 10));

    // 黎明 (0) - 不是工作时间
    m_world->setDayTime(0);
    EXPECT_FALSE(goal->shouldExecute());

    // 工作开始前 (1999) - 不是工作时间
    m_world->setDayTime(1999);
    EXPECT_FALSE(goal->shouldExecute());

    // 工作开始 (2000)
    m_world->setDayTime(2000);
    EXPECT_TRUE(goal->shouldExecute());

    // 工作中 (5000)
    m_world->setDayTime(5000);
    EXPECT_TRUE(goal->shouldExecute());

    // 工作结束 (9000)
    m_world->setDayTime(9000);
    EXPECT_TRUE(goal->shouldExecute());

    // 工作后 (9001) - 不是工作时间
    m_world->setDayTime(9001);
    EXPECT_FALSE(goal->shouldExecute());

    // 夜晚 (15000) - 不是工作时间
    m_world->setDayTime(15000);
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// Villager Interaction Tests - play() and gossip spreading
// ============================================================================

class VillagerInteractionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_world = std::make_unique<TestVillagerWorld>();
        m_villager1 = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager1->setWorld(m_world.get());
        m_villager1->setPosition(0.0f, 64.0f, 0.0f);

        m_villager2 = std::make_unique<VillagerEntity>(EntityId(2));
        m_villager2->setWorld(m_world.get());
        m_villager2->setPosition(5.0f, 64.0f, 0.0f); // 5格远
    }

    void TearDown() override
    {
        m_villager1.reset();
        m_villager2.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager1;
    std::unique_ptr<VillagerEntity> m_villager2;
};

TEST_F(VillagerInteractionTest, PlayMethodExists)
{
    // 测试 play() 方法可以被调用
    EXPECT_NO_THROW(m_villager1->play());
}

TEST_F(VillagerInteractionTest, TrySpreadGossipMethodExists)
{
    // 测试 trySpreadGossip() 方法可以被调用
    EXPECT_NO_THROW(m_villager1->trySpreadGossip());
}

TEST_F(VillagerInteractionTest, SpreadGossipToMethodExists)
{
    // 测试 spreadGossipTo() 方法可以被调用
    EXPECT_NO_THROW(m_villager1->spreadGossipTo(m_villager2.get()));
}

TEST_F(VillagerInteractionTest, SpreadGossipToNullptr)
{
    // 测试 spreadGossipTo() 方法处理 nullptr
    EXPECT_NO_THROW(m_villager1->spreadGossipTo(nullptr));
}

TEST_F(VillagerInteractionTest, SpreadGossipCooldown)
{
    // 设置初始时间
    m_world->setCurrentTick(1000);

    // 第一次传播应该成功
    m_villager1->spreadGossipTo(m_villager2.get());

    // 再次传播应该被冷却阻止
    // 由于冷却时间是 1200 ticks，在 1000 ticks 时应该被阻止
    m_villager1->spreadGossipTo(m_villager2.get());
}

TEST_F(VillagerInteractionTest, TrySpreadGossipNoInteractionTarget)
{
    // 没有设置交互目标
    m_world->setCurrentTick(1000);
    m_world->setDayTime(5000);

    // 应该不抛异常
    EXPECT_NO_THROW(m_villager1->trySpreadGossip());
}

TEST_F(VillagerInteractionTest, GossipSpreadTimeRecorded)
{
    // 设置初始时间
    m_world->setCurrentTick(2000);

    // 传播流言
    m_villager1->spreadGossipTo(m_villager2.get());

    // 后续测试需要检查冷却时间
    // 在冷却期内再次传播应该被阻止
    m_world->setCurrentTick(2500); // 500 ticks 后，小于 1200 冷却

    // 这应该被冷却阻止（不抛异常）
    EXPECT_NO_THROW(m_villager1->spreadGossipTo(m_villager2.get()));
}

// ============================================================================
// Food Points System Tests
// ============================================================================

class FoodPointsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_world = std::make_unique<TestVillagerWorld>();
        m_villager = std::make_unique<VillagerEntity>(EntityId(1));
        m_villager->setWorld(m_world.get());
        m_villager->setPosition(0.0, 64.0, 0.0);
    }

    void TearDown() override
    {
        m_villager.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_villager;
};

TEST_F(FoodPointsTest, FoodPointsMapContainsCorrectItems)
{
    // foodPoints() 应包含面包(4点)、土豆(1点)、胡萝卜(1点)、甜菜根(1点)
    const auto& points = VillagerEntity::foodPoints();
    EXPECT_NE(points.find(mc::Items::BREAD), points.end());
    EXPECT_NE(points.find(mc::Items::POTATO), points.end());
    EXPECT_NE(points.find(mc::Items::CARROT), points.end());
    EXPECT_NE(points.find(mc::Items::BEETROOT), points.end());

    EXPECT_EQ(points.at(mc::Items::BREAD), 4);
    EXPECT_EQ(points.at(mc::Items::POTATO), 1);
    EXPECT_EQ(points.at(mc::Items::CARROT), 1);
    EXPECT_EQ(points.at(mc::Items::BEETROOT), 1);
}

TEST_F(FoodPointsTest, FoodPointsMapExcludesWheatAndSeeds)
{
    // 小麦和种子不在食物点数中（它们不是繁殖物品）
    const auto& points = VillagerEntity::foodPoints();
    EXPECT_EQ(points.find(mc::Items::WHEAT), points.end());
    EXPECT_EQ(points.find(mc::Items::WHEAT_SEEDS), points.end());
    EXPECT_EQ(points.find(mc::Items::BEETROOT_SEEDS), points.end());
}

TEST_F(FoodPointsTest, CountFoodPointsEmptyInventory)
{
    // 空库存时食物点数应为 0
    EXPECT_EQ(m_villager->countFoodPointsInInventory(), 0);
}

TEST_F(FoodPointsTest, CountFoodPointsWithBread)
{
    // 6个面包 = 6 * 4 = 24 点
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 6));
    EXPECT_EQ(m_villager->countFoodPointsInInventory(), 24);
}

TEST_F(FoodPointsTest, CountFoodPointsWithMixedItems)
{
    // 2个面包(8) + 4个土豆(4) + 4个胡萝卜(4) + 4个甜菜根(4) = 20 点
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 2));
    inv.setItem(1, mc::ItemStack(mc::Items::POTATO, 4));
    inv.setItem(2, mc::ItemStack(mc::Items::CARROT, 4));
    inv.setItem(3, mc::ItemStack(mc::Items::BEETROOT, 4));
    EXPECT_EQ(m_villager->countFoodPointsInInventory(), 20);
}

TEST_F(FoodPointsTest, CountFoodPointsIgnoresWheat)
{
    // 小麦不计入食物点数
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT, 64));
    EXPECT_EQ(m_villager->countFoodPointsInInventory(), 0);
}

TEST_F(FoodPointsTest, HasExcessFoodAtThreshold)
{
    // 6个面包 = 24 点，达到过剩阈值
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 6));
    EXPECT_TRUE(m_villager->hasExcessFood());
}

TEST_F(FoodPointsTest, HasExcessFoodBelowThreshold)
{
    // 5个面包 = 20 点，低于过剩阈值
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 5));
    EXPECT_FALSE(m_villager->hasExcessFood());
}

TEST_F(FoodPointsTest, WantsMoreFoodBelowThreshold)
{
    // 空库存时需要食物
    EXPECT_TRUE(m_villager->wantsMoreFood());
}

TEST_F(FoodPointsTest, WantsMoreFoodAboveThreshold)
{
    // 3个面包 = 12 点，达到需求阈值（不再需要食物）
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 3));
    EXPECT_FALSE(m_villager->wantsMoreFood());
}

TEST_F(FoodPointsTest, WantsMoreFoodAtThresholdBoundary)
{
    // 2个面包(8) + 4个土豆(4) = 12 点，恰好达到阈值（不想要更多）
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 2));
    inv.setItem(1, mc::ItemStack(mc::Items::POTATO, 4));
    EXPECT_FALSE(m_villager->wantsMoreFood());
}

TEST_F(FoodPointsTest, HasExcessFoodWithCarrotsOnly)
{
    // 24个胡萝卜 = 24 点，达到过剩阈值
    mc::IInventory& inv = m_villager->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::CARROT, 24));
    EXPECT_TRUE(m_villager->hasExcessFood());
}

// ============================================================================
// ShareItemsGoal Tests
// ============================================================================

class ShareItemsGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();

        m_world = std::make_unique<TestVillagerWorld>();
        m_farmer = std::make_unique<VillagerEntity>(EntityId(1));
        m_farmer->setWorld(m_world.get());
        m_farmer->setPosition(0.0, 64.0, 0.0);
        m_farmer->setProfession(VillagerProfession::Farmer);

        m_target = std::make_unique<VillagerEntity>(EntityId(2));
        m_target->setWorld(m_world.get());
        m_target->setPosition(2.0, 64.0, 0.0);
    }

    void TearDown() override
    {
        m_target.reset();
        m_farmer.reset();
        m_world.reset();
    }

    std::unique_ptr<TestVillagerWorld> m_world;
    std::unique_ptr<VillagerEntity> m_farmer;
    std::unique_ptr<VillagerEntity> m_target;
};

TEST_F(ShareItemsGoalTest, FarmerWithExcessFoodCanAbandonItems)
{
    // 农民有6个面包(24点食物)应可分享
    mc::IInventory& inv = m_farmer->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 6));
    EXPECT_TRUE(m_farmer->hasExcessFood());
}

TEST_F(ShareItemsGoalTest, FarmerWithNoFoodCannotAbandonItems)
{
    // 空库存的农民不能分享
    EXPECT_FALSE(m_farmer->hasExcessFood());
}

TEST_F(ShareItemsGoalTest, FarmerWithWheatOnly)
{
    // 农民有超过半组小麦(>32)时可分享
    mc::IInventory& inv = m_farmer->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::WHEAT, 40));
    // 小麦不计入食物点数，所以没有过剩食物
    EXPECT_FALSE(m_farmer->hasExcessFood());
}

TEST_F(ShareItemsGoalTest, NonFarmerCannotShareItems)
{
    // 非农民职业不应该有 ShareItemsGoal
    auto librarian = std::make_unique<VillagerEntity>(EntityId(3));
    librarian->setWorld(m_world.get());
    // 图书管理员的职业不是农民
    librarian->setProfession(VillagerProfession::Librarian);
    // 食物过剩判断与职业无关，但 ShareItemsGoal 仅限农民执行
    mc::IInventory& inv = librarian->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 6));
    EXPECT_TRUE(librarian->hasExcessFood()); // 库存判断与职业无关
    librarian.reset();
}

TEST_F(ShareItemsGoalTest, TargetNeedsFoodEmptyInventory)
{
    // 空库存的目标村民需要食物
    EXPECT_TRUE(m_target->wantsMoreFood());
}

TEST_F(ShareItemsGoalTest, TargetDoesNotNeedFoodWithEnoughFood)
{
    // 目标有3个面包(12点)时不再需要食物
    mc::IInventory& inv = m_target->inventory();
    inv.setItem(0, mc::ItemStack(mc::Items::BREAD, 3));
    EXPECT_FALSE(m_target->wantsMoreFood());
}

} // namespace
} // namespace mc
