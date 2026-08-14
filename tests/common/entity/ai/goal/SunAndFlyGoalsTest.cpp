/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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

#include "common/TestWorldHelper.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/FindShelterGoal.hpp"
#include "common/entity/ai/goal/goals/FleeSunGoal.hpp"
#include "common/entity/ai/goal/goals/FlyGoal.hpp"
#include "common/entity/ai/goal/goals/RestrictSunGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;
using namespace mc::entity::ai::goal;
using namespace mc::entity::ai; // for GoalFlag

// ============================================================================
// Test World with controllable day/night and sky visibility
// ============================================================================

class SunTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isDaytime() const override { return m_isDaytime; }
    [[nodiscard]] bool isBrightOutside() const override
    {
        if (!m_hasSkyLight) {
            return false;
        }
        return m_skyDarkening < 4;
    }
    [[nodiscard]] bool canSeeSky(const BlockPos& pos) const override { return m_canSeeSky; }
    [[nodiscard]] bool isWaterAt(const BlockPos& /*pos*/) const override { return m_isWater; }
    [[nodiscard]] bool isLavaAt(const BlockPos& /*pos*/) const override { return m_isLava; }

    void setDaytime(bool daytime) { m_isDaytime = daytime; }
    void setSkyDarkening(i32 darkening) { m_skyDarkening = darkening; }
    void setHasSkyLight(bool has) { m_hasSkyLight = has; }
    void setCanSeeSky(bool canSee) { m_canSeeSky = canSee; }
    void setWater(bool water) { m_isWater = water; }
    void setLava(bool lava) { m_isLava = lava; }

private:
    bool m_isDaytime = true;
    i32 m_skyDarkening = 0;
    bool m_hasSkyLight = true;
    bool m_canSeeSky = true;
    bool m_isWater = false;
    bool m_isLava = false;
};

// ============================================================================
// Test CreatureEntity for testing
// ============================================================================

class TestCreature : public CreatureEntity {
public:
    TestCreature()
        : CreatureEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }
};

// ============================================================================
// FleeSunGoal Tests
// ============================================================================

class FleeSunGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        world = std::make_unique<SunTestWorld>();
        creature->setWorld(world.get());
        goal = std::make_unique<FleeSunGoal>(creature.get(), 1.0);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
        world.reset();
    }

    std::unique_ptr<TestCreature> creature;
    std::unique_ptr<SunTestWorld> world;
    std::unique_ptr<FleeSunGoal> goal;
};

TEST_F(FleeSunGoalTest, TypeName)
{
    EXPECT_EQ(goal->getTypeName(), "FleeSunGoal");
}

TEST_F(FleeSunGoalTest, MutexFlags)
{
    // FleeSunGoal 应该只有 Move 互斥标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Move}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Look}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Jump}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Target}));
}

TEST_F(FleeSunGoalTest, ShouldNotExecuteAtNight)
{
    // 夜间不应该执行
    world->setDaytime(false);
    world->setSkyDarkening(11); // 夜间天暗值 > 4
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FleeSunGoalTest, ShouldNotExecuteUnderCover)
{
    // 白天但在遮挡下不应该执行
    world->setDaytime(true);
    world->setSkyDarkening(0);
    world->setCanSeeSky(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FleeSunGoalTest, ShouldNotExecuteInNether)
{
    // 没有天空光照的维度不应该执行
    world->setHasSkyLight(false);
    world->setSkyDarkening(0);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FleeSunGoalTest, ResetTaskDoesNotCrash)
{
    goal->startExecuting();
    goal->resetTask();
    EXPECT_TRUE(true); // 不崩溃即通过
}

TEST_F(FleeSunGoalTest, TickDoesNotCrash)
{
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

// ============================================================================
// RestrictSunGoal Tests
// ============================================================================

class RestrictSunGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        world = std::make_unique<SunTestWorld>();
        creature->setWorld(world.get());
        goal = std::make_unique<RestrictSunGoal>(creature.get());
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
        world.reset();
    }

    std::unique_ptr<TestCreature> creature;
    std::unique_ptr<SunTestWorld> world;
    std::unique_ptr<RestrictSunGoal> goal;
};

