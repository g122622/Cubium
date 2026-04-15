#include <gtest/gtest.h>

#include "common/entity/entities/projectile/AbstractFireballEntity.hpp"
#include "common/entity/entities/projectile/DamagingProjectileEntity.hpp"

#include <type_traits>

namespace mc {
namespace {

TEST(DamagingProjectileSupportTypesTest, FireballLayerInheritsDamagingProjectileEntity)
{
    EXPECT_TRUE((std::is_base_of_v<entity::DamagingProjectileEntity, entity::AbstractFireballEntity>));
}

TEST(DamagingProjectileSupportTypesTest, FireballDefaultsComeFromDamagingProjectileBase)
{
    entity::FireballEntity fireball(LegacyEntityType::Unknown, 1);

    EXPECT_FLOAT_EQ(fireball.damage(), 6.0f);
    EXPECT_TRUE(fireball.canBeCollidedWith());
    EXPECT_FLOAT_EQ(fireball.getCollisionBorderSize(), 1.0f);
}

TEST(DamagingProjectileSupportTypesTest, TickAppliesAccelerationUsingVanillaStyleMotionFactor)
{
    entity::FireballEntity fireball(LegacyEntityType::Unknown, 1);
    fireball.setPosition(0.0f, 64.0f, 0.0f);
    fireball.setVelocity(1.0f, 0.0f, 0.0f);
    fireball.setAcceleration(0.1f, 0.0f, 0.0f);

    fireball.tick();

    EXPECT_FLOAT_EQ(fireball.x(), 1.0f);
    EXPECT_FLOAT_EQ(fireball.y(), 64.0f);
    EXPECT_FLOAT_EQ(fireball.z(), 0.0f);
    EXPECT_NEAR(fireball.velocityX(), 1.045f, 1.0e-4f);
    EXPECT_FLOAT_EQ(fireball.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(fireball.velocityZ(), 0.0f);
}

} // namespace
} // namespace mc
