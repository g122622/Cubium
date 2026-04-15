#include <gtest/gtest.h>

#include "common/entity/entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "common/entity/entities/passive/fish/CodEntity.hpp"
#include "common/entity/entities/passive/fish/PufferfishEntity.hpp"
#include "common/entity/entities/passive/fish/SalmonEntity.hpp"
#include "common/entity/entities/passive/fish/TropicalFishEntity.hpp"

namespace mc {
namespace {

TEST(AbstractGroupFishEntityTest, FollowerJoinAndLeaveUpdatesLeaderState)
{
    CodEntity leader(LegacyEntityType::Cod, 1);
    SalmonEntity follower(LegacyEntityType::Salmon, 2);

    EXPECT_FALSE(leader.isGroupLeader());
    EXPECT_EQ(leader.getGroupSize(), 1);
    EXPECT_FALSE(follower.hasGroupLeader());

    follower.joinGroup(leader);

    EXPECT_TRUE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), &leader);
    EXPECT_EQ(leader.getGroupSize(), 2);
    EXPECT_TRUE(leader.isGroupLeader());
    EXPECT_TRUE(leader.canGroupGrow());

    follower.leaveGroup();

    EXPECT_FALSE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), nullptr);
    EXPECT_EQ(leader.getGroupSize(), 1);
    EXPECT_FALSE(leader.isGroupLeader());
}

TEST(AbstractGroupFishEntityTest, UsesVanillaLeaderRangeAndClearsDeadLeaderOnTick)
{
    CodEntity leader(LegacyEntityType::Cod, 1);
    TropicalFishEntity follower(LegacyEntityType::TropicalFish, 2);

    leader.setPosition(0.0f, 62.0f, 0.0f);
    follower.setPosition(11.0f, 62.0f, 0.0f);
    follower.joinGroup(leader);

    EXPECT_FLOAT_EQ(follower.getSchoolingRange(), 11.0f);
    EXPECT_TRUE(follower.inRangeOfGroupLeader());

    follower.setPosition(11.1f, 62.0f, 0.0f);
    EXPECT_FALSE(follower.inRangeOfGroupLeader());

    leader.remove();
    follower.tick();

    EXPECT_FALSE(follower.hasGroupLeader());
    EXPECT_EQ(follower.getGroupLeader(), nullptr);
}

TEST(FishSupportTypesTest, SchoolingFishUseGroupLayerButPufferfishDoesNot)
{
    CodEntity cod(LegacyEntityType::Cod, 1);
    SalmonEntity salmon(LegacyEntityType::Salmon, 2);
    TropicalFishEntity tropicalFish(LegacyEntityType::TropicalFish, 3);
    PufferfishEntity pufferfish(LegacyEntityType::Pufferfish, 4);

    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&cod), nullptr);
    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&salmon), nullptr);
    EXPECT_NE(dynamic_cast<AbstractGroupFishEntity*>(&tropicalFish), nullptr);
    EXPECT_EQ(dynamic_cast<AbstractGroupFishEntity*>(&pufferfish), nullptr);

    EXPECT_TRUE(cod.canSchool());
    EXPECT_TRUE(salmon.canSchool());
    EXPECT_TRUE(tropicalFish.canSchool());
    EXPECT_FALSE(pufferfish.canSchool());

    EXPECT_EQ(cod.getMaxGroupSize(), 8);
    EXPECT_EQ(salmon.getMaxGroupSize(), 5);
    EXPECT_EQ(tropicalFish.getMaxGroupSize(), 8);
}

} // namespace
} // namespace mc