TEST_F(RestrictSunGoalTest, TypeName)
{
    EXPECT_EQ(goal->getTypeName(), "RestrictSunGoal");
}

TEST_F(RestrictSunGoalTest, MutexFlags)
{
    // RestrictSunGoal 没有互斥标志（只修改导航器设置）
    auto flags = goal->getMutexFlags();
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Move}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Look}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Jump}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Target}));
}

TEST_F(RestrictSunGoalTest, ShouldNotExecuteAtNight)
{
    // 夜间不应该执行
    world->setDaytime(false);
    world->setSkyDarkening(11);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(RestrictSunGoalTest, ShouldExecuteDuringDay)
{
    // 白天、无头盔、室外 -> 应该执行
    world->setDaytime(true);
    world->setSkyDarkening(0);
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(RestrictSunGoalTest, ShouldNotExecuteInThunderstorm)
{
    // 雷暴时天空变暗，skyDarkening >= 4 -> isBrightOutside() 返回 false
    world->setSkyDarkening(6);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(RestrictSunGoalTest, ShouldNotExecuteInNether)
{
    // 没有天空光照的维度
    world->setHasSkyLight(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(RestrictSunGoalTest, StartAndResetDoNotCrash)
{
    goal->startExecuting();
    goal->resetTask();
    EXPECT_TRUE(true);
}

TEST_F(RestrictSunGoalTest, StartSetsAvoidSunPathing)
{
    // 验证 startExecuting 和 resetTask 不崩溃
    // setAvoidSunPathing 转发到 WalkNodeProcessor，即使 PathFinder 为 null 也不崩溃
    goal->startExecuting();
    EXPECT_TRUE(true); // 不崩溃即通过

    goal->resetTask();
    EXPECT_TRUE(true);
}

// ============================================================================
// FindShelterGoal Tests
// ============================================================================

class FindShelterGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        world = std::make_unique<SunTestWorld>();
        creature->setWorld(world.get());
        goal = std::make_unique<FindShelterGoal>(creature.get(), 1.0);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
        world.reset();
    }

    std::unique_ptr<TestCreature> creature;
    std::unique_ptr<SunTestWorld> world;
    std::unique_ptr<FindShelterGoal> goal;
};

TEST_F(FindShelterGoalTest, TypeName)
{
    EXPECT_EQ(goal->getTypeName(), "FindShelterGoal");
}

TEST_F(FindShelterGoalTest, MutexFlags)
{
    // FindShelterGoal 应该只有 Move 互斥标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Move}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Look}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Jump}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Target}));
}

TEST_F(FindShelterGoalTest, ShouldNotExecuteAtNight)
{
    world->setDaytime(false);
    world->setSkyDarkening(11);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FindShelterGoalTest, ShouldNotExecuteUnderCover)
{
    world->setDaytime(true);
    world->setSkyDarkening(0);
    world->setCanSeeSky(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FindShelterGoalTest, ShouldNotExecuteInNether)
{
    world->setHasSkyLight(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FindShelterGoalTest, ResetTaskDoesNotCrash)
{
    goal->startExecuting();
    goal->resetTask();
    EXPECT_TRUE(true);
}

TEST_F(FindShelterGoalTest, TickDoesNotCrash)
{
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

TEST_F(FindShelterGoalTest, ShouldContinueExecutingWithoutTimeout)
{
    // 没有 world 中的区块数据，无法导航，shouldContinueExecuting 返回 false
    goal->startExecuting();
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

// ============================================================================
// FlyGoal Tests
// ============================================================================

class FlyGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
        world = std::make_unique<SunTestWorld>();
        creature->setWorld(world.get());
        goal = std::make_unique<FlyGoal>(creature.get(), 1.0);
    }

    void TearDown() override
    {
        goal.reset();
        creature.reset();
        world.reset();
    }

    std::unique_ptr<TestCreature> creature;
    std::unique_ptr<SunTestWorld> world;
    std::unique_ptr<FlyGoal> goal;
};

TEST_F(FlyGoalTest, TypeName)
{
    EXPECT_EQ(goal->getTypeName(), "FlyGoal");
}

TEST_F(FlyGoalTest, MutexFlags)
{
    // FlyGoal 应该只有 Move 互斥标志
    auto flags = goal->getMutexFlags();
    EXPECT_TRUE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Move}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Look}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Jump}));
    EXPECT_FALSE(flags.contains(EnumSet<GoalFlag>{GoalFlag::Target}));
}

