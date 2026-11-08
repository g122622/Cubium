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

TEST(FlingingSupportTypesTest, HurtAndThrowTargetAppliesDamageAndKnockback)
{
    HoglinEntity hoglin(EntityInstanceId(1), mc::test::testEcsRegistry());
    hoglin.setPosition(0.0f, 64.0f, 0.0f);

    FlingingDummyTarget target(2);
    target.setPosition(2.0f, 64.0f, 0.0f);
    target.setHealth(20.0f);

    const f32 healthBefore = target.health();

    ASSERT_TRUE(entity::IFlinging::hurtAndThrowTarget(hoglin, target));
    EXPECT_LT(target.health(), healthBefore);
    // 抛飞会同时给水平与垂直速度（vanilla throwTarget 的 push(x, rand, z)）。
    EXPECT_GT(target.velocityX(), 0.0f);
    EXPECT_GT(target.velocityY(), 0.0f);
    // 击退需要同步到客户端，因此必须置上 hurtMarked。
    EXPECT_TRUE(target.isHurtMarked());
}

TEST(FlingingSupportTypesTest, HoglinAttackUpdatesAnimationTicks)
{
    HoglinEntity hoglin(EntityInstanceId(1), mc::test::testEcsRegistry());
    hoglin.setPosition(0.0f, 64.0f, 0.0f);

    FlingingDummyTarget target(2);
    target.setPosition(2.0f, 64.0f, 0.0f);

    // 通过 attackEntityAsMob 调用（MeleeAttackGoal 委托的真正攻击入口）。
    ASSERT_TRUE(hoglin.attackEntityAsMob(target));
    EXPECT_EQ(hoglin.getFlingAnimationTicks(), 10);

    hoglin.tick();
    EXPECT_EQ(hoglin.getFlingAnimationTicks(), 9);
}

TEST(FlingingSupportTypesTest, AttackAppliesDamageExactlyOnce)
{
    // attackEntityAsMob 只做表现层，伤害由 hurtAndThrowTarget 施加一次。
    // 若两者都调 hurt，这里会看到双倍伤害——用单次攻击的血量上界锁住这条约束。
    HoglinEntity hoglin(EntityInstanceId(1), mc::test::testEcsRegistry());
    hoglin.setPosition(0.0f, 64.0f, 0.0f);

    FlingingDummyTarget target(2);
    target.setPosition(2.0f, 64.0f, 0.0f);
    // 以目标自身的最大生命做基线：setHealth 会被夹到最大生命，不能用外部数值当基准。
    const f32 healthBefore = target.health();

    ASSERT_TRUE(hoglin.attackEntityAsMob(target));

    // 成年 Hoglin 单次伤害上界为 damage/2 + rand[0, damage)，即 < 1.5 * ATTACK_DAMAGE；
    // 这里取 3 倍作宽松上界，只为排除"双倍施加"。
    const f32 attackDamage =
        static_cast<f32>(hoglin.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));
    const f32 damageTaken = healthBefore - target.health();
    EXPECT_GT(damageTaken, 0.0f);
    EXPECT_LT(damageTaken, attackDamage * 3.0f);
}

} // namespace
} // namespace mc
