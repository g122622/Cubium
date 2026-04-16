#include <gtest/gtest.h>

#include "common/entity/ai/EntitySenses.hpp"
#include "common/entity/core/MobEntity.hpp"

namespace mc {
namespace {

TEST(EntitySensesTest, VisibleEntityIsCachedWithinSameTick)
{
    MobEntity observer(LegacyEntityType::Zombie, 1);
    MobEntity target(LegacyEntityType::Zombie, 2);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(4.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_TRUE(observer.senses()->canSee(target));

    target.setPosition(128.0f, 64.0f, 0.0f);
    EXPECT_TRUE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, CacheClearsOnTick)
{
    MobEntity observer(LegacyEntityType::Zombie, 3);
    MobEntity target(LegacyEntityType::Zombie, 4);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(4.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_TRUE(observer.senses()->canSee(target));

    target.setPosition(128.0f, 64.0f, 0.0f);
    observer.tick();

    EXPECT_FALSE(observer.senses()->canSee(target));
}

TEST(EntitySensesTest, InvisibleEntityIsCachedWithinSameTick)
{
    MobEntity observer(LegacyEntityType::Zombie, 5);
    MobEntity target(LegacyEntityType::Zombie, 6);

    observer.setPosition(0.0f, 64.0f, 0.0f);
    target.setPosition(128.0f, 64.0f, 0.0f);

    ASSERT_NE(observer.senses(), nullptr);
    EXPECT_FALSE(observer.senses()->canSee(target));

    target.setPosition(4.0f, 64.0f, 0.0f);
    EXPECT_FALSE(observer.senses()->canSee(target));
}

} // namespace
} // namespace mc
