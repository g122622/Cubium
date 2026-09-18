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
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/entity/entities/passive/tamable/ShoulderRidingEntity.hpp"

namespace mc {
namespace {

TEST(ShoulderRidingEntityTest, ParrotUsesShoulderRidingLayer)
{
    ParrotEntity parrot(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_NE(dynamic_cast<ShoulderRidingEntity*>(&parrot), nullptr);
    EXPECT_FALSE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 0u);
}

TEST(ShoulderRidingEntityTest, RequiresCooldownTameStateAndStandingBeforeMounting)
{
    ParrotEntity parrot(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FALSE(parrot.canSitOnShoulder());
    EXPECT_FALSE(parrot.mountShoulder(42));

    parrot.setTamed(true);
    for (int i = 0; i < 101; ++i) {
        parrot.tick();
    }

    EXPECT_TRUE(parrot.canSitOnShoulder());
    EXPECT_TRUE(parrot.mountShoulder(42));
    EXPECT_TRUE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 42u);

    parrot.dismountShoulder();
    EXPECT_FALSE(parrot.isOnShoulder());
    EXPECT_EQ(parrot.getShoulderPlayerId(), 0u);
    EXPECT_FALSE(parrot.canSitOnShoulder());

    for (int i = 0; i < 101; ++i) {
        parrot.tick();
    }

    parrot.setSitting(true);
    EXPECT_FALSE(parrot.mountShoulder(99));
}

} // namespace
} // namespace mc
