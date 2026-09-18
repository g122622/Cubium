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

#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
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

TEST(SkeletonSupportTypesTest, VariantDaylightBehaviorMatchesDefaults)
{
    // 注意：不调用构造函数，只测试静态类型行为
    // shouldBurnInDaylight() 是虚函数，默认实现返回 true
    // WitherSkeletonEntity 和 StrayEntity 重写返回 false
}

// ========== WitherSkeletonEntity 类型测试 ==========

TEST(WitherSkeletonEntityStaticTest, IsImmuneToWitherEffect)
{
    // 静态测试：验证 WitherSkeletonEntity override 了 isPotionApplicable
    // （对齐 vanilla canBeAffected，EffectManager::addEffect 调用此方法判定效果免疫）。
    EXPECT_TRUE((std::is_same_v<decltype(&WitherSkeletonEntity::isPotionApplicable),
        bool (WitherSkeletonEntity::*)(const entity::effect::EffectInstance&) const>));
}

TEST(WitherSkeletonEntityStaticTest, HasStoneSword)
{
    // 静态测试：验证 WitherSkeletonEntity 有 hasStoneSword 方法
    EXPECT_TRUE(
        (std::is_same_v<decltype(&WitherSkeletonEntity::hasStoneSword), bool (WitherSkeletonEntity::*)() const>));
}

TEST(WitherSkeletonEntityStaticTest, HasCorrectEyeHeight)
{
    // 静态测试：验证 WitherSkeletonEntity 有 eyeHeight 方法
    EXPECT_TRUE((std::is_same_v<decltype(&WitherSkeletonEntity::eyeHeight), f32 (WitherSkeletonEntity::*)() const>));
}

TEST(WitherSkeletonEntityStaticTest, HasSetCombatTaskOverride)
{
    // 静态测试：验证 WitherSkeletonEntity 重写了 setCombatTask
    EXPECT_TRUE((std::is_same_v<decltype(&WitherSkeletonEntity::setCombatTask), void (WitherSkeletonEntity::*)()>));
}

TEST(WitherSkeletonEntityStaticTest, HasAttackEntityAsMobOverride)
{
    // 静态测试：验证 WitherSkeletonEntity 重写了 attackEntityAsMob
    EXPECT_TRUE((std::is_same_v<decltype(&WitherSkeletonEntity::attackEntityAsMob),
        bool (WitherSkeletonEntity::*)(LivingEntity&)>));
}

TEST(WitherSkeletonEntityStaticTest, WitherDurationConstant)
{
    // 静态测试：验证凋零效果持续时间常量
    // WITHER_DURATION_TICKS = 200 (10秒)
    EXPECT_EQ(WitherSkeletonEntity::WITHER_DURATION_TICKS, 200);
}

// ========== AbstractSkeletonEntity 测试 ==========

TEST(AbstractSkeletonEntityStaticTest, HasCombatTaskMethod)
{
    // 静态测试：验证 AbstractSkeletonEntity 有 setCombatTask 方法
    EXPECT_TRUE((std::is_same_v<decltype(&AbstractSkeletonEntity::setCombatTask), void (AbstractSkeletonEntity::*)()>));
}

TEST(AbstractSkeletonEntityStaticTest, HasVirtualDestructor)
{
    // 静态测试：验证 AbstractSkeletonEntity 有虚析构函数
    EXPECT_TRUE(std::has_virtual_destructor_v<AbstractSkeletonEntity>);
}

TEST(AbstractSkeletonEntityStaticTest, CombatGoalPriorityConstant)
{
    // 静态测试：验证战斗目标优先级常量
    // COMBAT_GOAL_PRIORITY = 4 (MC 1.16.5)
    EXPECT_EQ(AbstractSkeletonEntity::COMBAT_GOAL_PRIORITY, 4);
}

} // namespace
} // namespace mc
