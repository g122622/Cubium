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
 */

#include <gtest/gtest.h>

#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "server/world/GenerationChunkCache.hpp"

using namespace mc;
using namespace mc::server;

TEST(GenerationChunkCache, BasicConstruction)
{
    GenerationChunkCache cache(0, 0, 0);
    EXPECT_EQ(cache.centerX(), 0);
    EXPECT_EQ(cache.centerZ(), 0);
    EXPECT_EQ(cache.radius(), 0);
    EXPECT_EQ(cache.diameter(), 1);
}

TEST(GenerationChunkCache, RadiusOneConstruction)
{
    GenerationChunkCache cache(5, 10, 1);
    EXPECT_EQ(cache.centerX(), 5);
    EXPECT_EQ(cache.centerZ(), 10);
    EXPECT_EQ(cache.radius(), 1);
    EXPECT_EQ(cache.diameter(), 3);
}

TEST(GenerationChunkCache, RadiusEightConstruction)
{
    GenerationChunkCache cache(0, 0, 8);
    EXPECT_EQ(cache.radius(), 8);
    EXPECT_EQ(cache.diameter(), 17);
}

TEST(GenerationChunkCache, SetAndGet)
{
    GenerationChunkCache cache(0, 0, 1);
    ChunkPrimer primer(0, 0);

    cache.set(0, 0, &primer);
    EXPECT_EQ(cache.get(0, 0), &primer);
    EXPECT_EQ(cache.get(1, 0), nullptr);
    EXPECT_EQ(cache.get(0, 1), nullptr);
    EXPECT_EQ(cache.get(-1, 0), nullptr);
}

TEST(GenerationChunkCache, SetAndGetNeighbors)
{
    GenerationChunkCache cache(0, 0, 1);
    ChunkPrimer center(0, 0);
    ChunkPrimer east(1, 0);
    ChunkPrimer south(0, 1);

    cache.set(0, 0, &center);
    cache.set(1, 0, &east);
    cache.set(0, 1, &south);

    EXPECT_EQ(cache.get(0, 0), &center);
    EXPECT_EQ(cache.get(1, 0), &east);
    EXPECT_EQ(cache.get(0, 1), &south);
    EXPECT_EQ(cache.get(-1, 0), nullptr);
    EXPECT_EQ(cache.get(1, 1), nullptr);
}

TEST(GenerationChunkCache, ContainsInBounds)
{
    GenerationChunkCache cache(5, 5, 2);
    EXPECT_TRUE(cache.contains(5, 5));
    EXPECT_TRUE(cache.contains(3, 3));
    EXPECT_TRUE(cache.contains(7, 7));
    EXPECT_FALSE(cache.contains(2, 5));
    EXPECT_FALSE(cache.contains(5, 8));
    EXPECT_FALSE(cache.contains(8, 8));
}

TEST(GenerationChunkCache, ContainsWithOffset)
{
    GenerationChunkCache cache(100, -50, 4);
    EXPECT_TRUE(cache.contains(100, -50));
    EXPECT_TRUE(cache.contains(96, -54));
    EXPECT_TRUE(cache.contains(104, -46));
    EXPECT_FALSE(cache.contains(95, -50));
    EXPECT_FALSE(cache.contains(105, -50));
}

TEST(GenerationChunkCache, GetOrFallback)
{
    GenerationChunkCache cache(0, 0, 1);
    ChunkPrimer primer(0, 0);
    ChunkPrimer fallback(99, 99);

    cache.set(0, 0, &primer);

    EXPECT_EQ(cache.getOrFallback(0, 0, &fallback), &primer);
    EXPECT_EQ(cache.getOrFallback(1, 0, &fallback), &fallback);
    EXPECT_EQ(cache.getOrFallback(0, 1, &fallback), &fallback);
    EXPECT_EQ(cache.getOrFallback(1, 0, nullptr), nullptr);
}

TEST(GenerationChunkCache, LargeRadius)
{
    GenerationChunkCache cache(0, 0, 8);

    // 设置所有位置的 Primer
    for (i32 dz = -8; dz <= 8; ++dz) {
        for (i32 dx = -8; dx <= 8; ++dx) {
            // 不实际创建 289 个 ChunkPrimer（太昂贵），
            // 只测试边界上的位置包含关系
            EXPECT_TRUE(cache.contains(dx, dz));
        }
    }

    EXPECT_FALSE(cache.contains(9, 0));
    EXPECT_FALSE(cache.contains(0, -9));
    EXPECT_FALSE(cache.contains(-9, -9));
}
