#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/interfaces/IFlinging.hpp"

#include <type_traits>

namespace mc {
namespace {

class FlingingDummyTarget : public LivingEntity {
public:
    explicit FlingingDummyTarget(EntityId id)
        : LivingEntity(LegacyEntityType::Unknown, id)
    {}
};

TEST(FlingingSupportTypesTest, HoglinAndZoglinImplementMarker)
{
    EXPECT_TRUE((std::is_base_of_v<entity::IFlinging, HoglinEntity>));
    EXPECT_TRUE((std::is_base_of_v<entity::IFlinging, ZoglinEntity>));
}

TEST(FlingingSupportTypesTest, AttackWithFlingAppliesDamageAndKnockback)
{
    HoglinEntity hoglin(LegacyEntityType::Hoglin, 1);
    hoglin.setPosition(0.0f, 64.0f, 0.0f);

    FlingingDummyTarget target(2);
    target.setPosition(2.0f, 64.0f, 0.0f);
    target.setHealth(20.0f);

    const f32 healthBefore = target.health();

    ASSERT_TRUE(entity::IFlinging::attackWithFling(hoglin, target, false));
    EXPECT_LT(target.health(), healthBefore);
    EXPECT_GT(target.velocityX(), 0.0f);
    EXPECT_GT(target.velocityY(), 0.0f);
}

TEST(FlingingSupportTypesTest, HoglinAttackUpdatesAnimationTicks)
{
    HoglinEntity hoglin(LegacyEntityType::Hoglin, 1);
    hoglin.setPosition(0.0f, 64.0f, 0.0f);

    FlingingDummyTarget target(2);
    target.setPosition(2.0f, 64.0f, 0.0f);

    ASSERT_TRUE(hoglin.attackLivingTarget(target));
    EXPECT_EQ(hoglin.getFlingAnimationTicks(), 10);

    hoglin.tick();
    EXPECT_EQ(hoglin.getFlingAnimationTicks(), 9);
}

} // namespace
} // namespace mc
