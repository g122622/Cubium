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

TEST(TraderLlamaEntityTest, DefaultDespawnDelayIsCorrect)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    EXPECT_EQ(traderLlama.getDespawnDelay(), TraderLlamaEntity::DEFAULT_DESPAWN_DELAY);
}

TEST(TraderLlamaEntityTest, SetDespawnDelay)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.setDespawnDelay(100);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 100);
}

TEST(TraderLlamaEntityTest, SyncDespawnDelayFromTrader)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.syncDespawnDelayFromTrader(48000);
    EXPECT_EQ(traderLlama.getDespawnDelay(), 47999);
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsTrueWhenUntamedAndUnleashed)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    // 默认状态：未驯服、未拴绳
    EXPECT_TRUE(traderLlama.canDespawn(128.0));
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsFalseWhenTamed)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.setTame(true);
    EXPECT_FALSE(traderLlama.canDespawn(128.0));
}

TEST(TraderLlamaEntityTest, CanDespawnReturnsFalseWhenLeashed)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    // 设置拴绳状态（拴到栅栏上）
    traderLlama.setLeashedToFence(BlockPos(0, 64, 0));
    EXPECT_FALSE(traderLlama.canDespawn(128.0));
}

TEST(TraderLlamaEntityTest, TamedTraderLlamaDoesNotDespawnViaCanDespawn)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.setTame(true);
    traderLlama.setDespawnDelay(1);
    // 驯服的商队羊驼不应消失
    EXPECT_FALSE(traderLlama.canDespawn(0.0));
}

TEST(TraderLlamaEntityTest, LeashedTraderLlamaDoesNotDespawnViaCanDespawn)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    traderLlama.setLeashedToFence(BlockPos(0, 64, 0));
    traderLlama.setDespawnDelay(1);
    // 被拴绳拴住的商队羊驼不应消失
    EXPECT_FALSE(traderLlama.canDespawn(0.0));
}

TEST(TraderLlamaEntityTest, IsTraderLlamaFlag)
{
    TraderLlamaEntity traderLlama(EntityId(1));
    EXPECT_TRUE(traderLlama.isTraderLlama());

    // TraderLlamaEntity 可以向上转型为 LlamaEntity
    LlamaEntity* llamaPtr = &traderLlama;
    EXPECT_NE(llamaPtr, nullptr);
}

} // namespace
} // namespace mc
