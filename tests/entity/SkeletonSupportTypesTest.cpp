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
