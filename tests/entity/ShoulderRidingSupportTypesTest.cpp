#include <gtest/gtest.h>

#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/entity/entities/passive/tamable/ShoulderRidingEntity.hpp"

namespace mc {
namespace {

TEST(ShoulderRidingEntityTest, ParrotUsesShoulderRidingLayer)
{
    ParrotEntity parrot(LegacyEntityType::Parrot, 1);

    EXPECT_NE(dynamic_cast<ShoulderRidingEntity*>(&parrot), nullptr);
    EXPECT_FALSE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 0u);
}

TEST(ShoulderRidingEntityTest, RequiresCooldownTameStateAndStandingBeforeMounting)
{
    ParrotEntity parrot(LegacyEntityType::Parrot, 1);

    EXPECT_FALSE(parrot.canSitOnShoulder());
    EXPECT_FALSE(parrot.mountShoulder(42));

    parrot.setTamed(true);
    for (int i = 0; i < 101; ++i) {
        parrot.tick();
    }

    EXPECT_TRUE(parrot.canSitOnShoulder());
    EXPECT_TRUE(parrot.mountShoulder(42));
    EXPECT_TRUE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 42u);

    parrot.dismountShoulder();
    EXPECT_FALSE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 0u);
    EXPECT_FALSE(parrot.canSitOnShoulder());

    for (int i = 0; i < 101; ++i) {
        parrot.tick();
    }

    parrot.setSitting(true);
    EXPECT_FALSE(parrot.mountShoulder(99));
}

} // namespace
} // namespace mc
