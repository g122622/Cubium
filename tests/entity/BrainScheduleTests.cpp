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

#include "common/entity/ai/brain/Brain.hpp"
#include "common/entity/ai/brain/memory/BlockPosTarget.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/memory/WalkTarget.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::entity::ai::brain::memory {
namespace {

TEST(WalkTargetTest, BlockPosConstructorUsesBlockCenter)
{
    const BlockPos blockPos(3, 70, -5);
    const WalkTarget walkTarget(blockPos, 0.85f, 2);

    ASSERT_NE(walkTarget.getTarget(), nullptr);
    EXPECT_EQ(walkTarget.getTarget()->getBlockPos(), blockPos);
    EXPECT_FLOAT_EQ(walkTarget.getTarget()->getPosition().x, 3.5f);
    EXPECT_FLOAT_EQ(walkTarget.getTarget()->getPosition().y, 70.5f);
    EXPECT_FLOAT_EQ(walkTarget.getTarget()->getPosition().z, -4.5f);
    EXPECT_FLOAT_EQ(walkTarget.getSpeed(), 0.85f);
    EXPECT_EQ(walkTarget.getDistance(), 2);
}

TEST(MemoryModuleTypesTest, WalkAndLookTargetsUseRealTypes)
{
    MemoryModuleTypes::initialize();

    Brain<int> brain;
    brain.registerMemory(MemoryModuleTypes::WALK_TARGET);
    brain.registerMemory(MemoryModuleTypes::LOOK_TARGET);

    const WalkTarget walkTarget(BlockPos(1, 64, 1), 1.0f, 1);
    const std::shared_ptr<IPositionTarget> lookTarget = std::make_shared<BlockPosTarget>(BlockPos(4, 65, 6));

    brain.setMemory(MemoryModuleTypes::WALK_TARGET, walkTarget);
    brain.setMemory(MemoryModuleTypes::LOOK_TARGET, lookTarget);

    const auto storedWalkTarget = brain.getMemory(MemoryModuleTypes::WALK_TARGET);
    const auto storedLookTarget = brain.getMemory(MemoryModuleTypes::LOOK_TARGET);

    ASSERT_TRUE(storedWalkTarget.has_value());
    ASSERT_TRUE(storedLookTarget.has_value());
    EXPECT_EQ(storedWalkTarget->getTarget()->getBlockPos(), BlockPos(1, 64, 1));
    EXPECT_EQ((*storedLookTarget)->getBlockPos(), BlockPos(4, 65, 6));
}

} // namespace
} // namespace mc::entity::ai::brain::memory

namespace mc::entity::ai::brain::schedule {
namespace {

TEST(ScheduleDutiesTest, UsesDiscreteWeightsAcrossDayBoundary)
{
    ScheduleDuties duties;
    duties.addDutyTime(2000, 1.0f);
    duties.addDutyTime(9000, 0.0f);

    EXPECT_FLOAT_EQ(duties.getWeightAt(1000), 0.0f);
    EXPECT_FLOAT_EQ(duties.getWeightAt(2000), 1.0f);
    EXPECT_FLOAT_EQ(duties.getWeightAt(8999), 1.0f);
    EXPECT_FLOAT_EQ(duties.getWeightAt(9000), 0.0f);
}

TEST(ScheduleBuilderTest, SwitchesActivitiesAtConfiguredBoundaries)
{
    Schedule schedule;
    ScheduleBuilder(schedule)
        .add(10, Activity::IDLE)
        .add(2000, Activity::WORK)
        .add(9000, Activity::MEET)
        .add(11000, Activity::IDLE)
        .add(12000, Activity::REST)
        .build();

    EXPECT_EQ(schedule.getScheduledActivity(0), Activity::REST);
    EXPECT_EQ(schedule.getScheduledActivity(10), Activity::IDLE);
    EXPECT_EQ(schedule.getScheduledActivity(1999), Activity::IDLE);
    EXPECT_EQ(schedule.getScheduledActivity(2000), Activity::WORK);
    EXPECT_EQ(schedule.getScheduledActivity(8999), Activity::WORK);
    EXPECT_EQ(schedule.getScheduledActivity(9000), Activity::MEET);
    EXPECT_EQ(schedule.getScheduledActivity(11000), Activity::IDLE);
    EXPECT_EQ(schedule.getScheduledActivity(12000), Activity::REST);
}

TEST(ScheduleTest, DirectAddMatchesBuilderSemantics)
{
    Schedule schedule;
    schedule.add(10, Activity::IDLE)
        .add(2000, Activity::WORK)
        .add(9000, Activity::MEET)
        .add(11000, Activity::IDLE)
        .add(12000, Activity::REST);

    EXPECT_EQ(schedule.getScheduledActivity(1500), Activity::IDLE);
    EXPECT_EQ(schedule.getScheduledActivity(5000), Activity::WORK);
    EXPECT_EQ(schedule.getScheduledActivity(9500), Activity::MEET);
    EXPECT_EQ(schedule.getScheduledActivity(11500), Activity::IDLE);
    EXPECT_EQ(schedule.getScheduledActivity(13000), Activity::REST);
}

} // namespace
} // namespace mc::entity::ai::brain::schedule
