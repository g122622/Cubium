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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/interfaces/IFlinging.hpp"

#include <type_traits>

namespace mc {
namespace {

class FlingingDummyTarget : public LivingEntity {
public:
    explicit FlingingDummyTarget(EntityInstanceId id)
        : LivingEntity(id, nullptr, mc::test::testEcsRegistry())
    {}
};

TEST(FlingingSupportTypesTest, HoglinAndZoglinImplementMarker)
{
    EXPECT_TRUE((std::is_base_of_v<entity::IFlinging, HoglinEntity>));
    EXPECT_TRUE((std::is_base_of_v<entity::IFlinging, ZoglinEntity>));
}

TEST(FlingingSupportTypesTest, AttackWithFlingAppliesDamageAndKnockback)
{
    HoglinEntity hoglin(EntityInstanceId(1), mc::test::testEcsRegistry());
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
    HoglinEntity hoglin(EntityInstanceId(1), mc::test::testEcsRegistry());
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
