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

#include "common/TestWorldHelper.hpp"
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
    entity::FireballEntity fireball(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(fireball.damage(), 6.0f);
    EXPECT_TRUE(fireball.canBeCollidedWith());
    EXPECT_FLOAT_EQ(fireball.getCollisionBorderSize(), 1.0f);
}

TEST(DamagingProjectileSupportTypesTest, TickAppliesAccelerationUsingVanillaStyleMotionFactor)
{
    entity::FireballEntity fireball(EntityInstanceId(1), mc::test::testEcsRegistry());
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
