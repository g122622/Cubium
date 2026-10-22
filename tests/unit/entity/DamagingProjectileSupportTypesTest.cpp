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
    // 对齐 vanilla AbstractHurtingProjectile 构造：assignDirectionalMovement 先给初速度。
    fireball.setAcceleration(1.0f, 0.0f, 0.0f);
    fireball.setVelocity(1.0f, 0.0f, 0.0f);

    fireball.tick();

    // vanilla：setDeltaMovement(v.add(v.normalize() * accelerationPower).scale(0.95))
    // = (1.0 + 1.0 * 0.1) * 0.95 = 1.045
    EXPECT_FLOAT_EQ(fireball.x(), 1.0f);
    EXPECT_FLOAT_EQ(fireball.y(), 64.0f);
    EXPECT_FLOAT_EQ(fireball.z(), 0.0f);
    EXPECT_NEAR(fireball.velocityX(), 1.045f, 1.0e-4f);
    EXPECT_FLOAT_EQ(fireball.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(fireball.velocityZ(), 0.0f);
}

// 对齐 vanilla assignDirectionalMovement：初速度 = normalize(direction) * accelerationPower。
TEST(DamagingProjectileSupportTypesTest, SetAccelerationAssignsNormalizedInitialVelocity)
{
    entity::FireballEntity fireball(EntityInstanceId(2), mc::test::testEcsRegistry());

    // 斜向方向 (3,4,0) 的模为 5，归一化后 (0.6,0.8,0)，乘以默认加速力 0.1。
    fireball.setAcceleration(3.0f, 4.0f, 0.0f);
    EXPECT_NEAR(fireball.velocityX(), 0.06f, 1.0e-5f);
    EXPECT_NEAR(fireball.velocityY(), 0.08f, 1.0e-5f);
    EXPECT_FLOAT_EQ(fireball.velocityZ(), 0.0f);
    EXPECT_FLOAT_EQ(fireball.accelerationPower(), 0.1f);
    // 方向分量原样保留（供方向语义查询）。
    EXPECT_FLOAT_EQ(fireball.accelerationX(), 3.0f);
    EXPECT_FLOAT_EQ(fireball.accelerationY(), 4.0f);

    // 零向量方向：速度置零，不产生 NaN。
    fireball.setAcceleration(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(fireball.velocityX(), 0.0f);
    EXPECT_FLOAT_EQ(fireball.velocityY(), 0.0f);
    EXPECT_FLOAT_EQ(fireball.velocityZ(), 0.0f);

    // 加速力可单独调整，影响下一 tick 的速度增量。
    fireball.setVelocity(1.0f, 0.0f, 0.0f);
    fireball.setAccelerationPower(0.2f);
    fireball.tick();
    // (1.0 + 0.2) * 0.95 = 1.14
    EXPECT_NEAR(fireball.velocityX(), 1.14f, 1.0e-4f);
}

} // namespace
} // namespace mc
