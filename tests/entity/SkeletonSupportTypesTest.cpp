#include <gtest/gtest.h>

#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/undead/AbstractSkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/SkeletonEntity.hpp"
#include "common/entity/entities/monster/undead/StrayEntity.hpp"
#include "common/entity/entities/monster/undead/WitherSkeletonEntity.hpp"
#include "common/entity/interfaces/IMob.hpp"

#include <type_traits>

namespace mc {
namespace {

TEST(SkeletonSupportTypesTest, MonsterImplementsImobMarker)
{
    EXPECT_TRUE((std::is_base_of_v<entity::IMob, MonsterEntity>));
}

TEST(SkeletonSupportTypesTest, SkeletonVariantsInheritAbstractSkeletonEntity)
{
    EXPECT_TRUE((std::is_base_of_v<AbstractSkeletonEntity, SkeletonEntity>));
    EXPECT_TRUE((std::is_base_of_v<AbstractSkeletonEntity, StrayEntity>));
    EXPECT_TRUE((std::is_base_of_v<AbstractSkeletonEntity, WitherSkeletonEntity>));
}

TEST(SkeletonSupportTypesTest, BaseSkeletonTracksBowChargeTimers)
{
    SkeletonEntity skeleton(LegacyEntityType::Skeleton, 1);
    skeleton.setAttackTimer(2);
    skeleton.setAttackCooldown(4);

    skeleton.tick();
    EXPECT_TRUE(skeleton.isChargingBow());
    EXPECT_EQ(skeleton.getAttackTimer(), 1);
    EXPECT_EQ(skeleton.getAttackCooldown(), 3);

    skeleton.tick();
    EXPECT_FALSE(skeleton.isChargingBow());
    EXPECT_EQ(skeleton.getAttackTimer(), 0);
    EXPECT_EQ(skeleton.getAttackCooldown(), 2);
}

TEST(SkeletonSupportTypesTest, VariantDaylightBehaviorMatchesDefaults)
{
    SkeletonEntity skeleton(LegacyEntityType::Skeleton, 1);
    StrayEntity stray(LegacyEntityType::Stray, 2);
    WitherSkeletonEntity witherSkeleton(LegacyEntityType::WitherSkeleton, 3);

    EXPECT_TRUE(skeleton.shouldBurnInDaylight());
    EXPECT_FALSE(stray.shouldBurnInDaylight());
    EXPECT_FALSE(witherSkeleton.shouldBurnInDaylight());
}

} // namespace
} // namespace mc
