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

#include "common/world/tick/list/ServerTickList.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/tick/base/ScheduledTick.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/list/EmptyTickList.hpp"
#include "common/world/tick/list/ITickList.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <stdexcept>

using namespace mc::world::tick;

namespace {

class MockTickTarget {
public:
    explicit MockTickTarget(int value)
        : id(value)
    {}

    int id;
};

class ServerTickListTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] bool isUltraWarm() const override { return false; }
    [[nodiscard]] mc::world::tick::TickManager& tickManager() override { throw std::runtime_error("unused"); }
    [[nodiscard]] const mc::world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("unused");
    }
    [[nodiscard]] bool hasChunk(mc::ChunkCoord, mc::ChunkCoord) const override { return true; }
};

} // namespace

// ============================================================================
// TickPriority Tests
// ============================================================================

TEST(TickPriorityTest, FromIntReturnsCorrectPriority)
{
    EXPECT_EQ(fromInt(-3), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(-2), TickPriority::VeryHigh);
    EXPECT_EQ(fromInt(-1), TickPriority::High);
    EXPECT_EQ(fromInt(0), TickPriority::Normal);
    EXPECT_EQ(fromInt(1), TickPriority::Low);
    EXPECT_EQ(fromInt(2), TickPriority::VeryLow);
    EXPECT_EQ(fromInt(3), TickPriority::ExtremelyLow);
}

TEST(TickPriorityTest, FromIntClampsOutOfRange)
{
    EXPECT_EQ(fromInt(-100), TickPriority::ExtremelyHigh);
    EXPECT_EQ(fromInt(100), TickPriority::ExtremelyLow);
}

TEST(TickPriorityTest, ToIntReturnsCorrectValue)
{
    EXPECT_EQ(toInt(TickPriority::ExtremelyHigh), -3);
    EXPECT_EQ(toInt(TickPriority::VeryHigh), -2);
    EXPECT_EQ(toInt(TickPriority::High), -1);
    EXPECT_EQ(toInt(TickPriority::Normal), 0);
    EXPECT_EQ(toInt(TickPriority::Low), 1);
    EXPECT_EQ(toInt(TickPriority::VeryLow), 2);
    EXPECT_EQ(toInt(TickPriority::ExtremelyLow), 3);
}

// ============================================================================
// ScheduledTick Tests
// ============================================================================

TEST(ScheduledTickTest, Construction)
{
    mc::BlockPos pos(10, 20, 30);
    MockTickTarget target(1);

    ScheduledTick<MockTickTarget> tick(pos, &target, 100, TickPriority::Normal, 1);

    EXPECT_EQ(tick.position.x, 10);
    EXPECT_EQ(tick.position.y, 20);
    EXPECT_EQ(tick.position.z, 30);
    EXPECT_EQ(tick.target, &target);
    EXPECT_EQ(tick.scheduledTick, 100);
    EXPECT_EQ(tick.priority, TickPriority::Normal);
    EXPECT_EQ(tick.tickEntryId, 1);
}

