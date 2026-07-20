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

#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/entity/entities/monster/illager/IllagerEntities.hpp"
#include "common/entity/entities/monster/illager/PatrollerEntity.hpp"

#include <type_traits>

namespace mc {
namespace {

TEST(PatrollerSupportTypesTest, RaidersInheritPatrollerBase)
{
    EXPECT_TRUE((std::is_base_of_v<PatrollerEntity, AbstractRaiderEntity>));
    EXPECT_TRUE((std::is_base_of_v<PatrollerEntity, PillagerEntity>));
    EXPECT_TRUE((std::is_base_of_v<PatrollerEntity, VindicatorEntity>));
}

TEST(PatrollerSupportTypesTest, PatrolStateTracksLeaderAndTarget)
{
    PillagerEntity pillager(EntityInstanceId(1));
    const BlockPos patrolTarget(12, 64, -8);

    EXPECT_FALSE(pillager.isPatrolling());
    EXPECT_FALSE(pillager.hasPatrolTarget());
    EXPECT_FALSE(pillager.isLeader());

    pillager.setLeader(true);
    pillager.setPatrolTarget(patrolTarget);

    EXPECT_TRUE(pillager.isLeader());
    EXPECT_TRUE(pillager.isPatrolling());
    EXPECT_TRUE(pillager.hasPatrolTarget());
    EXPECT_EQ(pillager.getPatrolTarget(), patrolTarget);
}

TEST(PatrollerSupportTypesTest, ResetPatrolTargetCreatesNewTarget)
{
    VindicatorEntity vindicator(EntityInstanceId(2));

    vindicator.setPosition(20.0f, 70.0f, -15.0f);
    vindicator.resetPatrolTarget();

    EXPECT_TRUE(vindicator.isPatrolling());
    EXPECT_TRUE(vindicator.hasPatrolTarget());
    EXPECT_NE(vindicator.getPatrolTarget(), BlockPos(20, 70, -15));
}

} // namespace
} // namespace mc
