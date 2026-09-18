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
#include "common/entity/entities/passive/horse/AbstractChestedHorseEntity.hpp"
#include "common/entity/entities/passive/horse/DonkeyEntity.hpp"
#include "common/entity/entities/passive/horse/HorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/MuleEntity.hpp"

namespace mc {
namespace {

TEST(AbstractChestedHorseEntityTest, CoversChestHorseSubtypesButNotPlainHorse)
{
    HorseEntity horse(EntityInstanceId(1), mc::test::testEcsRegistry());
    DonkeyEntity donkey(EntityInstanceId(2), mc::test::testEcsRegistry());
    MuleEntity mule(EntityInstanceId(3), mc::test::testEcsRegistry());
    LlamaEntity llama(EntityInstanceId(4), mc::test::testEcsRegistry());

    EXPECT_EQ(dynamic_cast<AbstractChestedHorseEntity*>(&horse), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&donkey), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&mule), nullptr);
    EXPECT_NE(dynamic_cast<AbstractChestedHorseEntity*>(&llama), nullptr);
}

TEST(AbstractChestedHorseEntityTest, UsesVanillaStyleChestInventorySizing)
{
    DonkeyEntity donkey(EntityInstanceId(1), mc::test::testEcsRegistry());
    MuleEntity mule(EntityInstanceId(2), mc::test::testEcsRegistry());
    LlamaEntity llama(EntityInstanceId(3), mc::test::testEcsRegistry());

    EXPECT_FALSE(donkey.hasChest());
    EXPECT_EQ(donkey.getInventorySize(), 2);
    donkey.setChest(true);
    EXPECT_EQ(donkey.getInventoryColumns(), 5);
    EXPECT_EQ(donkey.getInventorySize(), 17);

    EXPECT_FALSE(mule.hasChest());
    EXPECT_EQ(mule.getInventorySize(), 2);
    mule.setChest(true);
    EXPECT_EQ(mule.getInventoryColumns(), 5);
    EXPECT_EQ(mule.getInventorySize(), 17);

    llama.setStrength(1);
    EXPECT_EQ(llama.getInventorySize(), 2);
    EXPECT_EQ(llama.getInventoryColumns(), 1);
    llama.setStrength(5);
    llama.setChest(true);
    EXPECT_EQ(llama.getInventoryColumns(), 5);
    EXPECT_EQ(llama.getInventorySize(), 17);
}

TEST(HorseSupportTypesTest, LlamaStrengthIsClampedToVanillaRange)
{
    LlamaEntity llama(EntityInstanceId(1), mc::test::testEcsRegistry());

    llama.setStrength(0);
    EXPECT_EQ(llama.getStrength(), 1);

    llama.setStrength(3);
    EXPECT_EQ(llama.getStrength(), 3);
    EXPECT_EQ(llama.getInventoryColumns(), 3);

    llama.setStrength(8);
    EXPECT_EQ(llama.getStrength(), 5);
    EXPECT_EQ(llama.getInventoryColumns(), 5);
}

} // namespace
} // namespace mc