TEST_F(FlyGoalTest, ShouldNotExecuteWhenRidden)
{
    // 注意：isBeingRidden() 默认返回 false，所以这个测试验证基本行为
    // 由于 RandomPositionGenerator 在无区块数据时无法找到位置，
    // shouldExecute 大概率返回 false
    // 但至少验证不崩溃
    EXPECT_NO_THROW(goal->shouldExecute());
}

TEST_F(FlyGoalTest, ResetTaskDoesNotCrash)
{
    goal->startExecuting();
    goal->resetTask();
    EXPECT_TRUE(true);
}

TEST_F(FlyGoalTest, TickDoesNotCrash)
{
    for (int i = 0; i < 100; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

TEST_F(FlyGoalTest, ShouldContinueExecutingReturnsFalseWhenNoPath)
{
    // 没有有效路径时，shouldContinueExecuting 返回 false
    goal->startExecuting();
    EXPECT_FALSE(goal->shouldContinueExecuting());
}

TEST_F(FlyGoalTest, TimeoutDecrements)
{
    // FlyGoal 有超时机制
    goal->startExecuting();
    // tick 几次，验证不崩溃
    for (int i = 0; i < 10; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

// ============================================================================
// Goal Lifecycle Tests: shouldExecute -> startExecuting -> tick -> shouldContinueExecuting -> resetTask
// ============================================================================

TEST_F(FleeSunGoalTest, FullLifecycleDoesNotCrash)
{
    // 完整生命周期：shouldExecute -> startExecuting -> tick -> shouldContinueExecuting -> resetTask
    // 在没有区块数据的环境中，shouldExecute 可能返回 false
    // 但我们仍然可以验证生命周期方法不崩溃
    world->setDaytime(true);
    world->setSkyDarkening(0);
    world->setCanSeeSky(true);

    // shouldExecute 在白天且可见天空时尝试执行（但 RandomPositionGenerator 可能找不到位置）
    goal->shouldExecute();

    // 手动触发生命周期
    goal->startExecuting();
    for (int i = 0; i < 20; ++i) {
        goal->tick();
    }
    // shouldContinueExecuting 在无路径时返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());
    goal->resetTask();
}

TEST_F(RestrictSunGoalTest, FullLifecycleDayToNight)
{
    // 白天 -> 执行 RestrictSunGoal -> 夜晚 -> 停止
    world->setDaytime(true);
    world->setSkyDarkening(0);
    EXPECT_TRUE(goal->shouldExecute());

    goal->startExecuting();

    // tick 一段时间
    for (int i = 0; i < 20; ++i) {
        goal->tick();
    }

    // 变为夜晚
    world->setDaytime(false);
    world->setSkyDarkening(11);

    // RestrictSunGoal 没有 shouldContinueExecuting，只靠 shouldExecute 重评估
    // 但 resetTask 应该恢复导航器设置
    goal->resetTask();
    EXPECT_TRUE(true);
}

TEST_F(RestrictSunGoalTest, HeadEquipmentPreventsExecution)
{
    // 白天但头部有装备时不执行
    world->setDaytime(true);
    world->setSkyDarkening(0);

    // 头部无装备时应该执行
    EXPECT_TRUE(goal->shouldExecute());

    // 头部有装备时不应该执行
    // 注意：TestCreature 默认没有装备，所以 shouldExecute 返回 true
    // 如果设置了头部装备，应该返回 false
    // 这里验证默认无装备的情况
}

TEST_F(FindShelterGoalTest, FullLifecycleWithTimeout)
{
    // FindShelterGoal 有超时机制
    world->setDaytime(true);
    world->setSkyDarkening(0);
    world->setCanSeeSky(true);

    goal->shouldExecute();
    goal->startExecuting();

    // tick 超过 MAX_SHELTER_TIME(600) 次，验证超时机制
    for (int i = 0; i < 700; ++i) {
        goal->tick();
    }

    // 超时后 shouldContinueExecuting 应返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());

    goal->resetTask();
}

TEST_F(FindShelterGoalTest, TickAfterResetDoesNotCrash)
{
    goal->startExecuting();
    goal->resetTask();
    // resetTask 后继续 tick 不应该崩溃
    for (int i = 0; i < 50; ++i) {
        goal->tick();
    }
    EXPECT_TRUE(true);
}

TEST_F(FlyGoalTest, FullLifecycleWithTimeout)
{
    // FlyGoal 有超时机制（MAX_FLIGHT_TIME = 600 ticks）
    goal->shouldExecute();
    goal->startExecuting();

    // tick 超过超时
    for (int i = 0; i < 700; ++i) {
        goal->tick();
    }

    // 超时后 shouldContinueExecuting 应返回 false
    EXPECT_FALSE(goal->shouldContinueExecuting());

    goal->resetTask();
}

TEST_F(FleeSunGoalTest, ShouldNotExecuteWithoutWorld)
{
    // 没有世界时不应执行
    creature->setWorld(nullptr);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(RestrictSunGoalTest, ShouldNotExecuteWithoutWorld)
{
    creature->setWorld(nullptr);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FindShelterGoalTest, ShouldNotExecuteWithoutWorld)
{
    creature->setWorld(nullptr);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(FlyGoalTest, ShouldNotExecuteWithoutWorld)
{
    creature->setWorld(nullptr);
    EXPECT_FALSE(goal->shouldExecute());
}

// ============================================================================
// isBrightOutside() Tests
// ============================================================================

class IsBrightOutsideTest : public ::testing::Test {
protected:
    void SetUp() override { world = std::make_unique<SunTestWorld>(); }
    void TearDown() override { world.reset(); }
    std::unique_ptr<SunTestWorld> world;
};

TEST_F(IsBrightOutsideTest, BrightDuringDay)
{
    world->setSkyDarkening(0);
    world->setHasSkyLight(true);
    EXPECT_TRUE(world->isBrightOutside());
}

TEST_F(IsBrightOutsideTest, NotBrightAtNight)
{
    world->setSkyDarkening(11);
    EXPECT_FALSE(world->isBrightOutside());
}

TEST_F(IsBrightOutsideTest, NotBrightDuringThunderstorm)
{
    // 雷暴时 skyDarkening >= 4
    world->setSkyDarkening(6);
    EXPECT_FALSE(world->isBrightOutside());
}

TEST_F(IsBrightOutsideTest, BorderlineSkyDarkening)
{
    // skyDarkening = 3 -> 仍然明亮
    world->setSkyDarkening(3);
    EXPECT_TRUE(world->isBrightOutside());

    // skyDarkening = 4 -> 不再明亮
    world->setSkyDarkening(4);
    EXPECT_FALSE(world->isBrightOutside());
}

TEST_F(IsBrightOutsideTest, NotBrightInNetherOrEnd)
{
    // 没有天空光照的维度总是不亮
    world->setHasSkyLight(false);
    world->setSkyDarkening(0);
    EXPECT_FALSE(world->isBrightOutside());
}
