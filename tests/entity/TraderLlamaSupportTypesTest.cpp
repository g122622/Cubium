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

#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"

namespace mc {
namespace {

TEST(TraderLlamaEntityTest, InheritsFromLlamaAndExposesTraderFlag)
{
    TraderLlamaEntity traderLlama(EntityId(1));

    EXPECT_NE(dynamic_cast<LlamaEntity*>(&traderLlama), nullptr);
    EXPECT_TRUE(traderLlama.isTraderLlama());
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);
}

TEST(TraderLlamaEntityTest, UntamedTraderLlamaDespawnsWhenDelayExpires)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.setDespawnDelay(1);

    traderLlama.tick();

    EXPECT_TRUE(traderLlama.isRemoved());
}

TEST(TraderLlamaEntityTest, TamedOrRiddenTraderLlamaDoesNotDespawn)
{
    TraderLlamaEntity tamedTraderLlama(EntityId(1));
    tamedTraderLlama.setTame(true);
    tamedTraderLlama.setDespawnDelay(1);
    tamedTraderLlama.tick();
    EXPECT_FALSE(tamedTraderLlama.isRemoved());
    EXPECT_EQ(tamedTraderLlama.getDespawnDelay(), 1);

    TraderLlamaEntity riddenTraderLlama(EntityId(2));
    Player rider(1, "Steve");
    riddenTraderLlama.setRider(&rider);
    riddenTraderLlama.setDespawnDelay(1);
    riddenTraderLlama.tick();
    EXPECT_FALSE(riddenTraderLlama.isRemoved());
    EXPECT_EQ(riddenTraderLlama.getDespawnDelay(), 1);
}

} // namespace
} // namespace mc