TEST(ScheduledTickTest, ComparisonOrdersByScheduledTick)
{
    MockTickTarget target(1);

    ScheduledTick<MockTickTarget> tick1(mc::BlockPos(0, 0, 0), &target, 10, TickPriority::Normal, 1);
    ScheduledTick<MockTickTarget> tick2(mc::BlockPos(0, 0, 0), &target, 20, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, ComparisonOrdersByPriorityWhenSameTick)
{
    MockTickTarget target(1);

    ScheduledTick<MockTickTarget> tick1(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::High, 1);
    ScheduledTick<MockTickTarget> tick2(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2); // High优先级 < Normal优先级
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, ComparisonOrdersByIdWhenSameTickAndPriority)
{
    MockTickTarget target(1);

    ScheduledTick<MockTickTarget> tick1(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 1);
    ScheduledTick<MockTickTarget> tick2(mc::BlockPos(0, 0, 0), &target, 100, TickPriority::Normal, 2);

    EXPECT_TRUE(tick1 < tick2);
    EXPECT_FALSE(tick2 < tick1);
}

TEST(ScheduledTickTest, EqualityBasedOnPositionAndTarget)
{
    MockTickTarget target1(1);
    MockTickTarget target2(2);

    ScheduledTick<MockTickTarget> tick1(
        mc::BlockPos(0, 0, 0), &target1, 10, TickPriority::Normal, 1); // Same pos and target
    ScheduledTick<MockTickTarget> tick2(
        mc::BlockPos(0, 0, 0), &target1, 20, TickPriority::High, 2); // Same pos and target
    ScheduledTick<MockTickTarget> tick3(mc::BlockPos(1, 0, 0), &target1, 10, TickPriority::Normal, 3); // Different pos
    ScheduledTick<MockTickTarget> tick4(
        mc::BlockPos(0, 0, 0), &target2, 10, TickPriority::Normal, 4); // Different target

    EXPECT_TRUE(tick1 == tick2);  // Same position and target
    EXPECT_FALSE(tick1 == tick3); // Different position
    EXPECT_FALSE(tick1 == tick4); // Different target
}

TEST(ScheduledTickTest, HashCodeConsistency)
{
    MockTickTarget target(1);

    ScheduledTick<MockTickTarget> tick1(mc::BlockPos(0, 0, 0), &target, 10, TickPriority::Normal, 1);
    ScheduledTick<MockTickTarget> tick2(mc::BlockPos(0, 0, 0), &target, 20, TickPriority::High, 2);

    // Same position and target should have same hash
    EXPECT_EQ(tick1.hashCode(), tick2.hashCode());
}

// ============================================================================
// EmptyTickList Tests
// ============================================================================

TEST(EmptyTickListTest, AllOperationsReturnFalse)
{
    EmptyTickList<MockTickTarget>& tickList = EmptyTickList<MockTickTarget>::get();

    MockTickTarget target(1);
    mc::BlockPos pos(0, 0, 0);

    EXPECT_FALSE(tickList.isTickScheduled(pos, target));
    EXPECT_FALSE(tickList.isTickPending(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 0);
}

TEST(EmptyTickListTest, ScheduleDoesNothing)
{
    EmptyTickList<MockTickTarget>& tickList = EmptyTickList<MockTickTarget>::get();

    MockTickTarget target(1);
    mc::BlockPos pos(0, 0, 0);

    // Should not throw or do anything
    tickList.scheduleTick(pos, target, 10);
    tickList.scheduleTick(pos, target, 10, TickPriority::High);

    EXPECT_FALSE(tickList.isTickScheduled(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 0);
}

TEST(EmptyTickListTest, SingletonPattern)
{
    EmptyTickList<MockTickTarget>& list1 = EmptyTickList<MockTickTarget>::get();
    EmptyTickList<MockTickTarget>& list2 = EmptyTickList<MockTickTarget>::get();

    EXPECT_EQ(&list1, &list2);
}

// ============================================================================
// ServerTickList Tests
// ============================================================================

// Note: ServerTickList tests would require a mock ServerWorld
// For now, we'll test basic functionality

TEST(ServerTickListTest, Construction)
{
    // ServerTickList requires ServerWorld reference, filter, serializer, deserializer, and callback
    // We'll test the basic scheduling and cancellation logic

    // This test would require mocking ServerWorld, which is complex
    // For full integration tests, see the integration test suite
}

TEST(ServerTickListTest, TickRemovesEntryFromPendingSetBeforeExecution)
{
    ServerTickListTestWorld world;
    MockTickTarget target(1);
    size_t executionCount = 0;

    ServerTickList<MockTickTarget> tickList(
        world,
        [](MockTickTarget&) { return false; },
        [](MockTickTarget&) -> const mc::ResourceLocation& {
            static const mc::ResourceLocation id("test", "mock_target");
            return id;
        },
        [](const mc::ResourceLocation&) -> MockTickTarget* { return nullptr; },
        [&executionCount](mc::IWorld&, const mc::BlockPos&, MockTickTarget&) { ++executionCount; });

    const mc::BlockPos pos(64, 62, 87);
    tickList.setCurrentTick(20);
    tickList.scheduleTick(pos, target, 1, TickPriority::Normal);

    EXPECT_TRUE(tickList.isTickScheduled(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 1);

    tickList.tick(21, 65536);

    EXPECT_EQ(executionCount, 1);
    EXPECT_FALSE(tickList.isTickScheduled(pos, target));
    EXPECT_FALSE(tickList.isTickPending(pos, target));
    EXPECT_EQ(tickList.pendingCount(), 0);
    EXPECT_EQ(tickList.executedThisTickCount(), 1);
}
