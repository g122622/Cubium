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
    PillagerEntity pillager(LegacyEntityType::Pillager, 1);
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
    VindicatorEntity vindicator(LegacyEntityType::Vindicator, 2);

    vindicator.setPosition(20.0f, 70.0f, -15.0f);
    vindicator.resetPatrolTarget();

    EXPECT_TRUE(vindicator.isPatrolling());
    EXPECT_TRUE(vindicator.hasPatrolTarget());
    EXPECT_NE(vindicator.getPatrolTarget(), BlockPos(20, 70, -15));
}

} // namespace
} // namespace mc
